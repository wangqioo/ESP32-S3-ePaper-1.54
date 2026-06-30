# 官方介绍整理

本文件根据用户提供的官方产品介绍整理，便于后续导入固件例程和硬件资料时引用。

## 核心定位

ESP32-S3 ePaper 1.54 是一款面向 AIoT 场景的低功耗墨水屏开发板，适合物联网终端、电子标签、便携显示器、语音交互设备和环境监测设备。

## 关键约束

- V1 与 V2 硬件版本的官方例程不通用。
- 2025-11-01 起产品切换到 V2。
- 产品背面贴有 `V2` 标识，或 PCB 左上角有 `V2` 丝印的，为 V2 版本。
- Micro SD 卡使用前需要格式化为 FAT32。
- 按住 BOOT 并重新上电可进入下载模式。

## 后续待补充

- 官方例程来源与版本
- ESP-IDF 或 Arduino 开发环境要求
- V1/V2 分支或目录组织方式
- 烧录步骤
- 功耗测试记录

## 已整理资料

- [GPIO 引脚分配](../hardware/pinout.md)
- [官方 ESP-IDF V2 例程](../firmware/official/v2/esp-idf/README.md)
- [可复用 ESP-IDF V2 BSP](../firmware/bsp/esp32-s3-epaper-1.54-v2/README.md)
- [板级自检应用](../firmware/apps/board_smoke_test/)
