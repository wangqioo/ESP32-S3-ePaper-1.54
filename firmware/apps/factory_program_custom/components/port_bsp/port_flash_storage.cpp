#include "port_flash_storage.h"

#include <esp_err.h>
#include <esp_log.h>
#include <esp_vfs_fat.h>
#include <wear_levelling.h>

static const char *TAG = "flash_storage";
static wl_handle_t s_wl_handle = WL_INVALID_HANDLE;

bool FlashStorage_Init(void) {
    if (s_wl_handle != WL_INVALID_HANDLE) {
        return true;
    }

    esp_vfs_fat_mount_config_t mount_config = {};
    mount_config.max_files = 5;
    mount_config.format_if_mount_failed = true;
    mount_config.allocation_unit_size = CONFIG_WL_SECTOR_SIZE;

    esp_err_t ret = esp_vfs_fat_spiflash_mount_rw_wl(
        FLASH_STORAGE_MOUNT_POINT,
        "storage",
        &mount_config,
        &s_wl_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to mount flash storage: %s", esp_err_to_name(ret));
        s_wl_handle = WL_INVALID_HANDLE;
        return false;
    }

    ESP_LOGI(TAG, "Mounted flash storage at %s", FLASH_STORAGE_MOUNT_POINT);
    return true;
}
