#include "board_audio.h"

#include <stdbool.h>

#include "board_power.h"
#include "codec_board.h"
#include "codec_init.h"
#include "esp_check.h"
#include "esp_codec_dev.h"

static const char *TAG = "board_audio";
static esp_codec_dev_handle_t s_playback = NULL;
static esp_codec_dev_handle_t s_record = NULL;
static bool s_audio_open = false;

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
    ESP_RETURN_ON_FALSE(esp_codec_dev_open(s_record, &sample_info) == ESP_CODEC_DEV_OK,
                        ESP_FAIL,
                        TAG,
                        "open record failed");

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

    ESP_RETURN_ON_FALSE(init_codec(&codec_cfg) == 0, ESP_FAIL, TAG, "codec init failed");

    s_playback = get_playback_handle();
    s_record = get_record_handle();
    ESP_RETURN_ON_FALSE(s_playback != NULL, ESP_FAIL, TAG, "playback handle unavailable");
    ESP_RETURN_ON_FALSE(s_record != NULL, ESP_FAIL, TAG, "record handle unavailable");

    ESP_RETURN_ON_FALSE(esp_codec_dev_set_out_vol(s_playback, 100.0f) == ESP_CODEC_DEV_OK,
                        ESP_FAIL,
                        TAG,
                        "set volume failed");
    ESP_RETURN_ON_FALSE(esp_codec_dev_set_in_gain(s_record, 45.0f) == ESP_CODEC_DEV_OK,
                        ESP_FAIL,
                        TAG,
                        "set gain failed");

    return board_audio_open_streams();
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
