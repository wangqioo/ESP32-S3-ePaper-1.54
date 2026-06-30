#include "board_i2c.h"

#include <string.h>
#include "board_pins.h"
#include "esp_check.h"

static const char *TAG = "board_i2c";
static i2c_master_bus_handle_t s_i2c_bus = NULL;

esp_err_t board_i2c_init(void)
{
    if (s_i2c_bus != NULL) {
        return ESP_OK;
    }

    i2c_master_bus_config_t bus_config = {
        .i2c_port = BOARD_I2C_PORT,
        .sda_io_num = BOARD_I2C_SDA_PIN,
        .scl_io_num = BOARD_I2C_SCL_PIN,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7,
        .intr_priority = 0,
        .trans_queue_depth = 0,
        .flags = {
            .enable_internal_pullup = true,
        },
    };

    return i2c_new_master_bus(&bus_config, &s_i2c_bus);
}

i2c_master_bus_handle_t board_i2c_get_bus(void)
{
    return s_i2c_bus;
}

esp_err_t board_i2c_register_device(uint16_t address, i2c_master_dev_handle_t *out_handle)
{
    ESP_RETURN_ON_FALSE(out_handle != NULL, ESP_ERR_INVALID_ARG, TAG, "out_handle is null");
    ESP_RETURN_ON_ERROR(board_i2c_init(), TAG, "i2c init failed");

    i2c_device_config_t dev_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = address,
        .scl_speed_hz = BOARD_I2C_SPEED_HZ,
    };

    return i2c_master_bus_add_device(s_i2c_bus, &dev_cfg, out_handle);
}

esp_err_t board_i2c_write_read(i2c_master_dev_handle_t dev_handle,
                               const uint8_t *write_buf,
                               uint8_t write_len,
                               uint8_t *read_buf,
                               uint8_t read_len)
{
    ESP_RETURN_ON_FALSE(dev_handle != NULL, ESP_ERR_INVALID_ARG, TAG, "device handle is null");
    ESP_RETURN_ON_FALSE(write_buf != NULL || write_len == 0, ESP_ERR_INVALID_ARG, TAG, "write buffer is null");
    ESP_RETURN_ON_FALSE(read_buf != NULL || read_len == 0, ESP_ERR_INVALID_ARG, TAG, "read buffer is null");
    return i2c_master_transmit_receive(dev_handle, write_buf, write_len, read_buf, read_len, -1);
}

esp_err_t board_i2c_read_reg(i2c_master_dev_handle_t dev_handle, uint8_t reg, uint8_t *buf, uint8_t len)
{
    return board_i2c_write_read(dev_handle, &reg, 1, buf, len);
}

esp_err_t board_i2c_write_reg(i2c_master_dev_handle_t dev_handle, uint8_t reg, const uint8_t *buf, uint8_t len)
{
    uint8_t write_buf[32];
    ESP_RETURN_ON_FALSE(dev_handle != NULL, ESP_ERR_INVALID_ARG, TAG, "device handle is null");
    ESP_RETURN_ON_FALSE(buf != NULL || len == 0, ESP_ERR_INVALID_ARG, TAG, "write buffer is null");
    ESP_RETURN_ON_FALSE(len + 1 <= sizeof(write_buf), ESP_ERR_INVALID_SIZE, TAG, "i2c write too large");
    write_buf[0] = reg;
    memcpy(&write_buf[1], buf, len);
    return i2c_master_transmit(dev_handle, write_buf, len + 1, -1);
}
