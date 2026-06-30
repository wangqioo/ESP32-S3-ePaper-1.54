#include "board_sdcard.h"

#include <stdio.h>

#include "board_pins.h"
#include "driver/sdmmc_host.h"
#include "esp_check.h"
#include "esp_vfs_fat.h"
#include "sdmmc_cmd.h"

#define BOARD_SDCARD_MOUNT_POINT "/sdcard"

static const char *TAG = "board_sdcard";
static sdmmc_card_t *s_card = NULL;

esp_err_t board_sdcard_mount(void)
{
    if (s_card != NULL) {
        return ESP_OK;
    }

    esp_vfs_fat_sdmmc_mount_config_t mount_config = {
        .format_if_mount_failed = false,
        .max_files = 5,
        .allocation_unit_size = 16 * 1024 * 3,
    };

    sdmmc_host_t host = SDMMC_HOST_DEFAULT();
    host.max_freq_khz = SDMMC_FREQ_HIGHSPEED;

    sdmmc_slot_config_t slot_config = SDMMC_SLOT_CONFIG_DEFAULT();
    slot_config.width = 1;
    slot_config.clk = BOARD_SD_CLK_PIN;
    slot_config.cmd = BOARD_SD_MOSI_PIN;
    slot_config.d0 = BOARD_SD_MISO_PIN;

    ESP_RETURN_ON_ERROR(esp_vfs_fat_sdmmc_mount(BOARD_SDCARD_MOUNT_POINT,
                                                &host,
                                                &slot_config,
                                                &mount_config,
                                                &s_card),
                        TAG,
                        "mount failed");
    return ESP_OK;
}

esp_err_t board_sdcard_unmount(void)
{
    if (s_card == NULL) {
        return ESP_OK;
    }

    esp_err_t err = esp_vfs_fat_sdcard_unmount(BOARD_SDCARD_MOUNT_POINT, s_card);
    if (err == ESP_OK) {
        s_card = NULL;
    }
    return err;
}

esp_err_t board_sdcard_read_file(const char *path, char *buffer, uint32_t *out_len)
{
    ESP_RETURN_ON_FALSE(path != NULL, ESP_ERR_INVALID_ARG, TAG, "path is null");
    ESP_RETURN_ON_FALSE(buffer != NULL, ESP_ERR_INVALID_ARG, TAG, "buffer is null");
    ESP_RETURN_ON_FALSE(out_len != NULL, ESP_ERR_INVALID_ARG, TAG, "out_len is null");
    ESP_RETURN_ON_FALSE(s_card != NULL, ESP_ERR_INVALID_STATE, TAG, "card not mounted");
    ESP_RETURN_ON_ERROR(sdmmc_get_status(s_card), TAG, "card status failed");

    FILE *file = fopen(path, "rb");
    ESP_RETURN_ON_FALSE(file != NULL, ESP_ERR_NOT_FOUND, TAG, "open read failed");

    esp_err_t err = ESP_OK;
    if (fseek(file, 0, SEEK_END) != 0) {
        err = ESP_FAIL;
        goto cleanup;
    }

    long file_len = ftell(file);
    if (file_len < 0) {
        err = ESP_FAIL;
        goto cleanup;
    }

    if (fseek(file, 0, SEEK_SET) != 0) {
        err = ESP_FAIL;
        goto cleanup;
    }

    size_t bytes_read = fread(buffer, 1, (size_t)file_len, file);
    if (bytes_read != (size_t)file_len && ferror(file)) {
        err = ESP_FAIL;
        goto cleanup;
    }

    *out_len = bytes_read;

cleanup:
    fclose(file);
    return err;
}

esp_err_t board_sdcard_write_file(const char *path, const char *data)
{
    ESP_RETURN_ON_FALSE(path != NULL, ESP_ERR_INVALID_ARG, TAG, "path is null");
    ESP_RETURN_ON_FALSE(data != NULL, ESP_ERR_INVALID_ARG, TAG, "data is null");
    ESP_RETURN_ON_FALSE(s_card != NULL, ESP_ERR_INVALID_STATE, TAG, "card not mounted");
    ESP_RETURN_ON_ERROR(sdmmc_get_status(s_card), TAG, "card status failed");

    FILE *file = fopen(path, "w");
    ESP_RETURN_ON_FALSE(file != NULL, ESP_ERR_NOT_FOUND, TAG, "open write failed");

    esp_err_t err = fputs(data, file) >= 0 ? ESP_OK : ESP_FAIL;
    fclose(file);
    return err;
}

float board_sdcard_get_capacity_gb(void)
{
    if (s_card == NULL) {
        return 0.0f;
    }

    return (float)s_card->csd.capacity / 2048.0f / 1024.0f;
}
