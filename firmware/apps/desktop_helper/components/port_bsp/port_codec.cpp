#include <stdio.h>
#include <freertos/FreeRTOS.h>
#include <esp_heap_caps.h>
#include "port_codec.h"
#include "codec_board.h"
#include "codec_init.h"
#include "esp_codec_dev.h"


static esp_codec_dev_handle_t playback = NULL;
static esp_codec_dev_handle_t record = NULL;
static bool i2s_initialized = false;

extern const uint8_t music_pcm_start[] asm("_binary_canon_pcm_start");
extern const uint8_t music_pcm_end[]   asm("_binary_canon_pcm_end");


esp_err_t Codec_Init(const char *strName) {
  	if(i2s_initialized)
    return ESP_OK;
    set_codec_board_type(strName);
  	codec_init_cfg_t codec_cfg = {};
	codec_cfg.in_mode = CODEC_I2S_MODE_TDM;
	codec_cfg.out_mode = CODEC_I2S_MODE_TDM;
	codec_cfg.in_use_tdm = false;
	codec_cfg.reuse_dev = false;
  	ESP_ERROR_CHECK(init_codec(&codec_cfg));
    i2s_initialized = true;
	playback = get_playback_handle();
	record = get_record_handle();
    return ESP_OK;
}

esp_codec_dev_handle_t Codec_SpeakerInit(void) {
    ESP_ERROR_CHECK(Codec_Init("S3_ePaper_1_54"));
  	return get_playback_handle();
}

esp_codec_dev_handle_t Codec_MicrophoneInit(void) {
    ESP_ERROR_CHECK(Codec_Init("S3_ePaper_1_54"));
    return get_record_handle();
}

void Codec_StartInit() {
	Codec_Init("S3_ePaper_1_54");
	esp_codec_dev_sample_info_t fs = {};
    fs.sample_rate = 16000;        
    fs.channel = 2;
    fs.bits_per_sample = 16;
    ESP_ERROR_CHECK(esp_codec_dev_open(playback, &fs));          //打开播放
    ESP_ERROR_CHECK(esp_codec_dev_open(record, &fs));            //打开录音
    ESP_ERROR_CHECK(esp_codec_dev_set_out_vol(playback, 100));   //设置100声音大小
    ESP_ERROR_CHECK(esp_codec_dev_set_in_gain(record, 45.0));    //设置录音时的增益
}

esp_err_t Codec_PlaybackData(uint8_t *buffer,size_t bytes) {
	return esp_codec_dev_write(playback, buffer, bytes);
}

esp_err_t Codec_RecordData(uint8_t *buffer,size_t bytes) {
	return esp_codec_dev_read(record, buffer, bytes);
}
