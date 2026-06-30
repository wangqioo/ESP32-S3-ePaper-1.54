#include <stdio.h>
#include <freertos/FreeRTOS.h>
#include <esp_log.h>
#include <driver/gpio.h>
#include <esp_err.h>
#include "user_config.h"
#include "i2c_bsp.h"
#include "ft6336_bsp.h"

I2cMasterBus *i2c_bus = NULL;
I2cFt6336Dev *ft6336_dev = NULL;


static QueueHandle_t gpio_evt_queue = NULL;

static void IRAM_ATTR gpio_isr_handler(void* arg)
{
    uint32_t gpio_num = (uint32_t) arg;
    xQueueSendFromISR(gpio_evt_queue, &gpio_num, NULL);
}

void gpio_touchint_init() {
    gpio_config_t io_conf = {};
    io_conf.intr_type = GPIO_INTR_NEGEDGE;
    io_conf.pin_bit_mask = (1ULL<<EPD_TP_INT_PIN);
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_conf.pull_up_en = GPIO_PULLUP_ENABLE;
    gpio_config(&io_conf);
    //install gpio isr service
    gpio_install_isr_service(0);
    gpio_isr_handler_add(EPD_TP_INT_PIN, gpio_isr_handler, (void*) EPD_TP_INT_PIN);
    gpio_evt_queue = xQueueCreate(3, sizeof(uint32_t));
}

void touch_test_task(void *pvParameters)
{
    uint32_t io_num;
    for(;;) {
        if (xQueueReceive(gpio_evt_queue, &io_num, portMAX_DELAY)) {
            uint16_t x,y;
            if(ft6336_dev->GetTouchPoint(&x,&y)) {
				ESP_LOGW("Test","Touch Point:(%d,%d)",x,y);
			}
        }
    }
}

extern "C" void app_main(void) {
    i2c_bus = I2cMasterBus::requestInstance(ESP32_I2C_SCL_PIN, ESP32_I2C_SDA_PIN, ESP32_I2C_DEV_NUM);
	assert(i2c_bus);
	ft6336_dev = I2cFt6336Dev::requestInstance(i2c_bus->Get_I2cBusHandle(),I2C_FT6336_DEV_Address,EPD_WIDTH,EPD_HEIGHT);
	assert(ft6336_dev);
	ft6336_dev->Ft6336_Reset(EPD_TP_RST_PIN);
	gpio_touchint_init();
	xTaskCreatePinnedToCore(touch_test_task, "touch_test_task", 4 * 1024, NULL, 2, NULL,1);
}
