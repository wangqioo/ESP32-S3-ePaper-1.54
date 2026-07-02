# ESP32-S3 ePaper 1.54 AIoT 开发板

本仓库用于存放 ESP32-S3 ePaper 1.54 AIoT 开发板相关资料、固件源码、硬件文档与开发工具。

## 产品简介

本产品是一款墨水屏 AIoT 开发板，搭载 ESP32-S3 微控制器，支持 Wi-Fi 与 BLE 双模通信。板载 1.54 英寸电子墨水屏，功耗低，阳光下可视，适合便携设备及长续航场景。

开发板集成 RTC 实时时钟、SHTC3 温湿度传感器、Micro SD 卡槽、低功耗音频编解码芯片电路以及锂电池充放电管理电路。预留 USB、UART、I2C 及 GPIO 等扩展接口，便于功能拓展与传感器连接，可用于物联网终端、电子标签和便携显示器等应用。

## 版本注意事项

从 2025-11-01 起，产品开始替换为 V2 版本。产品背面贴有 `V2` 标识，或 PCB 左上角有 `V2` 丝印的，为 V2 版本；没有这些标识的为 V1 版本。

V1 与 V2 版本的例程不通用，导入官方程序时需要先确认硬件版本。

| 版本 | 主控与存储 | 说明 |
| --- | --- | --- |
| V1 | ESP32-S3FH4R2，内置 4MB Flash 和 2MB PSRAM | 旧版本硬件 |
| V2 | ESP32-S3-PICO-1-N8R8，内置 8MB Flash 和 8MB PSRAM | 新版本硬件，同时优化睡眠模式下整板功耗 |

## SKU

| SKU | 产品 |
| --- | --- |
| 32298 | ESP32-S3-ePaper-1.54 |
| 32299 | ESP32-S3-ePaper-1.54-EN |
| 34211 | ESP32-S3-Touch-ePaper-1.54 |
| 34212 | ESP32-S3-Touch-ePaper-1.54-EN |

## 产品特性

- 搭载高性能 Xtensa 32 位 LX7 双核处理器，主频高达 240MHz
- 支持 2.4GHz Wi-Fi 和 Bluetooth 5 (LE)，板载内部天线
- 内置 512KB SRAM 和 384KB ROM，同时叠封集成 Flash 与 PSRAM
- 搭载 1.54 英寸电子墨水屏，分辨率 200 x 200，具备高对比度、宽视角等特性
- 板载音频编解码芯片，支持语音采集与播放，便于实现 AI 语音交互应用
- 板载 PCF85063 RTC 实时时钟与 SHTC3 温湿度传感器，可实现精准 RTC 时间管理及温湿度监测
- 板载 Micro SD 卡槽，可外接 Micro SD 卡存储图片或文件
- 板载 PWR、BOOT 两个可自定义功能的侧边按钮，方便使用按钮进行自定义功能开发
- 预留 2 x 6 2.54mm 间距的排母接口，方便客户外接扩展使用

## 硬件资源

| 资源 | 说明 |
| --- | --- |
| ESP32-S3-PICO-1-N8R8 | Wi-Fi 和蓝牙 SoC，240MHz 运行频率，叠封集成 8MB Flash 与 8MB PSRAM |
| Micro SD 卡槽 | 使用时需要将 SD 卡格式化为 FAT32 |
| ES8311 音频编解码芯片 | 支持音频输入与输出，低功耗设计，适合语音识别与语音播放应用 |
| BOOT 按键 | 按住 BOOT 后重新上电可进入下载模式 |
| PWR 按键 | 配合程序可实现锂电池供电情况下的电源控制 |
| Type-C 接口 | ESP32-S3 USB 接口，可用于烧录程序和日志打印 |
| 麦克风 | 采集音频 |
| SHTC3 温湿度传感器 | 提供环境温湿度测量，便于实现环境监测功能 |
| MX1.25 2PIN 扬声器接口 | 音频输出信号，外接扬声器 |
| MX1.25 2PIN 锂电池接口 | 用于连接锂电池 |
| 板载贴片天线 | 支持 2.4GHz Wi-Fi (802.11 b/g/n) 和 Bluetooth 5 (LE) |
| PCF85063 RTC | RTC 时钟芯片，位于背面，支持时间保持功能 |
| 2 x 6PIN 2.54mm 间距排母 | 用于扩展使用 |
| 扬声器 | 播放音频 |

## 项目入口

本仓库当前不是单一固件工程，而是一个板级开发仓库。官方例程、抽取出的 BSP、实验应用和完整应用都放在同一个仓库中，方便互相参考。

### 官方资料与例程

- [firmware/official/v2/esp-idf](firmware/official/v2/esp-idf/): 官方 ESP-IDF V2 例程，保持原始结构，作为硬件行为参考。
- [docs/official-overview.md](docs/official-overview.md): 官方资料整理。
- [hardware/pinout.md](hardware/pinout.md): GPIO、e-Paper、SD、I2S、UART、RTC 等引脚分配。

### BSP

- [firmware/bsp/esp32-s3-epaper-1.54-v2](firmware/bsp/esp32-s3-epaper-1.54-v2/): 从官方 V2 例程中抽取的可复用 BSP 组件。

### 应用程序

| 应用 | 路径 | 说明 |
| --- | --- | --- |
| 板级自检 | [firmware/apps/board_smoke_test](firmware/apps/board_smoke_test/) | 用于快速验证屏幕、传感器、电源等基础硬件是否可用 |
| 语音备忘录 | [firmware/apps/factory_program_custom](firmware/apps/factory_program_custom/) | 基于官方出厂综合程序改造，支持录音、播放、删除、收藏、Flash/SD 存储、触摸列表、按键声效、快速录音等功能 |
| 桌面天气/时钟 | [firmware/apps/desktop_helper](firmware/apps/desktop_helper/) | 显示上海天气、RTC 时间、室内温湿度和电量；短按 PWR 切换天气/时钟面板，BOOT 手动刷新，长按 PWR 休眠 |
| AI 语音小助手 | [firmware/apps/ai_voice_assistant](firmware/apps/ai_voice_assistant/) | 按住 BOOT 录音，松手后上传到本机代理服务，并在墨水屏显示 AI 回复；默认支持 mock 代理，智谱 GLM Key 只放在 Mac 代理环境变量中 |

### 开发记录

- [docs/board-lessons-learned.md](docs/board-lessons-learned.md): 开发过程中踩过的硬件、音频、触摸、睡眠、存储等问题记录。
- `docs/superpowers/specs/`: 功能设计文档。
- `docs/superpowers/plans/`: 实施计划文档。

## 仓库结构

```text
.
├── docs/
│   ├── board-lessons-learned.md
│   ├── official-overview.md
│   └── superpowers/
│       ├── plans/
│       └── specs/
├── firmware/
│   ├── apps/
│   │   ├── ai_voice_assistant/
│   │   ├── board_smoke_test/
│   │   ├── desktop_helper/
│   │   └── factory_program_custom/
│   ├── bsp/
│   │   └── esp32-s3-epaper-1.54-v2/
│   └── official/
│       └── v2/esp-idf/
├── hardware/
│   └── pinout.md
└── tools/
```

## 常用构建命令

需要先安装并导出 ESP-IDF 环境。以下命令均在仓库根目录执行。

构建语音备忘录：

```bash
cd firmware/apps/factory_program_custom
idf.py build
```

构建桌面天气/时钟：

```bash
cd firmware/apps/desktop_helper
idf.py build
```

构建 AI 语音小助手：

```bash
cd firmware/apps/ai_voice_assistant
idf.py build
```

烧录时请先确认串口对应的设备，避免在同时连接两块板子时刷错目标板：

```bash
idf.py -p /dev/cu.usbmodem11101 flash monitor
```

## 当前注意事项

- 本仓库当前主要面向 V2 硬件。
- `firmware/official/v2/esp-idf/` 保持官方例程原样，作为参考，不直接在其中开发新应用。
- `firmware/apps/factory_program_custom/` 是语音备忘录主应用，来自官方出厂综合程序的定制版本。
- `firmware/apps/desktop_helper/` 是桌面天气/时钟应用，当前天气面板不会自动深睡，只是降低刷新频率；长按 PWR 才进入休眠/关机路径。
- `firmware/apps/ai_voice_assistant/` 是 AI 语音小助手应用；公开仓库默认不包含 Wi-Fi、代理 IP 或 GLM API Key。
- `firmware/apps/desktop_helper/components/app/desktop_config.h` 中的 Wi-Fi SSID/密码默认留空；本地使用前需要自行填写，公开提交前不要提交私人网络凭据。
- 构建产生的 `build/` 和 `sdkconfig` 不应提交，保留 `sdkconfig.defaults` 作为项目配置基线。
