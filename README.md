# EcoStep 项目说明 (ESP32-S3 / ESP-IDF)

## 项目简介
EcoStep 是一款基于 ESP32-S3 的个人碳足迹监测设备，当前阶段以稳定、长期的数据采集为核心目标。系统会将 25Hz IMU 与 1Hz GPS 数据统一落盘到 SD 卡，方便后续训练与分析。

## 运行模式
- 数据采集模式 (Data Collection)：默认启动，仅写入 Raw Log，不自动生成 events/summary。
- Mock 模式：仅用于验证状态机逻辑，本阶段已关闭。

启动串口会看到：
```
MODE: DATA_COLLECTION (MOCK disabled)
DATALOG: raw logging enabled
```

## SD 卡日志结构
日志按天分目录保存：
```
/sdcard/YYYYMMDD/
  raw_HH.csv
  events.csv
  summary.csv
```

当前数据采集阶段主要写入 `raw_HH.csv`，events/summary 仅在上层逻辑主动调用时写入。

### Raw Log (raw_HH.csv)
- 按小时分片，例如 `raw_09.csv`
- Header:
```
datetime_local,uptime_ms,acc_x,acc_y,acc_z,gyro_x,gyro_y,gyro_z,speed_mps,turn_rate_deg_s,gps_valid
```

### Events Log (events.csv)
由 Traffic State Machine 生成（当前模式不自动写入）：
```
start_time,end_time,start_uptime_ms,end_uptime_ms,duration_sec,mode,avg_speed_mps
```

### Summary Log (summary.csv)
按天汇总（当前模式不自动写入），最后一行为 TOTAL 汇总：
```
mode,total_duration_min,carbon_factor_g_per_min,co2_g
```

## 构建与烧录
1. 进入工程目录：
```
cd S:\esp32\JofTmode scr change\hxc143
```
2. 构建：
```
idf.py build
```
3. 烧录：
```
idf.py -p PORT flash
```

如果你使用的是独立 ESP-IDF 工具链，请确保 `cmake`、`ninja` 与 xtensa 工具链已加入 PATH，或使用 IDF 的 `export` 脚本初始化环境。

## 数据采集注意事项
- 建议在正式采集前清理或备份 SD 卡旧数据，避免不同阶段数据混淆。
- 采集期间 `uptime_ms` 应持续单调递增（除非设备重启）。
- 目前不启动 mock，不应产生新的 events/summary 行。

## 组件说明
- `app_datalog`：SD 卡数据落盘（Raw/Event/Summary）。
- `app_logic`：交通状态机与 Mock 脚本（当前不启动）。
- `app_state`：IMU/GPS 数据中枢。
- `app_axis6` / `app_gps`：IMU / GPS 采集模块。
- `app_imu_calib`：IMU 校准与 NVS 保存。

## 开发者控制开关（可选）
为了方便单独校准和调试 IMU/GPS/写卡，提供了统一停止/恢复接口，且不依赖 menuconfig 或按键。
你可以在代码里直接调用：
`app_control_stop_all()` / `app_control_resume_all()` 或单独的 `app_control_stop_imu()`、`app_control_stop_gps()`、`app_control_stop_datalog()`。
也可以在 `main/main_app.c` 中把 `DEV_STOP_ALL_ON_BOOT` 改为 `1`，开机即停止 IMU/GPS/写卡，方便单独校准与测试。
如需恢复 Mock 逻辑测试，可在 `main/main_app.c` 中开启 `ENABLE_MOCK_TEST` 并调用 `app_logic_start()`。

## IMU 校准指令
说明：
- `imu_calib <seconds>`，例如 `imu_calib 40`
- `imu_calib_reset`

说明：
- 可用于单独停止 IMU/GPS/写卡进行校准。
- 校准后的 bias 写入 NVS 的 `imu_calib` 命名空间，运行时 IMU 使用 raw - bias。
- 校准数据也会写入 `/sdcard/imu_calib/calib.csv`。

## 功耗管理说明（当前行为）
- 仅关屏：30 秒无触摸与短按按键现在只关闭屏幕（不进入轻睡眠）。
- 唤醒行为：触摸或按键点亮屏幕；首次唤醒触摸会被消费，避免误触界面操作。
- 已开启上拉：KEY_GPIO 与 TOUCH_INT_GPIO 使用内部上拉，避免悬空为低。
- 禁用轻睡眠原因：轻睡眠时 USB 串口会断开，且该硬件出现“立即唤醒”。

### 如需再次尝试轻睡眠
- 在 `components/application/app_power/app_power.c` 中恢复轻睡眠逻辑（power_task 调用 `esp_light_sleep_start()` 并配置唤醒源）。
- 使用 UART 或其它非 USB 日志；轻睡眠时 USB CDC 会断开。
- 进入睡眠前确保 KEY/TOUCH 线保持高电平，避免立即唤醒。

## 近期变更摘要
- 启用 PSRAM 并将 TFLM Tensor Arena 放入 PSRAM，同时让 Wi‑Fi 和 LWIP 优先使用 PSRAM
- 新增 ML 周期任务与滑窗推理，pred 1Hz 写入，UI 直接显示 pred_smooth 数值
- SD 写入闭环完善，raw、pred、events、summary 文件生成，summary TOTAL 行仅保留总时长与总排放
- LVGL 全屏双缓冲与 DMA bounce 优化，触摸唤醒去抖
- 修复多次崩溃与写卡问题，修复 summary 栈溢出与 Wi‑Fi SSID 打印越界
- 删除 mock 测试逻辑，统一六类模式 Stationary Walk Bike Car Bus Subway
- Wi‑Fi 校时改为开机与按钮触发，30 秒超时自动关闭，并同步 UI 开关状态
- 震动仅在 Wi‑Fi 校时成功及切换到 Car Bus Subway 时触发
