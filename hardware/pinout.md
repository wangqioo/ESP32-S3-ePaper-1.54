# GPIO 引脚分配

本文件根据用户提供的官方引脚分配图整理。后续导入官方原理图、例程或板级配置文件后，应再核对一次命名与方向。

## 引脚表

| GPIO | e-Paper | SD Card | I2S | UART & USB | RTC | Other | OUTPUT |
| --- | --- | --- | --- | --- | --- | --- | --- |
| IO0 |  |  |  |  |  | BOOT0 |  |
| IO1 |  |  |  |  |  |  | IO1 |
| IO2 |  |  |  |  |  |  | IO2 |
| IO3 |  |  |  |  |  |  | IO3 |
| IO4 |  |  |  |  |  | BAT_ADC |  |
| IO5 |  |  |  |  | RTC_INT |  |  |
| IO6 | EPD3V3_EN |  |  |  |  |  |  |
| IO7 | EPD_TP_RST |  |  |  |  |  |  |
| IO8 | EPD_BUSY |  |  |  |  |  |  |
| IO9 | EPD_RST |  |  |  |  |  |  |
| IO10 | EPD_D/C |  |  |  |  |  |  |
| IO11 | EPD_CS |  |  |  |  |  |  |
| IO12 | EPD_SCLK |  |  |  |  |  |  |
| IO13 | EPD_SDI |  |  |  |  |  |  |
| IO14 |  |  | I2S_MCLK |  |  |  |  |
| IO15 |  |  | I2S_SCLK |  |  |  |  |
| IO16 |  |  | I2S_ASDOUT |  |  |  |  |
| IO17 |  |  |  |  |  | BAT_Control |  |
| IO18 |  |  |  |  |  | PA_EN |  |
| IO19 |  |  |  | U_N |  |  | IO19 |
| IO20 |  |  |  | U_P |  |  | IO20 |
| IO21 | EPD_TP_INT |  |  |  |  |  |  |
| IO38 |  |  | I2S_LRCK |  |  |  |  |
| IO39 |  | SD_CLK |  |  |  |  |  |
| IO40 |  | SD_MISO |  |  |  |  |  |
| IO41 |  | SD_MOSI |  |  |  |  |  |
| IO42 |  |  | PA_EN |  |  |  |  |
| IO43 |  |  |  | TXD |  |  | IO43 |
| IO44 |  |  |  | RXD |  |  | IO44 |
| IO45 |  |  | I2S_DSDIN |  |  |  |  |
| IO46 |  |  | PA_CTRL |  |  |  |  |
| IO47 | EPD_TP_SDA |  |  |  | RTC_SDA |  | IO47 |
| IO48 | EPD_TP_SCL |  |  |  | RTC_SCL |  | IO48 |

## 按功能整理

### 墨水屏与触摸

| 信号 | GPIO |
| --- | --- |
| EPD3V3_EN | IO6 |
| EPD_TP_RST | IO7 |
| EPD_BUSY | IO8 |
| EPD_RST | IO9 |
| EPD_D/C | IO10 |
| EPD_CS | IO11 |
| EPD_SCLK | IO12 |
| EPD_SDI | IO13 |
| EPD_TP_INT | IO21 |
| EPD_TP_SDA | IO47 |
| EPD_TP_SCL | IO48 |

### Micro SD

| 信号 | GPIO |
| --- | --- |
| SD_CLK | IO39 |
| SD_MISO | IO40 |
| SD_MOSI | IO41 |

### I2S / 音频

| 信号 | GPIO |
| --- | --- |
| I2S_MCLK | IO14 |
| I2S_SCLK | IO15 |
| I2S_ASDOUT | IO16 |
| I2S_LRCK | IO38 |
| Audio_PWR / PA_EN | IO42 |
| I2S_DSDIN | IO45 |
| PA_CTRL | IO46 |

> BSP 核对：官方 V2 ESP-IDF 例程将 `Audio_PWR_PIN` 定义为 IO42，将 `PWR_BUTTON_PIN` 定义为 IO18。用户提供的引脚图中 IO18 的 Other 栏标有 `PA_EN`，这里保留原图信息，但应用开发以官方例程和 BSP 映射为准。

### UART / USB

| 信号 | GPIO |
| --- | --- |
| U_N | IO19 |
| U_P | IO20 |
| TXD | IO43 |
| RXD | IO44 |

### RTC

| 信号 | GPIO |
| --- | --- |
| RTC_INT | IO5 |
| RTC_SDA | IO47 |
| RTC_SCL | IO48 |

### 电源与其他

| 信号 | GPIO |
| --- | --- |
| BOOT0 | IO0 |
| BAT_ADC | IO4 |
| BAT_Control | IO17 |
| PWR_BUTTON | IO18 |
| Audio_PWR / PA_EN | IO42 |

### 扩展输出

| GPIO |
| --- |
| IO1 |
| IO2 |
| IO3 |
| IO19 |
| IO20 |
| IO43 |
| IO44 |
| IO47 |
| IO48 |
