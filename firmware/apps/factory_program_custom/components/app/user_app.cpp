#include "user_app.h"

#include "audio_loop_policy.h"
#include <freertos/FreeRTOS.h>
#include <freertos/event_groups.h>
#include <freertos/queue.h>
#include <freertos/task.h>
#include <driver/gpio.h>
#include <driver/rtc_io.h>
#include <esp_err.h>
#include <esp_log.h>
#include <esp_sleep.h>
#include <esp_timer.h>
#include <nvs.h>

#include "button.h"
#include "epaper_config.h"
#include "memo_player.h"
#include "memo_favorites.h"
#include "memo_recorder.h"
#include "memo_storage.h"
#include "memo_utils.h"
#include "pcf85063a.h"
#include "power_sleep_policy.h"
#include "port_adc.h"
#include "port_codec.h"
#include "port_flash_storage.h"
#include "port_ft6336.h"
#include "port_i2c.h"
#include "port_lvgl.h"
#include "port_power.h"
#include "port_sdcard.h"
#include "port_shtc3.h"
#include "touch_gesture.h"
#include "ui_sfx.h"
#include "voice_ui.h"
#include "voice_settings.h"

#include <algorithm>
#include <cstdlib>
#include <string>
#include <vector>

#define TAG "voice_app"

namespace {

constexpr EventBits_t BIT_BOOT_DOWN = 0x01;
constexpr EventBits_t BIT_BOOT_UP = 0x02;
constexpr EventBits_t BIT_PWR_LONG = 0x04;
constexpr EventBits_t BIT_PWR_UP = 0x08;
constexpr EventBits_t BIT_TOUCH_ACTION = 0x10;
constexpr EventBits_t BIT_RECORD_ERROR = 0x20;
constexpr EventBits_t BIT_SAVE_DONE = 0x40;
constexpr EventBits_t BIT_SAVE_ERROR = 0x80;

constexpr EventBits_t BIT_REC_WORKER_START = 0x01;
constexpr EventBits_t BIT_REC_WORKER_STOP = 0x02;
constexpr EventBits_t BIT_REC_WORKER_STOPPED = 0x04;

constexpr EventBits_t BIT_SAVE_WORKER_START = 0x01;
constexpr EventBits_t BIT_SAVE_WORKER_DONE = 0x02;

I2cMasterBus *i2c_bus = nullptr;
I2cFt6336Dev *ft6336_dev = nullptr;
pcf85063a_dev_t pcf85063 = {};
bool rtc_ready = false;
bool sd_ready = false;
bool flash_ready = false;
bool power_button_released = false;

Button *boot_button = nullptr;
Button *power_button = nullptr;
EventGroupHandle_t app_events = nullptr;
EventGroupHandle_t recorder_events = nullptr;
EventGroupHandle_t save_events = nullptr;
QueueHandle_t gpio_evt_queue = nullptr;
QueueHandle_t ui_action_queue = nullptr;

VoiceUi voice_ui;
MemoStorage memo_storage;
MemoFavorites memo_favorites;
MemoRecorder memo_recorder;
MemoPlayer memo_player;
WavFormat wav_format;
VoiceSettings voice_settings;
std::vector<MemoMetadata> memos;

VoiceAppState app_state = VoiceAppState::List;
uint32_t current_page = 0;
uint32_t selected_index = 0;
uint32_t active_record_sequence = 0;
uint32_t last_record_elapsed = UINT32_MAX;
uint32_t last_play_elapsed = UINT32_MAX;
int64_t saving_started_us = 0;
uint32_t last_saving_frame = UINT32_MAX;
std::string pending_saved_path;
bool recording_worker_active = false;
esp_err_t recorder_error = ESP_OK;
esp_err_t save_error = ESP_OK;

bool StorageReady();

void ApplyRuntimeSettings() {
    SetUiSfxEnabled(voice_settings.sfx_enabled);
    SetUiSfxVolume(voice_settings.sfx_volume);
}

void LoadSettings() {
    nvs_handle_t handle = 0;
    esp_err_t ret = nvs_open("voice", NVS_READONLY, &handle);
    if (ret != ESP_OK) {
        ApplyRuntimeSettings();
        return;
    }

    uint8_t sound = voice_settings.sfx_enabled ? 1 : 0;
    uint8_t volume = static_cast<uint8_t>(voice_settings.sfx_volume);
    uint8_t record_beep = voice_settings.record_start_sfx_enabled ? 1 : 0;
    uint8_t record_mode = static_cast<uint8_t>(voice_settings.record_mode);
    nvs_get_u8(handle, "sound", &sound);
    nvs_get_u8(handle, "volume", &volume);
    nvs_get_u8(handle, "rec_beep", &record_beep);
    nvs_get_u8(handle, "rec_mode", &record_mode);
    nvs_close(handle);

    voice_settings.sfx_enabled = sound != 0;
    if (volume <= static_cast<uint8_t>(UiSfxVolume::High)) {
        voice_settings.sfx_volume = static_cast<UiSfxVolume>(volume);
    }
    voice_settings.record_start_sfx_enabled = record_beep != 0;
    if (record_mode <= static_cast<uint8_t>(VoiceRecordMode::Tap)) {
        voice_settings.record_mode = static_cast<VoiceRecordMode>(record_mode);
    }
    ApplyRuntimeSettings();
}

void SaveSettings() {
    nvs_handle_t handle = 0;
    esp_err_t ret = nvs_open("voice", NVS_READWRITE, &handle);
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "Open settings failed: %s", esp_err_to_name(ret));
        return;
    }
    nvs_set_u8(handle, "sound", voice_settings.sfx_enabled ? 1 : 0);
    nvs_set_u8(handle, "volume", static_cast<uint8_t>(voice_settings.sfx_volume));
    nvs_set_u8(handle, "rec_beep", voice_settings.record_start_sfx_enabled ? 1 : 0);
    nvs_set_u8(handle, "rec_mode", static_cast<uint8_t>(voice_settings.record_mode));
    nvs_commit(handle);
    nvs_close(handle);
    ApplyRuntimeSettings();
}

std::string FavoritesPath() {
    return memo_storage.BaseDir() + "/.memo_stars";
}

void LoadFavorites() {
    memo_favorites.Clear();
    if (!StorageReady()) {
        return;
    }

    FILE *file = fopen(FavoritesPath().c_str(), "rb");
    if (file == nullptr) {
        return;
    }
    std::string data;
    char buffer[128];
    size_t read = 0;
    while ((read = fread(buffer, 1, sizeof(buffer), file)) > 0) {
        data.append(buffer, read);
    }
    fclose(file);
    memo_favorites.Parse(data);
}

void SaveFavorites() {
    if (!StorageReady()) {
        return;
    }

    FILE *file = fopen(FavoritesPath().c_str(), "wb");
    if (file == nullptr) {
        ESP_LOGW(TAG, "Failed to save favorites");
        return;
    }
    std::string data = memo_favorites.Serialize();
    fwrite(data.data(), 1, data.size(), file);
    fclose(file);
}

uint8_t BatteryPercent() {
    return Get_Batterylevel();
}

bool StorageReady() {
    return sd_ready || flash_ready;
}

const char *StorageLabel() {
    if (sd_ready) {
        return "SD";
    }
    if (flash_ready) {
        return "FLASH";
    }
    return "NONE";
}

void ScanInitialPowerButtonRelease() {
    if (!power_button_released && gpio_get_level(PWR_BUTTON_PIN) == 1) {
        power_button_released = true;
    }
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
        voice_ui.ShowList(memos, current_page, BatteryPercent(), StorageReady(), StorageLabel(), message);
        Lvgl_unlock();
    }
}

void RenderRecording(uint32_t elapsed_seconds) {
    if (Lvgl_lock(-1)) {
        voice_ui.ShowRecording(elapsed_seconds);
        Lvgl_unlock();
    }
}

void RenderSaving(uint8_t frame) {
    if (Lvgl_lock(-1)) {
        voice_ui.ShowSaving(frame);
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

void RenderDeleteConfirm() {
    if (selected_index >= memos.size()) {
        app_state = VoiceAppState::List;
        RenderList();
        return;
    }

    if (Lvgl_lock(-1)) {
        voice_ui.ShowDeleteConfirm(memos[selected_index]);
        Lvgl_unlock();
    }
}

void RenderSettings() {
    if (Lvgl_lock(-1)) {
        voice_ui.ShowSettings(voice_settings, memos.size());
        Lvgl_unlock();
    }
}

void RenderClearAllConfirm() {
    if (Lvgl_lock(-1)) {
        voice_ui.ShowClearAllConfirm(memos.size());
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
    if (!StorageReady()) {
        current_page = 0;
        return;
    }

    if (memo_storage.Init() != ESP_OK) {
        sd_ready = false;
        flash_ready = false;
        current_page = 0;
        return;
    }

    esp_err_t ret = memo_storage.Scan(memos, wav_format);
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "Memo scan failed: %s", esp_err_to_name(ret));
    }
    for (MemoMetadata &memo : memos) {
        memo_favorites.ApplyTo(memo);
    }
    current_page = ClampPage(current_page, CalculatePageCount(memos.size(), VoiceUi::kRowsPerPage));
}

void DeleteSelectedMemo() {
    if (selected_index >= memos.size()) {
        app_state = VoiceAppState::List;
        RenderList();
        return;
    }

    if (memo_player.IsPlaying()) {
        memo_player.Stop();
    }

    std::string path = memos[selected_index].path;
    esp_err_t ret = memo_storage.RemoveFile(path.c_str());
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to delete memo %s: %s", path.c_str(), esp_err_to_name(ret));
        PlayUiSfx(UiSfxEvent::Error);
        RenderError("DELETE FAILED");
        vTaskDelay(pdMS_TO_TICKS(1200));
        int32_t still_exists = FindMemoIndexByPath(memos, path);
        if (still_exists >= 0) {
            selected_index = static_cast<uint32_t>(still_exists);
            app_state = VoiceAppState::Detail;
            RenderDetail(false);
        } else {
            app_state = VoiceAppState::List;
            RenderList();
        }
        return;
    }

    ESP_LOGI(TAG, "Deleted memo %s", path.c_str());
    memo_favorites.SetStarred(path, false);
    SaveFavorites();
    uint32_t previous_page = current_page;
    LoadMemos();
    current_page = ClampPage(previous_page, CalculatePageCount(memos.size(), VoiceUi::kRowsPerPage));
    selected_index = 0;
    app_state = VoiceAppState::List;
    RenderList("Deleted");
}

void DeleteAllMemos() {
    if (memo_player.IsPlaying()) {
        memo_player.Stop();
    }

    bool failed = false;
    for (const MemoMetadata &memo : memos) {
        if (memo.starred) {
            continue;
        }
        esp_err_t ret = memo_storage.RemoveFile(memo.path.c_str());
        if (ret != ESP_OK && ret != ESP_ERR_NOT_FOUND) {
            ESP_LOGE(TAG, "Failed to delete memo %s: %s", memo.path.c_str(), esp_err_to_name(ret));
            failed = true;
        } else {
            memo_favorites.SetStarred(memo.path, false);
        }
    }
    SaveFavorites();

    LoadMemos();
    selected_index = 0;
    current_page = 0;
    if (failed) {
        PlayUiSfx(UiSfxEvent::Error);
        app_state = VoiceAppState::Settings;
        RenderSettings();
    } else {
        PlayUiSfx(UiSfxEvent::Confirm);
        app_state = VoiceAppState::Settings;
        RenderSettings();
    }
}

void ToggleSelectedStar() {
    if (selected_index >= memos.size()) {
        app_state = VoiceAppState::List;
        RenderList();
        return;
    }

    std::string path = memos[selected_index].path;
    memo_favorites.Toggle(path);
    SaveFavorites();
    LoadMemos();
    int32_t index = FindMemoIndexByPath(memos, path);
    selected_index = index >= 0 ? static_cast<uint32_t>(index) : 0;
    PlayUiSfx(UiSfxEvent::Touch);
    RenderDetail(false);
}

void StartRecording() {
    if (memo_recorder.IsRecording() || memo_player.IsPlaying()) {
        return;
    }

    if (!StorageReady()) {
        PlayUiSfx(UiSfxEvent::Error);
        app_state = VoiceAppState::List;
        RenderList("NO STORAGE");
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
        PlayUiSfx(UiSfxEvent::Error);
        app_state = VoiceAppState::List;
        RenderList("SAVE FAILED");
        return;
    }

    if (voice_settings.record_start_sfx_enabled) {
        PlayUiSfx(UiSfxEvent::RecordStart);
    }
    app_state = VoiceAppState::Recording;
    last_record_elapsed = UINT32_MAX;
    recording_worker_active = true;
    xEventGroupSetBits(recorder_events, BIT_REC_WORKER_START);
    RenderRecording(0);
}

void StopRecording() {
    if (!memo_recorder.IsRecording()) {
        return;
    }

    recording_worker_active = false;
    xEventGroupSetBits(recorder_events, BIT_REC_WORKER_STOP);
    xEventGroupWaitBits(
        recorder_events,
        BIT_REC_WORKER_STOPPED,
        pdTRUE,
        pdFALSE,
        portMAX_DELAY);
    PlayUiSfx(UiSfxEvent::RecordStop);

    app_state = VoiceAppState::Saving;
    saving_started_us = esp_timer_get_time();
    last_saving_frame = UINT32_MAX;
    pending_saved_path = memo_recorder.Path();
    ESP_LOGI(TAG, "Saving memo start path=%s bytes=%u", pending_saved_path.c_str(), static_cast<unsigned>(memo_recorder.DataBytes()));
    RenderSaving(0);
    xEventGroupClearBits(save_events, BIT_SAVE_WORKER_DONE);
    xEventGroupSetBits(save_events, BIT_SAVE_WORKER_START);
}

void TogglePlayback() {
    if (app_state == VoiceAppState::Playing) {
        PlayUiSfx(UiSfxEvent::Touch);
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

    PlayUiSfx(UiSfxEvent::Touch);
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
            PlayUiSfx(UiSfxEvent::Navigate);
            current_page++;
            RenderList();
        }
        break;
    }
    case VoiceUiActionType::PreviousPage:
        if (current_page > 0) {
            PlayUiSfx(UiSfxEvent::Navigate);
            current_page--;
            RenderList();
        }
        break;
    case VoiceUiActionType::SelectRow: {
        if (app_state == VoiceAppState::Settings) {
            VoiceSettingsItem item = VoiceSettingsItemForRow(action.row);
            if (item == VoiceSettingsItem::ClearAll) {
                PlayUiSfx(UiSfxEvent::Delete);
                app_state = VoiceAppState::ClearAllConfirm;
                RenderClearAllConfirm();
            } else {
                ApplyVoiceSettingsItem(&voice_settings, item);
                SaveSettings();
                PlayUiSfx(UiSfxEvent::Touch);
                RenderSettings();
            }
            break;
        }

        uint32_t index = current_page * VoiceUi::kRowsPerPage + action.row;
        if (index < memos.size()) {
            PlayUiSfx(UiSfxEvent::Touch);
            selected_index = index;
            app_state = VoiceAppState::Detail;
            RenderDetail(false);
        }
        break;
    }
    case VoiceUiActionType::PlayStop:
        TogglePlayback();
        break;
    case VoiceUiActionType::Delete:
        if (selected_index < memos.size()) {
            PlayUiSfx(UiSfxEvent::Delete);
            if (memo_player.IsPlaying()) {
                memo_player.Stop();
            }
            app_state = VoiceAppState::DeleteConfirm;
            RenderDeleteConfirm();
        } else {
            app_state = VoiceAppState::List;
            RenderList();
        }
        break;
    case VoiceUiActionType::Star:
        if (app_state == VoiceAppState::Detail || app_state == VoiceAppState::Playing) {
            if (memo_player.IsPlaying()) {
                memo_player.Stop();
                app_state = VoiceAppState::Detail;
            }
            ToggleSelectedStar();
        }
        break;
    case VoiceUiActionType::ConfirmDelete:
        if (app_state == VoiceAppState::DeleteConfirm) {
            PlayUiSfx(UiSfxEvent::Confirm);
            DeleteSelectedMemo();
        }
        break;
    case VoiceUiActionType::ConfirmClearAll:
        if (app_state == VoiceAppState::ClearAllConfirm) {
            DeleteAllMemos();
        }
        break;
    case VoiceUiActionType::Cancel:
        if (app_state == VoiceAppState::DeleteConfirm) {
            PlayUiSfx(UiSfxEvent::Cancel);
            app_state = VoiceAppState::Detail;
            RenderDetail(false);
        } else if (app_state == VoiceAppState::ClearAllConfirm) {
            PlayUiSfx(UiSfxEvent::Cancel);
            app_state = VoiceAppState::Settings;
            RenderSettings();
        }
        break;
    case VoiceUiActionType::Back:
        if (app_state == VoiceAppState::DeleteConfirm) {
            PlayUiSfx(UiSfxEvent::Cancel);
            app_state = VoiceAppState::Detail;
            RenderDetail(false);
            break;
        }
        if (app_state == VoiceAppState::ClearAllConfirm) {
            PlayUiSfx(UiSfxEvent::Cancel);
            app_state = VoiceAppState::Settings;
            RenderSettings();
            break;
        }
        PlayUiSfx(UiSfxEvent::Navigate);
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
        xEventGroupWaitBits(
            save_events,
            BIT_SAVE_WORKER_DONE,
            pdTRUE,
            pdFALSE,
            portMAX_DELAY);
        LoadMemos();
        pending_saved_path.clear();
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

void ToggleSettingsMenu() {
    if (recording_worker_active || app_state == VoiceAppState::Saving || memo_recorder.IsRecording()) {
        return;
    }

    if (memo_player.IsPlaying()) {
        memo_player.Stop();
    }

    if (app_state == VoiceAppState::Settings) {
        PlayUiSfx(UiSfxEvent::Navigate);
        app_state = VoiceAppState::List;
        RenderList();
        return;
    }

    if (app_state == VoiceAppState::ClearAllConfirm) {
        PlayUiSfx(UiSfxEvent::Cancel);
        app_state = VoiceAppState::Settings;
        RenderSettings();
        return;
    }

    PlayUiSfx(UiSfxEvent::Navigate);
    LoadMemos();
    app_state = VoiceAppState::Settings;
    RenderSettings();
}

void App_LoopTask(void *arg) {
    (void)arg;
    LoadMemos();
    RenderList(StorageReady() ? nullptr : "NO STORAGE");

    for (;;) {
        EventBits_t events = xEventGroupWaitBits(
            app_events,
            BIT_BOOT_DOWN | BIT_BOOT_UP | BIT_PWR_LONG | BIT_PWR_UP | BIT_TOUCH_ACTION | BIT_RECORD_ERROR | BIT_SAVE_DONE | BIT_SAVE_ERROR,
            pdTRUE,
            pdFALSE,
            pdMS_TO_TICKS(AppEventWaitTimeoutMs(memo_recorder.IsRecording(), memo_player.IsPlaying())));

        if (events & BIT_PWR_LONG) {
            if (app_state == VoiceAppState::Saving) {
                ESP_LOGI(TAG, "Ignore PWR long press while saving");
            } else if (power_button_released) {
                PlayUiSfx(UiSfxEvent::Power);
                power_button_released = false;
                Shutdown();
            } else {
                ESP_LOGI(TAG, "Ignore PWR long press before first release");
            }
        }
        if (events & BIT_PWR_UP) {
            if (power_button_released) {
                ToggleSettingsMenu();
            }
            power_button_released = true;
        }
        if (events & BIT_BOOT_DOWN) {
            if (app_state != VoiceAppState::Saving && voice_settings.record_mode == VoiceRecordMode::Hold) {
                StartRecording();
            }
        }
        if (events & BIT_BOOT_UP) {
            if (app_state != VoiceAppState::Saving) {
                if (voice_settings.record_mode == VoiceRecordMode::Hold) {
                    StopRecording();
                } else if (recording_worker_active) {
                    StopRecording();
                } else {
                    StartRecording();
                }
            }
        }
        if (events & BIT_TOUCH_ACTION) {
            VoiceUiAction action;
            while (xQueueReceive(ui_action_queue, &action, 0) == pdTRUE) {
                if (!recording_worker_active && app_state != VoiceAppState::Saving) {
                    HandleUiAction(action);
                }
            }
        }
        if (events & BIT_RECORD_ERROR) {
            ESP_LOGE(TAG, "Recording failed: %s", esp_err_to_name(recorder_error));
            PlayUiSfx(UiSfxEvent::Error);
            memo_recorder.Abort();
            LoadMemos();
            app_state = VoiceAppState::List;
            RenderList("SAVE FAILED");
        }
        if (events & BIT_SAVE_DONE) {
            PlayUiSfx(UiSfxEvent::SaveDone);
            LoadMemos();
            int32_t saved_index = FindMemoIndexByPath(memos, pending_saved_path);
            pending_saved_path.clear();
            if (saved_index >= 0) {
                selected_index = static_cast<uint32_t>(saved_index);
                app_state = VoiceAppState::Detail;
                RenderDetail(false);
            } else {
                app_state = VoiceAppState::List;
                selected_index = 0;
                RenderList("Saved");
            }
        }
        if (events & BIT_SAVE_ERROR) {
            ESP_LOGE(TAG, "Failed to save memo: %s", esp_err_to_name(save_error));
            PlayUiSfx(UiSfxEvent::Error);
            LoadMemos();
            pending_saved_path.clear();
            app_state = VoiceAppState::List;
            RenderList("SAVE FAILED");
        }

        if (recording_worker_active) {
            uint32_t elapsed = memo_recorder.ElapsedSeconds();
            if (ShouldRefreshAudioProgress(elapsed, last_record_elapsed)) {
                last_record_elapsed = elapsed;
                RenderRecording(elapsed);
            }
        }

        if (app_state == VoiceAppState::Saving) {
            uint32_t elapsed = static_cast<uint32_t>((esp_timer_get_time() - saving_started_us) / 1000000);
            uint8_t frame = SavingAnimationFrame(elapsed);
            if (frame != last_saving_frame) {
                last_saving_frame = frame;
                RenderSaving(frame);
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
                if (ShouldRefreshAudioProgress(elapsed, last_play_elapsed)) {
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

void Save_LoopTask(void *arg) {
    (void)arg;

    for (;;) {
        xEventGroupWaitBits(
            save_events,
            BIT_SAVE_WORKER_START,
            pdTRUE,
            pdFALSE,
            portMAX_DELAY);

        MemoMetadata saved = {};
        int64_t started_us = esp_timer_get_time();
        save_error = memo_recorder.Stop(&saved);
        int64_t elapsed_ms = (esp_timer_get_time() - started_us) / 1000;
        ESP_LOGI(TAG, "Saving memo done ret=%s elapsed_ms=%lld path=%s bytes=%u",
                 esp_err_to_name(save_error),
                 static_cast<long long>(elapsed_ms),
                 saved.path.c_str(),
                 static_cast<unsigned>(saved.data_bytes));
        xEventGroupSetBits(save_events, BIT_SAVE_WORKER_DONE);
        xEventGroupSetBits(app_events, save_error == ESP_OK ? BIT_SAVE_DONE : BIT_SAVE_ERROR);
    }
}

void Recorder_LoopTask(void *arg) {
    (void)arg;

    for (;;) {
        xEventGroupWaitBits(
            recorder_events,
            BIT_REC_WORKER_START,
            pdTRUE,
            pdFALSE,
            portMAX_DELAY);

        xEventGroupClearBits(recorder_events, BIT_REC_WORKER_STOPPED);
        recorder_error = ESP_OK;

        while (memo_recorder.IsRecording()) {
            EventBits_t bits = xEventGroupGetBits(recorder_events);
            if (bits & BIT_REC_WORKER_STOP) {
                xEventGroupClearBits(recorder_events, BIT_REC_WORKER_STOP);
                break;
            }

            esp_err_t ret = memo_recorder.CaptureChunk();
            if (ret != ESP_OK) {
                recorder_error = ret;
                recording_worker_active = false;
                xEventGroupSetBits(app_events, BIT_RECORD_ERROR);
                break;
            }
        }

        xEventGroupSetBits(recorder_events, BIT_REC_WORKER_STOPPED);
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

        uint16_t end_x = start_x;
        uint16_t end_y = start_y;
        uint16_t max_delta_x = 0;
        uint16_t max_delta_y = 0;

        for (;;) {
            vTaskDelay(pdMS_TO_TICKS(25));

            uint16_t sample_x = end_x;
            uint16_t sample_y = end_y;
            if (!ft6336_dev->GetTouchPoint(&sample_x, &sample_y)) {
                break;
            }

            end_x = sample_x;
            end_y = sample_y;
            max_delta_x = std::max<uint16_t>(max_delta_x, static_cast<uint16_t>(std::abs(static_cast<int>(sample_x) - static_cast<int>(start_x))));
            max_delta_y = std::max<uint16_t>(max_delta_y, static_cast<uint16_t>(std::abs(static_cast<int>(sample_y) - static_cast<int>(start_y))));
        }

        VoiceUiAction action = ClassifyTouchGesture(start_x, start_y, end_x, end_y, max_delta_x, max_delta_y, app_state);
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
    LoadSettings();

    app_events = xEventGroupCreate();
    recorder_events = xEventGroupCreate();
    save_events = xEventGroupCreate();
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
    InitializeButtons();
    ScanInitialPowerButtonRelease();

    sd_ready = Sdcard_Init();
    if (sd_ready) {
        memo_storage.SetBaseDir("/sdcard/memos");
        ESP_LOGI(TAG, "Using SD card memo storage");
    } else {
        flash_ready = FlashStorage_Init();
        if (flash_ready) {
            memo_storage.SetBaseDir("/flash/memos");
            ESP_LOGI(TAG, "Using flash memo storage");
        } else {
            ESP_LOGE(TAG, "No memo storage available");
        }
    }
    if (StorageReady()) {
        memo_storage.Init();
        LoadFavorites();
    }
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
    xTaskCreatePinnedToCore(Recorder_LoopTask, "VoiceRecorder", 6 * 1024, nullptr, 5, nullptr, 1);
    xTaskCreatePinnedToCore(Save_LoopTask, "VoiceSave", 6 * 1024, nullptr, 2, nullptr, 1);
    xTaskCreatePinnedToCore(Touch_LoopTask, "VoiceTouch", 4 * 1024, nullptr, 2, nullptr, 1);
}
