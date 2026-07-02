#include "user_app.h"

#include "assistant_client.h"
#include "assistant_model.h"
#include "assistant_ui.h"
#include "button.h"
#include "epaper_config.h"
#include "memo_recorder.h"
#include "memo_storage.h"
#include "port_adc.h"
#include "port_codec.h"
#include "port_flash_storage.h"
#include "port_i2c.h"
#include "port_lvgl.h"
#include "port_power.h"
#include "port_sdcard.h"
#include "power_sleep_policy.h"

#include <driver/gpio.h>
#include <driver/rtc_io.h>
#include <esp_err.h>
#include <esp_log.h>
#include <esp_sleep.h>
#include <freertos/FreeRTOS.h>
#include <freertos/event_groups.h>
#include <freertos/task.h>

#include <cassert>
#include <string>
#include <unistd.h>

namespace {

constexpr const char *TAG = "ai_voice";

constexpr EventBits_t BIT_BOOT_DOWN = 0x01;
constexpr EventBits_t BIT_BOOT_UP = 0x02;
constexpr EventBits_t BIT_PWR_LONG = 0x04;
constexpr EventBits_t BIT_PWR_UP = 0x08;
constexpr EventBits_t BIT_RECORD_ERROR = 0x10;
constexpr EventBits_t BIT_UPLOAD_DONE = 0x20;
constexpr EventBits_t BIT_UPLOAD_ERROR = 0x40;

constexpr EventBits_t BIT_REC_WORKER_START = 0x01;
constexpr EventBits_t BIT_REC_WORKER_STOP = 0x02;
constexpr EventBits_t BIT_REC_WORKER_STOPPED = 0x04;

Button *boot_button = nullptr;
Button *power_button = nullptr;
EventGroupHandle_t app_events = nullptr;
EventGroupHandle_t recorder_events = nullptr;

I2cMasterBus *i2c_bus = nullptr;
AssistantUi assistant_ui;
AssistantSession session;
MemoStorage storage;
MemoRecorder recorder;
WavFormat wav_format;
AssistantResponse upload_response;

bool sd_ready = false;
bool flash_ready = false;
bool power_button_released = false;
bool recording_worker_active = false;
bool upload_worker_active = false;
esp_err_t recorder_error = ESP_OK;
esp_err_t upload_error = ESP_OK;
std::string temp_wav_path;
uint32_t last_record_elapsed = UINT32_MAX;

bool StorageReady() {
    return sd_ready || flash_ready;
}

void RenderHome() {
    if (Lvgl_lock(-1)) {
        assistant_ui.ShowHome(AssistantWifiConfigured(), AssistantProxyConfigured());
        Lvgl_unlock();
    }
}

void RenderRecording(uint32_t elapsed_seconds) {
    if (Lvgl_lock(-1)) {
        assistant_ui.ShowRecording(elapsed_seconds);
        Lvgl_unlock();
    }
}

void RenderBusy(AssistantState state) {
    if (Lvgl_lock(-1)) {
        assistant_ui.ShowBusy(state);
        Lvgl_unlock();
    }
}

void RenderAnswer() {
    if (Lvgl_lock(-1)) {
        assistant_ui.ShowAnswer(session.answer, session.request_id);
        Lvgl_unlock();
    }
}

void RenderError(AssistantError error) {
    if (Lvgl_lock(-1)) {
        assistant_ui.ShowError(error);
        Lvgl_unlock();
    }
}

void RenderPowerOff() {
    if (Lvgl_lock(-1)) {
        assistant_ui.ShowPowerOff();
        Lvgl_unlock();
    }
}

void ScanInitialPowerButtonRelease() {
    if (!power_button_released && gpio_get_level(PWR_BUTTON_PIN) == 1) {
        power_button_released = true;
    }
}

void InitializeButtons() {
    boot_button->OnPressDown([]() {
        xEventGroupSetBits(app_events, BIT_BOOT_DOWN);
    });
    boot_button->OnPressUp([]() {
        xEventGroupSetBits(app_events, BIT_BOOT_UP);
    });
    power_button->OnLongPress([]() {
        xEventGroupSetBits(app_events, BIT_PWR_LONG);
    });
    power_button->OnPressUp([]() {
        xEventGroupSetBits(app_events, BIT_PWR_UP);
    });
}

std::string BuildTempWavPath() {
    return storage.BaseDir() + "/assistant_last.wav";
}

void StartRecording() {
    if (recording_worker_active || upload_worker_active || recorder.IsRecording()) {
        return;
    }
    if (!StorageReady()) {
        ApplyAssistantError(session, AssistantError::StorageFailed);
        RenderError(session.error);
        return;
    }

    temp_wav_path = BuildTempWavPath();
    unlink(temp_wav_path.c_str());
    ApplyAssistantEvent(session, AssistantEvent::BootDown);
    last_record_elapsed = UINT32_MAX;

    esp_err_t ret = recorder.Start(temp_wav_path.c_str(), wav_format);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "record start failed: %s", esp_err_to_name(ret));
        ApplyAssistantError(session, AssistantError::RecordFailed);
        RenderError(session.error);
        return;
    }

    recording_worker_active = true;
    xEventGroupSetBits(recorder_events, BIT_REC_WORKER_START);
    RenderRecording(0);
}

void StopRecordingAndUpload() {
    if (!recording_worker_active && !recorder.IsRecording()) {
        return;
    }

    recording_worker_active = false;
    xEventGroupSetBits(recorder_events, BIT_REC_WORKER_STOP);
    xEventGroupWaitBits(recorder_events, BIT_REC_WORKER_STOPPED, pdTRUE, pdFALSE, portMAX_DELAY);

    MemoMetadata saved = {};
    esp_err_t ret = recorder.Stop(&saved);
    if (ret != ESP_OK || saved.data_bytes == 0) {
        ESP_LOGE(TAG, "record stop failed: %s bytes=%u", esp_err_to_name(ret), static_cast<unsigned>(saved.data_bytes));
        ApplyAssistantError(session, AssistantError::RecordFailed);
        RenderError(session.error);
        return;
    }

    temp_wav_path = saved.path;
    ApplyAssistantEvent(session, AssistantEvent::BootUp);
    RenderBusy(AssistantState::Uploading);
    upload_worker_active = true;
    xTaskCreatePinnedToCore([](void *) {
        upload_response = AssistantResponse{};
        upload_error = AssistantUploadWav(temp_wav_path.c_str(), &upload_response);
        xEventGroupSetBits(app_events, upload_error == ESP_OK ? BIT_UPLOAD_DONE : BIT_UPLOAD_ERROR);
        upload_worker_active = false;
        vTaskDelete(nullptr);
    }, "AiUpload", 8 * 1024, nullptr, 3, nullptr, 1);
}

void Shutdown() {
    if (recording_worker_active || recorder.IsRecording() || upload_worker_active) {
        ApplyAssistantError(session, AssistantError::Busy);
        RenderError(session.error);
        return;
    }

    ApplyAssistantEvent(session, AssistantEvent::PowerLong);
    RenderPowerOff();
    vTaskDelay(pdMS_TO_TICKS(1500));
    BoardPower_Audio_OFF();
    BoardPower_VBAT_OFF();

    power_button_released = false;
    ESP_LOGI(TAG, "Waiting for PWR release before deep sleep");
    while (gpio_get_level(PWR_BUTTON_PIN) == 0) {
        vTaskDelay(pdMS_TO_TICKS(20));
    }
    power_button_released = true;

    if (ShouldEnterPowerButtonWakeSleep(power_button_released)) {
        ESP_LOGI(TAG, "Entering deep sleep, wake on PWR low");
        ESP_ERROR_CHECK_WITHOUT_ABORT(rtc_gpio_pullup_en(PWR_BUTTON_PIN));
        ESP_ERROR_CHECK_WITHOUT_ABORT(rtc_gpio_pulldown_dis(PWR_BUTTON_PIN));
        ESP_ERROR_CHECK_WITHOUT_ABORT(esp_sleep_enable_ext0_wakeup(PWR_BUTTON_PIN, 0));
        esp_deep_sleep_start();
    }
}

void AppLoopTask(void *) {
    if (StorageReady()) {
        temp_wav_path = BuildTempWavPath();
    }
    RenderHome();

    for (;;) {
        EventBits_t events = xEventGroupWaitBits(
            app_events,
            BIT_BOOT_DOWN | BIT_BOOT_UP | BIT_PWR_LONG | BIT_PWR_UP | BIT_RECORD_ERROR | BIT_UPLOAD_DONE | BIT_UPLOAD_ERROR,
            pdTRUE,
            pdFALSE,
            pdMS_TO_TICKS(recording_worker_active ? 100 : 500));

        if (events & BIT_PWR_LONG) {
            if (power_button_released) {
                power_button_released = false;
                Shutdown();
            } else {
                ESP_LOGI(TAG, "Ignore PWR long press before first release");
            }
        }
        if (events & BIT_PWR_UP) {
            if (!recording_worker_active && !upload_worker_active && (session.state == AssistantState::Answer || session.state == AssistantState::Error)) {
                session = AssistantSession{};
                RenderHome();
            }
            power_button_released = true;
        }
        if (events & BIT_BOOT_DOWN) {
            StartRecording();
        }
        if (events & BIT_BOOT_UP) {
            StopRecordingAndUpload();
        }
        if (events & BIT_RECORD_ERROR) {
            ESP_LOGE(TAG, "recording failed: %s", esp_err_to_name(recorder_error));
            recorder.Abort();
            recording_worker_active = false;
            ApplyAssistantError(session, AssistantError::RecordFailed);
            RenderError(session.error);
        }
        if (events & BIT_UPLOAD_DONE) {
            ApplyAssistantAnswer(session, upload_response.text, upload_response.request_id);
            RenderAnswer();
        }
        if (events & BIT_UPLOAD_ERROR) {
            ESP_LOGE(TAG, "upload failed: %s error=%d", esp_err_to_name(upload_error), static_cast<int>(upload_response.error));
            ApplyAssistantError(session, upload_response.error == AssistantError::None ? AssistantError::UploadFailed : upload_response.error);
            RenderError(session.error);
        }

        if (recording_worker_active) {
            uint32_t elapsed = recorder.ElapsedSeconds();
            if (elapsed != last_record_elapsed) {
                last_record_elapsed = elapsed;
                session.elapsed_seconds = elapsed;
                RenderRecording(elapsed);
            }
        }
    }
}

void RecorderLoopTask(void *) {
    for (;;) {
        xEventGroupWaitBits(recorder_events, BIT_REC_WORKER_START, pdTRUE, pdFALSE, portMAX_DELAY);
        xEventGroupClearBits(recorder_events, BIT_REC_WORKER_STOPPED);
        recorder_error = ESP_OK;

        while (recorder.IsRecording()) {
            EventBits_t bits = xEventGroupGetBits(recorder_events);
            if (bits & BIT_REC_WORKER_STOP) {
                xEventGroupClearBits(recorder_events, BIT_REC_WORKER_STOP);
                break;
            }
            esp_err_t ret = recorder.CaptureChunk();
            if (ret != ESP_OK) {
                recorder_error = ret;
                xEventGroupSetBits(app_events, BIT_RECORD_ERROR);
                break;
            }
        }

        xEventGroupSetBits(recorder_events, BIT_REC_WORKER_STOPPED);
    }
}

}  // namespace

void UserApp_Init() {
    BoardPower_Init();
    BoardPower_EPD_ON();
    BoardPower_Audio_ON();
    BoardPower_VBAT_ON();

    app_events = xEventGroupCreate();
    recorder_events = xEventGroupCreate();

    i2c_bus = I2cMasterBus::requestInstance(ESP32_I2C_SCL_PIN, ESP32_I2C_SDA_PIN, ESP32_I2C_DEV_NUM);
    assert(i2c_bus);

    boot_button = new Button(BOOT_BUTTON_PIN);
    power_button = new Button(PWR_BUTTON_PIN);
    InitializeButtons();
    ScanInitialPowerButtonRelease();

    sd_ready = Sdcard_Init();
    if (sd_ready) {
        storage.SetBaseDir("/sdcard/assistant");
        ESP_LOGI(TAG, "Using SD assistant storage");
    } else {
        flash_ready = FlashStorage_Init();
        if (flash_ready) {
            storage.SetBaseDir("/flash/assistant");
            ESP_LOGI(TAG, "Using flash assistant storage");
        } else {
            ESP_LOGE(TAG, "No assistant storage available");
        }
    }
    if (StorageReady()) {
        esp_err_t ret = storage.Init();
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "storage init failed: %s", esp_err_to_name(ret));
            sd_ready = false;
            flash_ready = false;
        }
    }

    BoardAdc_Init();
    Codec_StartInit();
}

void UserUi_Init() {
    assistant_ui.Init();
}

void UserApp_Start_Init() {
    xTaskCreatePinnedToCore(AppLoopTask, "AiVoiceApp", 8 * 1024, nullptr, 3, nullptr, 1);
    xTaskCreatePinnedToCore(RecorderLoopTask, "AiRecorder", 6 * 1024, nullptr, 5, nullptr, 1);
}
