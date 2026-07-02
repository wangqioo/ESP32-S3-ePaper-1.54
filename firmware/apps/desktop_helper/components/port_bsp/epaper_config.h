#ifndef EPAPER_CONFIG_H
#define EPAPER_CONFIG_H

#define EPD_SPI_NUM        SPI2_HOST
#define ESP32_I2C_DEV_NUM  I2C_NUM_0

#define EPD_WIDTH  200
#define EPD_HEIGHT 200

/*EPD port Init*/
#define EPD_DC_PIN      GPIO_NUM_10
#define EPD_CS_PIN      GPIO_NUM_11
#define EPD_SCK_PIN     GPIO_NUM_12
#define EPD_MOSI_PIN    GPIO_NUM_13
#define EPD_RST_PIN     GPIO_NUM_9
#define EPD_BUSY_PIN    GPIO_NUM_8
#define EPD_TP_RST_PIN  GPIO_NUM_7
#define EPD_TP_INT_PIN  GPIO_NUM_21

/*DEV POWER init*/
#define EPD_PWR_PIN     GPIO_NUM_6
#define Audio_PWR_PIN   GPIO_NUM_42
#define VBAT_PWR_PIN    GPIO_NUM_17

#define BOOT_BUTTON_PIN GPIO_NUM_0
#define PWR_BUTTON_PIN  GPIO_NUM_18

#define LED_PIN         GPIO_NUM_3

/*Low-power wake-up*/
#define ext_wakeup_pin_1 GPIO_NUM_0

/*ESP32 I2C Init*/
#define ESP32_I2C_SDA_PIN GPIO_NUM_47
#define ESP32_I2C_SCL_PIN GPIO_NUM_48

/*sdcard*/
#define SD_MISO_D0_PIN      GPIO_NUM_40
#define SD_MOSI_CMD_PIN     GPIO_NUM_41
#define SD_CLK_PIN          GPIO_NUM_39

#define SDlist "/sdcard" 

/*i2c dev*/
#define I2C_RTC_DEV_Address        0x51
#define I2C_SHTC3_DEV_Address      0x70
#define I2C_FT6336_DEV_Address     0x38         

#endif // !USER_CONFIG_H