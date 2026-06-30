# 官方 ESP-IDF V2 例程

本目录保存官方 V2 硬件版本 ESP-IDF 例程，来源：

`/Users/wq/Downloads/ESP32-S3-ePaper-1.54-main/02_Example/ESP-IDF/V2`

V1 与 V2 的例程不通用。本目录只适用于 V2 硬件：ESP32-S3-PICO-1-N8R8，8MB Flash，8MB PSRAM。

## 示例工程

| 目录 | 功能 |
| --- | --- |
| `01_ADC_Test` | 电池 ADC 测试 |
| `02_I2C_PCF85063` | PCF85063 RTC 测试 |
| `03_I2C_SHTC3` | SHTC3 温湿度传感器测试 |
| `04_SD_Card` | Micro SD 卡测试 |
| `05_WIFI_AP` | Wi-Fi AP 测试 |
| `06_WIFI_STA` | Wi-Fi STA 测试 |
| `07_BATT_PWR_Test` | 电池供电与电源控制测试 |
| `08_Audio_Test` | ES8311 / I2S 音频测试 |
| `09_LVGL_V8_Test` | LVGL v8 墨水屏显示测试 |
| `10_LVGL_V9_Test` | LVGL v9 墨水屏显示测试 |
| `11_FactoryProgram` | 出厂综合程序 |
| `12_RTC_Sleep_Test` | RTC 与睡眠模式测试 |
| `13_FT6336_Test` | FT6336 触摸测试 |

## 构建方式

每个子目录都是独立 ESP-IDF 工程。进入具体示例目录后再运行：

```sh
idf.py set-target esp32s3
idf.py build
idf.py flash monitor
```

实际串口端口、ESP-IDF 版本和芯片烧录参数需要结合本机开发环境确认。
