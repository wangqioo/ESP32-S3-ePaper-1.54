#include <stdio.h>
#include <freertos/FreeRTOS.h>
#include <esp_log.h>
#include <driver/gpio.h>
#include <esp_log.h>
#include "port_shtc3.h"
#include "port_i2c.h"
#include "epaper_config.h"

#define TAG "shtc3"

#define CRC_POLYNOMIAL 0x131
#define SHTC3_PETP_NUM 4

static uint16_t SHTC3_ID;
static i2c_master_dev_handle_t i2c_master_dev;
I2cMasterBus *i2cmaster;

etError Shtc3_Wakeup() {
	uint8_t senBuf[2] = {(WAKEUP >> 8),(WAKEUP & 0xff)};
  	int err = i2cmaster->i2c_write_buff(i2c_master_dev,-1,senBuf,2);
  	etError error = (err==ESP_OK) ? NO_ERROR : ACK_ERROR;
  	//esp_rom_delay_us(100); //100us
  	vTaskDelay(pdMS_TO_TICKS(50));  //50MS
  	if(error != NO_ERROR)
  	ESP_LOGE(TAG,"Wakeup Failure");
  	return error;
}

etError Shtc3_SoftReset() {
  	uint8_t senBuf[2] = {(SOFT_RESET>>8),(SOFT_RESET&0xff)};
  	int err = i2cmaster->i2c_write_buff(i2c_master_dev,-1,senBuf,2);
  	etError error = (err==ESP_OK) ? NO_ERROR : ACK_ERROR;
  	if(error != NO_ERROR)
  	ESP_LOGE(TAG,"SoftReset Failure");
  	return error;
}

etError SHTC3_CheckCrc(uint8_t data[], uint8_t nbrOfBytes,uint8_t checksum) {
  	uint8_t bit;       
  	uint8_t crc = 0xFF;
  	uint8_t byteCtr;   

  	for(byteCtr = 0; byteCtr < nbrOfBytes; byteCtr++) {
  	  	crc ^= (data[byteCtr]);
  	  	for(bit = 8; bit > 0; --bit) {
  	  	  	if(crc & 0x80) {
  	  	  	  	crc = (crc << 1) ^ CRC_POLYNOMIAL;
  	  	  	} else {
  	  	  	  	crc = (crc << 1);
  	  	  	}
  	  	}
  	}

  	if(crc != checksum) {
  	  	return CHECKSUM_ERROR;
  	} else {
  	  	return NO_ERROR;
  	}
}

etError Shtc3_GetId() {
	uint8_t senBuf[2] = {(READ_ID>>8),(READ_ID&0xff)};
  	uint8_t readBuf[3] = {0,0,0};
  	int err = i2cmaster->i2c_master_write_read_dev(i2c_master_dev,senBuf,2,readBuf,3);
  	etError error = (err==ESP_OK) ? NO_ERROR : ACK_ERROR;
  	if(error != NO_ERROR)
  	{ESP_LOGE(TAG,"GetId WRITE Failure");return error;}
  	error = SHTC3_CheckCrc(readBuf,2,readBuf[2]);
  	if(error != NO_ERROR)
  	{ESP_LOGE(TAG,"GetId CRC Failure");return error;}
  	SHTC3_ID = ((readBuf[0] << 8) | readBuf[1]);
  	return error;
}

etError Shtc3_Sleep() {
  	uint8_t senBuf[2] = {(SLEEP>>8),(SLEEP&0xff)};
  	int err = i2cmaster->i2c_write_buff(i2c_master_dev,-1,senBuf,2);
  	etError error = (err==ESP_OK) ? NO_ERROR : ACK_ERROR;
  	if(error != NO_ERROR)
  	ESP_LOGE(TAG,"Sleep Failure");
  	return error;
}

void Shtc3_Init(I2cMasterBus *i2cmasterbus) {
    i2cmaster = i2cmasterbus;
    i2c_device_config_t dev_cfg = {};
    dev_cfg.dev_addr_length = I2C_ADDR_BIT_LEN_7;
    dev_cfg.device_address = I2C_SHTC3_DEV_Address;
    dev_cfg.scl_speed_hz = 400000;
  	ESP_ERROR_CHECK(i2c_master_bus_add_device(i2cmasterbus->Get_I2cBusHandle(), &dev_cfg, &i2c_master_dev));

    Shtc3_Wakeup();
    Shtc3_SoftReset();
    vTaskDelay(pdMS_TO_TICKS(20));  //20MS
    Shtc3_GetId();
    ESP_LOGW(TAG,"ID:%04x",SHTC3_ID);
}

float SHTC3_CalcTemperature(uint16_t rawValue) {
  return 175 * (float)rawValue / 65536.0f - 45.0f - SHTC3_PETP_NUM;
}

float SHTC3_CalcHumidity(uint16_t rawValue) {
  return 100 * (float)rawValue / 65536.0f;
}

etError SHTC3_GetTempAndHumiPolling(float *temp, float *humi)
{
  	int err = 0;
  	etError  error;           // error code
  	uint16_t rawValueTemp;    // temperature raw value from sensor
  	uint16_t rawValueHumi;    // humidity raw value from sensor
  	uint8_t bytes[6] = {0};;
  	uint8_t senBuf[2] = {(MEAS_T_RH_POLLING>>8),(MEAS_T_RH_POLLING&0xff)};
  	err = i2cmaster->i2c_write_buff(i2c_master_dev,-1,senBuf,2);
  	error = (err==ESP_OK) ? NO_ERROR : ACK_ERROR;
  	if(error != NO_ERROR)
  	{ESP_LOGE(TAG,"GetTempAndHumi WRITE Failure");return error;}
	
  	vTaskDelay(pdMS_TO_TICKS(20));

  	// if no error, read temperature and humidity raw values
  	err = i2cmaster->i2c_read_buff(i2c_master_dev,-1,bytes,6);
  	error = (err == ESP_OK) ? NO_ERROR : ACK_ERROR;
  	if(error != NO_ERROR)
  	{ESP_LOGE(TAG,"GetTempAndHumi READ Failure"); return error;}
  	error = SHTC3_CheckCrc(bytes, 2, bytes[2]);
  	if(error != NO_ERROR)
  	{ESP_LOGE(TAG,"GetTempAndHumi TempCRC Failure"); return error;}
  	error = SHTC3_CheckCrc(&bytes[3], 2, bytes[5]);
  	if(error != NO_ERROR)
  	{ESP_LOGE(TAG,"GetTempAndHumi humidityCRC Failure"); return error;}
  	// if no error, calculate temperature in °C and humidity in %RH
  	rawValueTemp = (bytes[0] << 8) | bytes[1];
  	rawValueHumi = (bytes[3] << 8) | bytes[4];
  	*temp = SHTC3_CalcTemperature(rawValueTemp);
  	*humi = SHTC3_CalcHumidity(rawValueHumi);
  	return error;
}

void Shtc3_ReadTempHumi(float * t,float * h) {
    etError  error;
  	float temperature;
  	float humidity;
  	Shtc3_Wakeup();
  	error = SHTC3_GetTempAndHumiPolling(&temperature, &humidity);
  	if(error != NO_ERROR) {
  	  ESP_LOGE(TAG,"error:%d",error);
      *t = -1000;
      *h = -1000;
  	} else {
  	  *t = temperature;
      *h = humidity;
  	}
  	Shtc3_Sleep();
}

