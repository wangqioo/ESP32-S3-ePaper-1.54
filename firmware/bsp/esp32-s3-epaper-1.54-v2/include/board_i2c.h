#pragma once

#include <stdint.h>
#include "driver/i2c_master.h"
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

esp_err_t board_i2c_init(void);
i2c_master_bus_handle_t board_i2c_get_bus(void);
esp_err_t board_i2c_register_device(uint16_t address, i2c_master_dev_handle_t *out_handle);
esp_err_t board_i2c_write_read(i2c_master_dev_handle_t dev_handle,
                               const uint8_t *write_buf,
                               uint8_t write_len,
                               uint8_t *read_buf,
                               uint8_t read_len);
esp_err_t board_i2c_read_reg(i2c_master_dev_handle_t dev_handle, uint8_t reg, uint8_t *buf, uint8_t len);
esp_err_t board_i2c_write_reg(i2c_master_dev_handle_t dev_handle, uint8_t reg, const uint8_t *buf, uint8_t len);

#ifdef __cplusplus
}
#endif
