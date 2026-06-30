#ifndef PORT_SDCARD_H
#define PORT_SDCARD_H

#include "driver/sdmmc_host.h"

extern sdmmc_card_t *card;
extern uint32_t sdcard_slot;

#ifdef __cplusplus
extern "C" {
#endif



bool Sdcard_Init(void);
esp_err_t Sdcard_ReadFile(const char *path,char *pxbuf,uint32_t *outLen);
esp_err_t Sdcard_WriteFile(const char *path, char *data);




#ifdef __cplusplus
}
#endif

#endif