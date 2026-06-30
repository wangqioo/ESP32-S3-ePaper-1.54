#include "board_sensors.h"

#include "SensorPCF85063.hpp"
#include "board_i2c.h"
#include "board_pins.h"
#include "driver/gpio.h"
#include "esp_check.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "board_sensors";

static SensorPCF85063 s_rtc;
static bool s_rtc_initialized;
static i2c_master_dev_handle_t s_shtc3_handle;
static bool s_shtc3_initialized;

enum {
    SHTC3_READ_ID = 0xEFC8,
    SHTC3_SOFT_RESET = 0x805D,
    SHTC3_SLEEP = 0xB098,
    SHTC3_WAKEUP = 0x3517,
    SHTC3_MEAS_T_RH_POLLING = 0x7866,
    SHTC3_CRC_POLYNOMIAL = 0x131,
};

static constexpr float SHTC3_TEMP_OFFSET_C = 4.0f;

static esp_err_t shtc3_write_command(uint16_t command)
{
    uint8_t buf[2] = {
        static_cast<uint8_t>(command >> 8),
        static_cast<uint8_t>(command & 0xff),
    };
    return i2c_master_transmit(s_shtc3_handle, buf, sizeof(buf), -1);
}

static esp_err_t shtc3_write_read_command(uint16_t command, uint8_t *read_buf, size_t read_len)
{
    uint8_t write_buf[2] = {
        static_cast<uint8_t>(command >> 8),
        static_cast<uint8_t>(command & 0xff),
    };
    return i2c_master_transmit_receive(s_shtc3_handle, write_buf, sizeof(write_buf), read_buf, read_len, -1);
}

static bool shtc3_check_crc(const uint8_t *data, uint8_t len, uint8_t checksum)
{
    uint8_t crc = 0xff;

    for (uint8_t byte = 0; byte < len; byte++) {
        crc ^= data[byte];
        for (uint8_t bit = 8; bit > 0; bit--) {
            if ((crc & 0x80) != 0) {
                crc = static_cast<uint8_t>((crc << 1) ^ SHTC3_CRC_POLYNOMIAL);
            } else {
                crc = static_cast<uint8_t>(crc << 1);
            }
        }
    }

    return crc == checksum;
}

static float shtc3_calc_temperature(uint16_t raw_value)
{
    return 175.0f * static_cast<float>(raw_value) / 65536.0f - 45.0f - SHTC3_TEMP_OFFSET_C;
}

static float shtc3_calc_humidity(uint16_t raw_value)
{
    return 100.0f * static_cast<float>(raw_value) / 65536.0f;
}

static esp_err_t shtc3_init(void)
{
    if (s_shtc3_initialized) {
        return ESP_OK;
    }

    if (s_shtc3_handle == NULL) {
        ESP_RETURN_ON_ERROR(board_i2c_register_device(BOARD_SHTC3_I2C_ADDR, &s_shtc3_handle),
                            TAG,
                            "shtc3 register failed");
    }

    ESP_RETURN_ON_ERROR(shtc3_write_command(SHTC3_WAKEUP), TAG, "shtc3 wake failed");
    vTaskDelay(pdMS_TO_TICKS(50));
    ESP_RETURN_ON_ERROR(shtc3_write_command(SHTC3_SOFT_RESET), TAG, "shtc3 reset failed");
    vTaskDelay(pdMS_TO_TICKS(20));

    uint8_t id_buf[3] = {};
    ESP_RETURN_ON_ERROR(shtc3_write_read_command(SHTC3_READ_ID, id_buf, sizeof(id_buf)),
                        TAG,
                        "shtc3 id read failed");
    ESP_RETURN_ON_FALSE(shtc3_check_crc(id_buf, 2, id_buf[2]), ESP_ERR_INVALID_CRC, TAG, "shtc3 id crc failed");

    uint16_t sensor_id = (static_cast<uint16_t>(id_buf[0]) << 8) | id_buf[1];
    ESP_LOGI(TAG, "shtc3 id: %04x", sensor_id);
    s_shtc3_initialized = true;
    return ESP_OK;
}

static esp_err_t rtc_init(void)
{
    if (s_rtc_initialized) {
        return ESP_OK;
    }

    ESP_RETURN_ON_FALSE(board_i2c_get_bus() != NULL, ESP_ERR_INVALID_STATE, TAG, "i2c bus not initialized");
    ESP_RETURN_ON_FALSE(s_rtc.begin(board_i2c_get_bus(), BOARD_RTC_I2C_ADDR), ESP_FAIL, TAG, "rtc init failed");
    s_rtc_initialized = true;
    return ESP_OK;
}

esp_err_t board_sensors_init(void)
{
    ESP_RETURN_ON_ERROR(board_i2c_init(), TAG, "i2c init failed");
    ESP_RETURN_ON_ERROR(rtc_init(), TAG, "rtc init failed");
    ESP_RETURN_ON_ERROR(shtc3_init(), TAG, "shtc3 init failed");
    return ESP_OK;
}

esp_err_t board_rtc_get_time(board_rtc_time_t *out_time)
{
    ESP_RETURN_ON_FALSE(out_time != NULL, ESP_ERR_INVALID_ARG, TAG, "out_time is null");
    ESP_RETURN_ON_ERROR(board_sensors_init(), TAG, "sensors init failed");

    RTC_DateTime datetime = s_rtc.getDateTime();
    out_time->year = datetime.getYear();
    out_time->month = datetime.getMonth();
    out_time->day = datetime.getDay();
    out_time->hour = datetime.getHour();
    out_time->minute = datetime.getMinute();
    out_time->second = datetime.getSecond();
    out_time->week = datetime.getWeek();
    return ESP_OK;
}

esp_err_t board_rtc_set_time(const board_rtc_time_t *time)
{
    ESP_RETURN_ON_FALSE(time != NULL, ESP_ERR_INVALID_ARG, TAG, "time is null");
    ESP_RETURN_ON_ERROR(board_sensors_init(), TAG, "sensors init failed");

    s_rtc.setDateTime(RTC_DateTime(time->year,
                                   time->month,
                                   time->day,
                                   time->hour,
                                   time->minute,
                                   time->second,
                                   time->week));
    return ESP_OK;
}

esp_err_t board_shtc3_read(float *temperature_c, float *humidity_percent)
{
    ESP_RETURN_ON_FALSE(temperature_c != NULL, ESP_ERR_INVALID_ARG, TAG, "temperature_c is null");
    ESP_RETURN_ON_FALSE(humidity_percent != NULL, ESP_ERR_INVALID_ARG, TAG, "humidity_percent is null");
    ESP_RETURN_ON_ERROR(board_sensors_init(), TAG, "sensors init failed");

    ESP_RETURN_ON_ERROR(shtc3_write_command(SHTC3_WAKEUP), TAG, "shtc3 wake failed");
    vTaskDelay(pdMS_TO_TICKS(50));
    ESP_RETURN_ON_ERROR(shtc3_write_command(SHTC3_MEAS_T_RH_POLLING), TAG, "shtc3 measure command failed");
    vTaskDelay(pdMS_TO_TICKS(20));

    uint8_t bytes[6] = {};
    ESP_RETURN_ON_ERROR(i2c_master_receive(s_shtc3_handle, bytes, sizeof(bytes), -1), TAG, "shtc3 read failed");
    ESP_RETURN_ON_FALSE(shtc3_check_crc(bytes, 2, bytes[2]), ESP_ERR_INVALID_CRC, TAG, "shtc3 temp crc failed");
    ESP_RETURN_ON_FALSE(shtc3_check_crc(&bytes[3], 2, bytes[5]),
                        ESP_ERR_INVALID_CRC,
                        TAG,
                        "shtc3 humidity crc failed");

    uint16_t raw_temp = (static_cast<uint16_t>(bytes[0]) << 8) | bytes[1];
    uint16_t raw_humidity = (static_cast<uint16_t>(bytes[3]) << 8) | bytes[4];
    *temperature_c = shtc3_calc_temperature(raw_temp);
    *humidity_percent = shtc3_calc_humidity(raw_humidity);

    esp_err_t sleep_err = shtc3_write_command(SHTC3_SLEEP);
    if (sleep_err != ESP_OK) {
        ESP_LOGW(TAG, "shtc3 sleep failed: %s", esp_err_to_name(sleep_err));
    }
    return ESP_OK;
}

esp_err_t board_shtc3_read_data(board_shtc3_data_t *out_data)
{
    ESP_RETURN_ON_FALSE(out_data != NULL, ESP_ERR_INVALID_ARG, TAG, "out_data is null");
    return board_shtc3_read(&out_data->temperature_c, &out_data->humidity_percent);
}
