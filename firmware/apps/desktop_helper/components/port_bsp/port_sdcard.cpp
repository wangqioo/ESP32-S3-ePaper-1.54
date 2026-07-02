#include <stdio.h>
#include <string.h>
#include <sys/unistd.h>
#include <sys/stat.h>
#include <esp_vfs_fat.h>
#include <sdmmc_cmd.h>
#include <driver/gpio.h>
#include <freertos/FreeRTOS.h>
#include <esp_log.h>
#include "port_sdcard.h"
#include "epaper_config.h"

static const char *TAG = "sdcard";

sdmmc_card_t *card = NULL;  
uint32_t sdcard_slot = 0;


bool Sdcard_Init(void)
{
  	esp_vfs_fat_sdmmc_mount_config_t mount_config = {};
    mount_config.format_if_mount_failed = false;     //If the hook fails, create a partition table and format the SD car
    mount_config.max_files = 5;                      //Maximum number of open files
    mount_config.allocation_unit_size = 2 * 1024;         //Similar to sector size

    sdmmc_host_t host = SDMMC_HOST_DEFAULT();
    host.max_freq_khz = SDMMC_FREQ_HIGHSPEED;

    sdmmc_slot_config_t slot_config = SDMMC_SLOT_CONFIG_DEFAULT();
    slot_config.width = 1;           //1-wire
    slot_config.flags |= SDMMC_SLOT_FLAG_INTERNAL_PULLUP;
    slot_config.cd = SDMMC_SLOT_NO_CD;
    slot_config.wp = SDMMC_SLOT_NO_WP;
    slot_config.clk = SD_CLK_PIN;
    slot_config.cmd = SD_MOSI_CMD_PIN;
    slot_config.d0 = SD_MISO_D0_PIN;
    slot_config.d1 = GPIO_NUM_NC;
    slot_config.d2 = GPIO_NUM_NC;
    slot_config.d3 = GPIO_NUM_NC;
    esp_err_t ret = esp_vfs_fat_sdmmc_mount(SDlist, &host, &slot_config, &mount_config, &card);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to mount SD card: %s", esp_err_to_name(ret));
        card = NULL;
        sdcard_slot = 0;
        return false;
    }

  	if(card != NULL) {
  	  	sdmmc_card_print_info(stdout, card);
		sdcard_slot = 1;
		return 1;
	}
	return 0;
}

float sd_card_get_value(void) {
  	if(card != NULL) {
  	  	return (float)(card->csd.capacity)/2048/1024; 
  	}
  	return -1;
}

esp_err_t Sdcard_WriteFile(const char *path, char *data)
{
  	esp_err_t err;
  	if(card == NULL)
  	{
  	  	return ESP_ERR_NOT_FOUND;
  	}
  	err = sdmmc_get_status(card); //First check if there is an SD card
  	if(err != ESP_OK)
  	{
  	  	return err;
  	}
  	FILE *f = fopen(path, "w"); //Get path address
  	if(f == NULL)
  	{
  	  	ESP_LOGI(TAG,"path:Write Wrong path");
  	  	return ESP_ERR_NOT_FOUND;
  	}
  	fprintf(f, data); //write in
  	fclose(f);
  	return ESP_OK;
}

esp_err_t Sdcard_ReadFile(const char *path,char *pxbuf,uint32_t *outLen)
{
  	esp_err_t err;
  	if(card == NULL)
  	{
  	  	ESP_LOGI(TAG,"card == NULL");
  	  	return ESP_ERR_NOT_FOUND;
  	}
  	err = sdmmc_get_status(card); //First check if there is an SD card
  	if(err != ESP_OK)
  	{
  	  	ESP_LOGI(TAG,"card == NO");
  	  	return err;
  	}
  	FILE *f = fopen(path, "rb");
  	if (f == NULL)
  	{
  	  	ESP_LOGI(TAG,"Read Wrong path");
  	  	return ESP_ERR_NOT_FOUND;
  	}
  	fseek(f, 0, SEEK_END);     //Move the pointer to the back
  	uint32_t unlen = ftell(f);
  	//fgets(pxbuf, unlen, f);  //Read text
  	fseek(f, 0, SEEK_SET);     //Move the pointer to the front
  	uint32_t poutLen = fread((void *)pxbuf,1,unlen,f);
  	if(outLen != NULL)
  	*outLen = poutLen;
  	fclose(f);
  	return ESP_OK;
}
