#ifndef PORT_CODEC_H
#define PORT_CODEC_H

#include <esp_err.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

esp_err_t Codec_Init(const char *strName);
void Codec_StartInit();

esp_err_t Codec_PlaybackData(uint8_t *buffer,size_t bytes);
esp_err_t Codec_RecordData(uint8_t *buffer,size_t bytes);

#ifdef __cplusplus
}
#endif



#endif
