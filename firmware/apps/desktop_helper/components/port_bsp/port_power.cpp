#include <stdio.h>
#include <freertos/FreeRTOS.h>
#include <driver/gpio.h>
#include "port_power.h"
#include "epaper_config.h"

void BoardPower_Init() {
    gpio_config_t gpio_conf = {};                                                            
    gpio_conf.intr_type = GPIO_INTR_DISABLE;                                             
    gpio_conf.mode = GPIO_MODE_OUTPUT;                                                   
    gpio_conf.pin_bit_mask = (0x1ULL << EPD_PWR_PIN) | (0x1ULL << Audio_PWR_PIN) | (0x1ULL << VBAT_PWR_PIN);
    gpio_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;                                      
    gpio_conf.pull_up_en = GPIO_PULLUP_ENABLE;                                           
    ESP_ERROR_CHECK_WITHOUT_ABORT(gpio_config(&gpio_conf));  
}

void BoardPower_EPD_ON() {
    gpio_set_level((gpio_num_t)EPD_PWR_PIN,0);
}

void BoardPower_EPD_OFF() {
    gpio_set_level((gpio_num_t)EPD_PWR_PIN,1);
}

void BoardPower_Audio_ON() {
    gpio_set_level((gpio_num_t)Audio_PWR_PIN,0);
}

void BoardPower_Audio_OFF() {
    gpio_set_level((gpio_num_t)Audio_PWR_PIN,1);
}

void BoardPower_VBAT_ON() {
    gpio_set_level((gpio_num_t)VBAT_PWR_PIN,1);
}

void BoardPower_VBAT_OFF() {
    gpio_set_level((gpio_num_t)VBAT_PWR_PIN,0);
}