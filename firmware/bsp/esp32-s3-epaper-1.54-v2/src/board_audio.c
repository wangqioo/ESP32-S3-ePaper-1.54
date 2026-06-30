#include "board_audio.h"

#include <stdbool.h>

#include "board_power.h"
#include "codec_board.h"
#include "codec_init.h"
#include "esp_check.h"
#include "esp_codec_dev.h"
#include "esp_log.h"

static const char *TAG = "board_audio";
static esp_codec_dev_handle_t s_playback = NULL;
static esp_codec_dev_handle_t s_record = NULL;
static bool s_playback_open = false;
static bool s_record_open = false;
static bool s_audio_open = false;

static void board_audio_close_streams(void)
{
    if (s_record_open) {
        esp_codec_dev_close(s_record);
    }
    if (s_playback_open) {
        esp_codec_dev_close(s_playback);
    }
    s_record_open = false;
    s_playback_open = false;
    s_audio_open = false;
}

static void board_audio_cleanup_init_failure(void)
{
    board_audio_close_streams();
    deinit_codec();
    s_playback = NULL;
    s_record = NULL;
    s_record_open = false;
    s_playback_open = false;
    s_audio_open = false;
    board_power_audio_off();
}

static esp_err_t board_audio_open_streams(void)
{
    if (s_audio_open) {
        return ESP_OK;
    }

    esp_codec_dev_sample_info_t sample_info = {
        .sample_rate = 16000,
        .channel = 2,
        .bits_per_sample = 16,
    };

    ESP_RETURN_ON_FALSE(esp_codec_dev_open(s_playback, &sample_info) == ESP_CODEC_DEV_OK,
                        ESP_FAIL,
                        TAG,
                        "open playback failed");
    s_playback_open = true;
    if (esp_codec_dev_open(s_record, &sample_info) != ESP_CODEC_DEV_OK) {
        ESP_LOGE(TAG, "open record failed");
        return ESP_FAIL;
    }

    s_record_open = true;
    s_audio_open = true;
    return ESP_OK;
}

esp_err_t board_audio_init(void)
{
    if (s_playback != NULL && s_record != NULL) {
        return board_audio_open_streams();
    }

    ESP_RETURN_ON_ERROR(board_power_init(), TAG, "power init failed");
    ESP_RETURN_ON_ERROR(board_power_audio_on(), TAG, "audio power on failed");

    set_codec_board_type("S3_ePaper_1_54");

    codec_init_cfg_t codec_cfg = {
        .in_mode = CODEC_I2S_MODE_STD,
        .out_mode = CODEC_I2S_MODE_STD,
        .in_use_tdm = false,
        .reuse_dev = false,
    };

    if (init_codec(&codec_cfg) != 0) {
        board_audio_cleanup_init_failure();
        ESP_LOGE(TAG, "codec init failed");
        return ESP_FAIL;
    }

    s_playback = get_playback_handle();
    s_record = get_record_handle();
    if (s_playback == NULL) {
        board_audio_cleanup_init_failure();
        ESP_LOGE(TAG, "playback handle unavailable");
        return ESP_FAIL;
    }
    if (s_record == NULL) {
        board_audio_cleanup_init_failure();
        ESP_LOGE(TAG, "record handle unavailable");
        return ESP_FAIL;
    }

    if (esp_codec_dev_set_out_vol(s_playback, 100.0f) != ESP_CODEC_DEV_OK) {
        board_audio_cleanup_init_failure();
        ESP_LOGE(TAG, "set volume failed");
        return ESP_FAIL;
    }
    if (esp_codec_dev_set_in_gain(s_record, 45.0f) != ESP_CODEC_DEV_OK) {
        board_audio_cleanup_init_failure();
        ESP_LOGE(TAG, "set gain failed");
        return ESP_FAIL;
    }

    esp_err_t err = board_audio_open_streams();
    if (err != ESP_OK) {
        board_audio_cleanup_init_failure();
    }
    return err;
}

void board_audio_set_volume(uint8_t volume)
{
    if (s_playback == NULL) {
        return;
    }

    esp_codec_dev_set_out_vol(s_playback, (float)volume);
}

esp_err_t board_audio_play_write(const void *data, uint32_t len)
{
    ESP_RETURN_ON_FALSE(data != NULL || len == 0, ESP_ERR_INVALID_ARG, TAG, "data is null");
    ESP_RETURN_ON_ERROR(board_audio_init(), TAG, "audio init failed");
    if (len == 0) {
        return ESP_OK;
    }

    return esp_codec_dev_write(s_playback, (void *)data, len) == ESP_CODEC_DEV_OK ? ESP_OK : ESP_FAIL;
}

esp_err_t board_audio_record_read(void *data, uint32_t len)
{
    ESP_RETURN_ON_FALSE(data != NULL || len == 0, ESP_ERR_INVALID_ARG, TAG, "data is null");
    ESP_RETURN_ON_ERROR(board_audio_init(), TAG, "audio init failed");
    if (len == 0) {
        return ESP_OK;
    }

    return esp_codec_dev_read(s_record, data, len) == ESP_CODEC_DEV_OK ? ESP_OK : ESP_FAIL;
}
