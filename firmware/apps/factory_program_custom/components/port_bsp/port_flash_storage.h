#ifndef PORT_FLASH_STORAGE_H
#define PORT_FLASH_STORAGE_H

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define FLASH_STORAGE_MOUNT_POINT "/flash"

bool FlashStorage_Init(void);

#ifdef __cplusplus
}
#endif

#endif
