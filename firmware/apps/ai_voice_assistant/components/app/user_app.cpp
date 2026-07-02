#include "user_app.h"

#include "assistant_client.h"
#include "assistant_ui.h"
#include "epaper_config.h"
#include "port_i2c.h"
#include "port_power.h"

#include <esp_log.h>

namespace {

constexpr const char *TAG = "ai_voice";
AssistantUi assistant_ui;
I2cMasterBus *i2c_bus = nullptr;

}  // namespace

void UserApp_Init() {
    ESP_LOGI(TAG, "init");
    BoardPower_Init();
    BoardPower_EPD_ON();
    BoardPower_Audio_OFF();
    i2c_bus = I2cMasterBus::requestInstance(ESP32_I2C_SCL_PIN, ESP32_I2C_SDA_PIN, ESP32_I2C_DEV_NUM);
}

void UserUi_Init() {
    assistant_ui.Init();
    assistant_ui.ShowHome(AssistantWifiConfigured(), AssistantProxyConfigured());
}

void UserApp_Start_Init() {
    ESP_LOGI(TAG, "start");
}
