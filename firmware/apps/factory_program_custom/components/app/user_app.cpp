#include "user_app.h"

#include <freertos/FreeRTOS.h>
#include <freertos/event_groups.h>
#include <freertos/queue.h>
#include <freertos/task.h>
#include <driver/gpio.h>
#include <esp_err.h>
#include <esp_log.h>

#include "button.h"
#include "epaper_config.h"
#include "memo_player.h"
#include "memo_recorder.h"
#include "memo_storage.h"
#include "memo_utils.h"
#include "pcf85063a.h"
#include "port_adc.h"
#include "port_codec.h"
#include "port_ft6336.h"
#include "port_i2c.h"
#include "port_lvgl.h"
#include "port_power.h"
#include "port_sdcard.h"
#include "port_shtc3.h"
#include "voice_ui.h"

#include <algorithm>
#include <cstdlib>
#include <string>
#include <vector>

#define TAG "voice_app"

namespace {

constexpr EventBits_t BIT_BOOT_DOWN = 0x01;
constexpr EventBits_t BIT_BOOT_UP = 0x02;
constexpr EventBits_t BIT_PWR_LONG = 0x04;
constexpr EventBits_t BIT_TOUCH_ACTION = 0x08;

I2cMasterBus *i2c_bus = nullptr;
I2cFt6336Dev *ft6336_dev = nullptr;
pcf85063a_dev_t pcf85063 = {};
bool rtc_ready = false;
bool sd_ready = false;

Button *boot_button = nullptr;
Button *power_button = nullptr;
EventGroupHandle_t app_events = nullptr;
QueueHandle_t gpio_evt_queue = nullptr;
QueueHandle_t ui_action_queue = nullptr;

VoiceUi voice_ui;
MemoStorage memo_storage;
MemoRecorder memo_recorder;
MemoPlayer memo_player;
WavFormat wav_format;
std::vector<MemoMetadata> memos;

VoiceAppState app_state = VoiceAppState::List;
uint32_t current_page = 0;
uint32_t selected_index = 0;
uint32_t active_record_sequence = 0;
uint32_t last_record_elapsed = UINT32_MAX;
uint32_t last_play_elapsed = UINT32_MAX;

uint8_t BatteryPercent() {
    return Get_Batterylevel();
}

bool ReadRtcHourMinute(uint8_t *hour, uint8_t *minute) {
    if (!rtc_ready || hour == nullptr || minute == nullptr) {
        return false;
    }

    pcf85063a_datetime_t current_time = {};
    if (pcf85063a_get_time_date(&pcf85063, &current_time) != ESP_OK) {
        return false;
    }
    if (current_time.hour > 23 || current_time.min > 59) {
        return false;
    }

    *hour = current_time.hour;
    *minute = current_time.min;
    return true;
}

void RenderList(const char *message = nullptr) {
    current_page = ClampPage(current_page, CalculatePageCount(memos.size(), VoiceUi::kRowsPerPage));
    if (Lvgl_lock(-1)) {
        voice_ui.ShowList(memos, current_page, BatteryPercent(), sd_ready, message);
        Lvgl_unlock();
    }
}

void RenderRecording(uint32_t elapsed_seconds) {
    if (Lvgl_lock(-1)) {
        voice_ui.ShowRecording(elapsed_seconds);
        Lvgl_unlock();
    }
}

void RenderDetail(bool playing) {
    if (selected_index >= memos.size()) {
        app_state = VoiceAppState::List;
        RenderList();
        return;
    }

    uint32_t elapsed = playing ? memo_player.ElapsedSeconds() : 0;
    if (Lvgl_lock(-1)) {
        voice_ui.ShowDetail(memos[selected_index], playing, elapsed);
        Lvgl_unlock();
    }
}

void RenderError(const char *message) {
    if (Lvgl_lock(-1)) {
        voice_ui.ShowError(message);
        Lvgl_unlock();
    }
}

void LoadMemos() {
    memos.clear();
    if (!sd_ready) {
        current_page = 0;
        return;
    }

    if (memo_storage.Init() != ESP_OK) {
        sd_ready = false;
        current_page = 0;
        return;
    }

    esp_err_t ret = memo_storage.Scan(memos, wav_format);
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "Memo scan failed: %s", esp_err_to_name(ret));
    }
    current_page = ClampPage(current_page, CalculatePageCount(memos.size(), VoiceUi::kRowsPerPage));
}

void StartRecording() {
    if (memo_recorder.IsRecording() || memo_player.IsPlaying()) {
        return;
    }

    if (!sd_ready) {
        app_state = VoiceAppState::List;
        RenderList("NO SD CARD");
        return;
    }

    uint8_t hour = 0;
    uint8_t minute = 0;
    bool has_time = ReadRtcHourMinute(&hour, &minute);
    active_record_sequence = memo_storage.NextSequence(memos);
    std::string path = memo_storage.BuildPathForSequence(active_record_sequence, has_time, hour, minute);

    esp_err_t ret = memo_recorder.Start(path.c_str(), wav_format);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to start recording: %s", esp_err_to_name(ret));
        app_state = VoiceAppState::List;
        RenderList("SAVE FAILED");
        return;
    }

    app_state = VoiceAppState::Recording;
    last_record_elapsed = UINT32_MAX;
    RenderRecording(0);
}

void StopRecording() {
    if (!memo_recorder.IsRecording()) {
        return;
    }

    MemoMetadata saved = {};
    esp_err_t ret = memo_recorder.Stop(&saved);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to stop recording: %s", esp_err_to_name(ret));
        LoadMemos();
        app_state = VoiceAppState::List;
        RenderList("SAVE FAILED");
        return;
    }

    LoadMemos();
    app_state = VoiceAppState::List;
    selected_index = 0;
    RenderList("Saved");
}

void TogglePlayback() {
    if (app_state == VoiceAppState::Playing) {
        memo_player.Stop();
        app_state = VoiceAppState::Detail;
        RenderDetail(false);
        return;
    }

    if (selected_index >= memos.size()) {
        app_state = VoiceAppState::List;
        RenderList();
        return;
    }

    esp_err_t ret = memo_player.Start(memos[selected_index].path.c_str(), wav_format);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to play memo: %s", esp_err_to_name(ret));
        app_state = VoiceAppState::Detail;
        RenderError("PLAY FAILED");
        vTaskDelay(pdMS_TO_TICKS(1200));
        RenderDetail(false);
        return;
    }

    app_state = VoiceAppState::Playing;
    last_play_elapsed = UINT32_MAX;
    RenderDetail(true);
}

void HandleUiAction(const VoiceUiAction &action) {
    switch (action.type) {
    case VoiceUiActionType::NextPage: {
        uint32_t page_count = CalculatePageCount(memos.size(), VoiceUi::kRowsPerPage);
        if (current_page + 1 < page_count) {
            current_page++;
            RenderList();
        }
        break;
    }
    case VoiceUiActionType::PreviousPage:
        if (current_page > 0) {
            current_page--;
            RenderList();
        }
        break;
    case VoiceUiActionType::SelectRow: {
        uint32_t index = current_page * VoiceUi::kRowsPerPage + action.row;
        if (index < memos.size()) {
            selected_index = index;
            app_state = VoiceAppState::Detail;
            RenderDetail(false);
        }
        break;
    }
    case VoiceUiActionType::PlayStop:
        TogglePlayback();
        break;
    case VoiceUiActionType::Back:
        if (memo_player.IsPlaying()) {
            memo_player.Stop();
        }
        app_state = VoiceAppState::List;
        RenderList();
        break;
    case VoiceUiActionType::None:
    default:
        break;
    }
}

void Shutdown() {
    if (memo_recorder.IsRecording()) {
        StopRecording();
    }
    if (memo_player.IsPlaying()) {
        memo_player.Stop();
    }

    app_state = VoiceAppState::Shutdown;
    const MemoMetadata *last_memo = memos.empty() ? nullptr : &memos.back();
    if (Lvgl_lock(-1)) {
        voice_ui.ShowShutdown(memos.size(), BatteryPercent(), last_memo);
        Lvgl_unlock();
    }

    vTaskDelay(pdMS_TO_TICKS(3000));
    BoardPower_VBAT_OFF();
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
}

void App_LoopTask(void *arg) {
    (void)arg;
    LoadMemos();
    RenderList(sd_ready ? nullptr : "NO SD CARD");

    for (;;) {
        EventBits_t events = xEventGroupWaitBits(
            app_events,
            BIT_BOOT_DOWN | BIT_BOOT_UP | BIT_PWR_LONG | BIT_TOUCH_ACTION,
            pdTRUE,
            pdFALSE,
            pdMS_TO_TICKS(50));

        if (events & BIT_PWR_LONG) {
            Shutdown();
        }
        if (events & BIT_BOOT_DOWN) {
            StartRecording();
        }
        if (events & BIT_BOOT_UP) {
            StopRecording();
        }
        if (events & BIT_TOUCH_ACTION) {
            VoiceUiAction action;
            while (xQueueReceive(ui_action_queue, &action, 0) == pdTRUE) {
                if (!memo_recorder.IsRecording()) {
                    HandleUiAction(action);
                }
            }
        }

        if (memo_recorder.IsRecording()) {
            esp_err_t ret = memo_recorder.CaptureChunk();
            if (ret != ESP_OK) {
                ESP_LOGE(TAG, "Recording failed: %s", esp_err_to_name(ret));
                memo_recorder.Abort();
                LoadMemos();
                app_state = VoiceAppState::List;
                RenderList("SAVE FAILED");
            } else {
                uint32_t elapsed = memo_recorder.ElapsedSeconds();
                if (elapsed != last_record_elapsed) {
                    last_record_elapsed = elapsed;
                    RenderRecording(elapsed);
                }
            }
        }

        if (memo_player.IsPlaying()) {
            esp_err_t ret = memo_player.PlayChunk();
            if (ret != ESP_OK) {
                ESP_LOGE(TAG, "Playback failed: %s", esp_err_to_name(ret));
                memo_player.Stop();
                app_state = VoiceAppState::Detail;
                RenderDetail(false);
            } else if (memo_player.IsPlaying()) {
                uint32_t elapsed = memo_player.ElapsedSeconds();
                if (elapsed != last_play_elapsed) {
                    last_play_elapsed = elapsed;
                    RenderDetail(true);
                }
            } else {
                app_state = VoiceAppState::Detail;
                RenderDetail(false);
            }
        }
    }
}

static void IRAM_ATTR gpio_isr_handler(void *arg) {
    uint32_t gpio_num = reinterpret_cast<uint32_t>(arg);
    xQueueSendFromISR(gpio_evt_queue, &gpio_num, nullptr);
}

void Touch_ISR_GPIO_Init() {
    gpio_evt_queue = xQueueCreate(3, sizeof(uint32_t));

    gpio_config_t io_conf = {};
    io_conf.intr_type = GPIO_INTR_NEGEDGE;
    io_conf.pin_bit_mask = (1ULL << EPD_TP_INT_PIN);
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_conf.pull_up_en = GPIO_PULLUP_ENABLE;
    gpio_config(&io_conf);
    gpio_install_isr_service(0);
    gpio_isr_handler_add(EPD_TP_INT_PIN, gpio_isr_handler, reinterpret_cast<void *>(EPD_TP_INT_PIN));
}

void Touch_LoopTask(void *arg) {
    (void)arg;
    uint32_t io_num = 0;

    for (;;) {
        if (xQueueReceive(gpio_evt_queue, &io_num, portMAX_DELAY) != pdTRUE) {
            continue;
        }

        uint16_t start_x = 0;
        uint16_t start_y = 0;
        if (!ft6336_dev->GetTouchPoint(&start_x, &start_y)) {
            continue;
        }

        vTaskDelay(pdMS_TO_TICKS(180));

        uint16_t end_x = start_x;
        uint16_t end_y = start_y;
        ft6336_dev->GetTouchPoint(&end_x, &end_y);

        VoiceUiAction action = voice_ui.HitTestSwipe(start_y, end_y, app_state);
        if (action.type == VoiceUiActionType::None) {
            action = voice_ui.HitTestTap(start_x, start_y, app_state);
        }

        if (action.type != VoiceUiActionType::None) {
            xQueueSend(ui_action_queue, &action, 0);
            xEventGroupSetBits(app_events, BIT_TOUCH_ACTION);
        }
    }
}

} // namespace

void UserApp_Init() {
    BoardPower_Init();
    BoardPower_EPD_ON();
    BoardPower_Audio_ON();
    BoardPower_VBAT_ON();

    app_events = xEventGroupCreate();
    ui_action_queue = xQueueCreate(5, sizeof(VoiceUiAction));

    i2c_bus = I2cMasterBus::requestInstance(ESP32_I2C_SCL_PIN, ESP32_I2C_SDA_PIN, ESP32_I2C_DEV_NUM);
    assert(i2c_bus);

    ft6336_dev = I2cFt6336Dev::requestInstance(i2c_bus->Get_I2cBusHandle(), I2C_FT6336_DEV_Address, EPD_WIDTH, EPD_HEIGHT);
    assert(ft6336_dev);
    ft6336_dev->Ft6336_Reset(EPD_TP_RST_PIN);

    esp_err_t rtc_ret = pcf85063a_init(&pcf85063, i2c_bus->Get_I2cBusHandle(), PCF85063A_ADDRESS);
    rtc_ready = rtc_ret == ESP_OK;
    if (!rtc_ready) {
        ESP_LOGW(TAG, "PCF85063A init failed: %s", esp_err_to_name(rtc_ret));
    }

    boot_button = new Button(BOOT_BUTTON_PIN);
    power_button = new Button(PWR_BUTTON_PIN);
    while (gpio_get_level(PWR_BUTTON_PIN) == 0) {
        vTaskDelay(pdMS_TO_TICKS(100));
    }
    InitializeButtons();

    sd_ready = Sdcard_Init();
    BoardAdc_Init();
    Shtc3_Init(i2c_bus);
    Touch_ISR_GPIO_Init();
    Codec_StartInit();
}

void UserUi_Init() {
    voice_ui.Init();
}

void UserApp_Start_Init() {
    xTaskCreatePinnedToCore(App_LoopTask, "VoiceApp", 8 * 1024, nullptr, 3, nullptr, 1);
    xTaskCreatePinnedToCore(Touch_LoopTask, "VoiceTouch", 4 * 1024, nullptr, 2, nullptr, 1);
}
