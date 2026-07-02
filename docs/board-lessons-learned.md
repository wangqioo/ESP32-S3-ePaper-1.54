# ESP32-S3-ePaper-1.54 踩坑记录

本文记录 `factory_program_custom` 语音备忘录应用开发过程中已经验证过的板级行为和固件坑点。后续新增功能时，优先按这里的结论检查，不要再从官方例程零散推断。

## 硬件版本和目标程序

- 当前目标板是 V2：ESP32-S3-PICO-1-N8R8，8MB Flash，8MB PSRAM。
- 当前应用目录：`firmware/apps/factory_program_custom`。
- 固件已经使用 8MB Flash 分区表，`storage` FAT 分区挂在 `/flash`，用于没有 SD 卡时保存录音。
- 连接电脑时可能同时出现两个串口设备，烧录前必须确认目标设备，避免刷到另一个板子。

## 电源和按键

关键 GPIO：

- `GPIO17`：VBAT 电源保持，高电平保持上电，低电平释放电池电源。
- `GPIO18`：PWR 按键，低电平表示按下。
- `GPIO6`：墨水屏电源使能，低电平开启，高电平关闭。
- `GPIO42`：音频电源使能，低电平开启，高电平关闭。

已经踩过的坑：

- PWR 用来开机时，用户会一直按住按键。如果固件启动后立刻响应 PWR 长按，设备会刚开机又关机。
- 官方 `07_BATT_PWR_Test` 里的 `is_vabtflag` 本质上是“先等 PWR 松开，再允许关机”的释放门控。我们自己的 `power_button_released` 也必须保留。
- 充电或 USB 连接时，`BoardPower_VBAT_OFF()` 只能释放电池保持，不一定能让 ESP32 真正断电。表现是“能关机，但插着充电线时按 PWR 开不了机”。
- 解决方式是关机画面显示后执行 `BoardPower_VBAT_OFF()`，然后等待 PWR 松开，再进入 deep sleep，并用 `GPIO18` 低电平作为唤醒源。
- 不能在 PWR 仍按下时立刻进入 `ext0` 唤醒，否则可能马上被同一次按压唤醒。

实现位置：

- 关机主逻辑：`components/app/user_app.cpp` 的 `Shutdown()`。
- 可测试策略：`components/app/power_sleep_policy.cpp`。
- 板级电源极性：`components/port_bsp/port_power.cpp`。

## 录音和播放

当前 WAV 格式：

- 采样率：16 kHz。
- 位宽：16 bit。
- 声道：2 channels。
- PCM 数据速率：`16000 * 2 * 2 = 64000 B/s`，约 64 KB/s。

已经踩过的坑：

- 录音不能依赖 UI 主循环持续读 I2S。墨水屏刷新、文件系统写入、LVGL 锁等待都会让音频读取不稳定。
- 录音时必须由独立任务连续调用 `Codec_RecordData()`，先写入 PSRAM 缓冲。
- `MemoRecorder::IsRecording()` 的含义是“仍持有待保存的录音缓冲”，停止采集后保存完成前也会返回 true。UI 逻辑不能只靠它判断“正在采集”，所以增加了 `recording_worker_active` 区分采集阶段和保存阶段。
- 播放变快的问题来自格式和帧边界处理不严。读取 WAV 后写给 codec 的数据必须按完整音频帧对齐，当前播放前会按 `channels * bits_per_sample / 8` 截掉不完整尾帧。
- 录音和播放的 WAV header 必须完全匹配当前格式，否则时长计算、扫描和播放都会出问题。

实现位置：

- 录音：`components/app/memo_recorder.cpp`。
- 播放：`components/app/memo_player.cpp`。
- WAV 格式和工具函数：`components/app/memo_types.h`、`components/app/memo_utils.cpp`。
- 调度刷新策略：`components/app/audio_loop_policy.cpp`。

## 存储策略

当前策略：

- 优先使用 SD 卡。
- SD 不可用时使用片上 Flash FAT 分区 `/flash`。
- 录音阶段先写 PSRAM，松开 BOOT 后再一次性保存 WAV。

已经踩过的坑：

- 用户的 SD 卡曾经因为分区太多导致板子识别不到。即使 SD 卡插入且看起来格式化正确，也要确认是设备能识别的 FAT32 分区布局。
- SD 卡初始化失败时不能让应用不可用，必须自动回退到 Flash。
- Flash FAT 可用空间有限，并且写入速度接近音频码率，边录边写 Flash 容易造成音频 underrun。
- 实测 PSRAM 写读速度约 14-15 MB/s，`memcpy` 约 20 MB/s，不是瓶颈。
- 实测 Flash 保存一次约 217 KB WAV 曾耗时约 4.7 秒，所以保存界面必须明确显示“正在写入”，否则用户会以为卡死。
- 保存时不要继续显示录音计时。录音已经结束，保存阶段只显示旋转/等待状态，保存完成后直接打开刚录好的详情页。

实现位置：

- 存储扫描和文件名：`components/app/memo_storage.cpp`。
- Flash FAT 挂载：`components/port_bsp/port_flash_storage.cpp`。
- 分区表：`firmware/apps/factory_program_custom/partitions.csv`。

## 保存阶段的调度

已经踩过的坑：

- 最早的关机路径在 App 任务里等待 `app_state == Saving` 结束，但保存完成事件也要靠 App 任务处理。这会造成自等待死锁，尤其是“录音中长按 PWR 关机”场景。
- 现在保存任务会设置独立的 `BIT_SAVE_WORKER_DONE`，关机路径等待这个 worker bit，而不是等待 App 任务自己处理 UI 状态。
- 每次启动保存前必须清掉 `BIT_SAVE_WORKER_DONE`，否则可能读到上一次保存留下的完成标志。

实现位置：

- 保存任务：`components/app/user_app.cpp` 的 `Save_LoopTask()`。
- 停止录音并开始保存：`StopRecording()`。
- 关机等待保存：`Shutdown()`。

## 墨水屏和触摸

已经踩过的坑：

- 墨水屏刷新慢，不能当作实时动画屏使用。录音时可以每秒刷新计时，但不要高频刷新。
- 保存阶段的旋转动画也应低频刷新，主要目的是告诉用户系统没有卡住。
- 列表翻页一开始很难滑动，是因为滑动手势被误判成点击录音文件。
- 触摸事件必须区分拖动距离和点击抖动：超过 tap slop 就不能再当作点击；列表页竖向位移超过翻页阈值才翻页。
- 录音或保存中应该忽略触摸操作，防止误触改变状态。

实现位置：

- UI 绘制：`components/app/voice_ui.cpp`。
- 手势分类：`components/app/touch_gesture.cpp`。
- UI 事件处理：`components/app/user_app.cpp` 的 `HandleUiAction()` 和主循环。

## 官方例程参考结论

- 电源极性直接参考官方 BATT/PWR 例程：VBAT 保持是 `GPIO17=1`，释放是 `GPIO17=0`。
- 官方例程的“先松开 PWR 再允许关机”逻辑是必要的，不是多余状态。
- 官方 BATT/PWR 例程只验证电池供电锁存，不覆盖 USB/充电仍给 ESP32 供电的完整关机场景，因此我们的 deep sleep 补充是必要的。
- 官方按钮 BSP 里有疑似复制粘贴问题：`button2` 初始化前出现过对 `button2` 的回调绑定。当前应用没有直接使用那套 multi_button 逻辑，后续如果迁移要重新检查。

## 后续改动前检查清单

- 改录音链路前，先确认 WAV 格式、I2S/codec 配置、`BytesToDurationSeconds()`、播放帧对齐四者一致。
- 改存储前，确认 SD 失败时 Flash fallback 仍可用，且 `/flash` 分区没有被新分区表挤掉。
- 改保存流程前，确认录音采集任务、保存任务、App UI 任务不会互相等待死锁。
- 改电源前，确认 PWR 释放门控和 USB/充电 deep sleep 唤醒仍存在。
- 改触摸前，确认滑动不会被当作点击，录音/保存阶段不会响应列表点击。
- 烧录前确认串口是当前目标板，不要误刷另一个设备。

## 常用验证

文档改动不需要重新构建固件。涉及代码时至少跑：

```sh
mkdir -p /tmp/esp-idf-host-include
touch /tmp/esp-idf-host-include/sdkconfig.h
c++ -std=c++17 \
  firmware/apps/factory_program_custom/components/app/test/test_memo_utils.cpp \
  firmware/apps/factory_program_custom/components/app/memo_utils.cpp \
  firmware/apps/factory_program_custom/components/app/memo_storage.cpp \
  firmware/apps/factory_program_custom/components/app/touch_gesture.cpp \
  firmware/apps/factory_program_custom/components/app/audio_loop_policy.cpp \
  firmware/apps/factory_program_custom/components/app/power_sleep_policy.cpp \
  -I /tmp/esp-idf-host-include \
  -I firmware/apps/factory_program_custom/components/app \
  -I /Users/wq/esp-idf/components/esp_common/include \
  -o /tmp/test_memo_utils && /tmp/test_memo_utils
```

```sh
. /Users/wq/esp-idf/export.sh >/tmp/esp-idf-export.log
idf.py build
idf.py -p /dev/cu.usbmodem11101 flash
idf.py -p /dev/cu.usbmodem11101 monitor
```
