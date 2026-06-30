#include <stdio.h>
#include <freertos/FreeRTOS.h>
#include <esp_log.h>
#include <driver/gpio.h>
#include "port_ft6336.h"
#include "port_i2c.h"


I2cFt6336Dev *I2cFt6336Dev::instance_ = NULL;

I2cFt6336Dev * I2cFt6336Dev::requestInstance(i2c_master_bus_handle_t i2c_master_bus,int i2c_dev_addr,int x_max,int y_max) {
    if(instance_ == NULL) {
        instance_ = new I2cFt6336Dev(i2c_master_bus,i2c_dev_addr,x_max,y_max);
    }
    return instance_;
}

I2cFt6336Dev::I2cFt6336Dev(i2c_master_bus_handle_t i2c_master_bus,int i2c_dev_addr,int x_max,int y_max) {
    i2c_device_config_t dev_cfg = {};
    dev_cfg.dev_addr_length = I2C_ADDR_BIT_LEN_7;
    dev_cfg.device_address = i2c_dev_addr;
    dev_cfg.scl_speed_hz = 400000;
  	ESP_ERROR_CHECK(i2c_master_bus_add_device(i2c_master_bus, &dev_cfg, &i2c_master_dev_));
    x_max_ = x_max;
    y_max_ = y_max;
}

I2cFt6336Dev::~I2cFt6336Dev() {

}

void I2cFt6336Dev::Ft6336_Reset(int reset_pin) {
    if(0 == reset_init_flag) {
        gpio_config_t io_conf = {};
        io_conf.intr_type = GPIO_INTR_DISABLE;
        io_conf.pin_bit_mask = (1ULL<<reset_pin);
        io_conf.mode = GPIO_MODE_OUTPUT;
        io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
        io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
        ESP_ERROR_CHECK(gpio_config(&io_conf));
        reset_init_flag = 1;
    } 
    gpio_set_level((gpio_num_t)reset_pin, 1);
    vTaskDelay(pdMS_TO_TICKS(100));
    gpio_set_level((gpio_num_t)reset_pin, 0);
    vTaskDelay(pdMS_TO_TICKS(100));
    gpio_set_level((gpio_num_t)reset_pin, 1);
    vTaskDelay(pdMS_TO_TICKS(100));
}

uint8_t I2cFt6336Dev::GetTouchPoint(uint16_t *x,uint16_t *y) {
    uint8_t data = 0;
    uint8_t buf[4];
    I2cMasterBus::instance_->i2c_read_buff(i2c_master_dev_,0x02,&data,1);
    if(data) { 
        I2cMasterBus::instance_->i2c_read_buff(i2c_master_dev_,0x03,buf,4);
        *x = (((uint16_t)buf[0] & 0x0f)<<8) | (uint16_t)buf[1];
        *y = (((uint16_t)buf[2] & 0x0f)<<8) | (uint16_t)buf[3];
        if(*x > x_max_)
        *x = x_max_;
        if(*y > y_max_)
        *y = y_max_;
        return 1;
    }
    return 0;
}