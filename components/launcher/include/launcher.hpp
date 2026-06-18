#pragma once

#include <cstdint>

/**
 * @file launcher.hpp
 * @brief Launcher 组件的公共 API。
 *
 * Launcher 负责管理已安装的 ESP32 应用程序：
 *   - 显示已安装应用列表，并允许引导启动
 *   - 浏览 SD 卡中的固件二进制文件并安装
 *   - 将应用元数据持久化到 NVS
 *   - 提供设置界面（背光、自动启动等）
 *
 * 架构：
 *   ┌─────────────────────────────────────────┐
 *   │  launcher::start() / startTask()        │  ← 公共 API（本文件）
 *   ├───────────────────────┬─────────────────┤
 *   │       UI 层           │   Core 层        │
 *   │  (LVGL 界面)          │  (AppRegistry,  │
 *   │                       │   BootManager,  │
 *   │                       │   SdInstaller)  │
 *   ├───────────────────────┴─────────────────┤
 *   │  HAL 接口层      │  OSAL 接口层          │
 *   │  i_display.hpp   │  i_time.hpp          │
 *   │  i_input.hpp     │  i_mutex.hpp         │
 *   │  i_storage.hpp   │                      │
 *   │  i_partition.hpp │                      │
 *   ├──────────────────┴──────────────────────┤
 *   │  HAL ESP32 实现  │  OSAL FreeRTOS 实现   │
 *   │  (wrapper-esp32  │  (wrapper::Task,     │
 *   │   board/device)  │   wrapper::Delay)    │
 *   └─────────────────────────────────────────┘
 *
 * 使用方式：
 * @code
 *   // 在 app_main() 中：
 *   launcher::Config cfg;
 *   cfg.boot_to_last_app = true;
 *   launcher::startTask(cfg);   // 立即返回
 *   // ... 或者：
 *   launcher::start(cfg);       // 永久阻塞
 * @endcode
 */

namespace launcher
{

/**
 * @brief Launcher 配置参数。
 */
struct Config
{
    /// 若为 true，且 NVS 中记录了上次启动的应用，则启动时自动引导该应用。
    bool boot_to_last_app = true;

    /// 显示亮度 0–100%（初始化时应用）。
    int brightness = 70;

    /// 输入轮询任务的 FreeRTOS 栈大小（字节）。
    uint32_t input_task_stack = 4096;

    /// 输入轮询任务的 FreeRTOS 优先级。
    uint8_t input_task_prio = 4;
};

/**
 * @brief 在调用任务的上下文中启动 Launcher。
 *
 * 初始化板级硬件（如果尚未完成），连接所有 HAL/OSAL/Core 层，
 * 显示初始界面，然后永久阻塞运行事件循环。
 *
 * @note 正常情况下永不返回。用户选择应用启动时设备将重启。
 */
void start(const Config& cfg = {});

/**
 * @brief 创建一个运行 Launcher 的 FreeRTOS 任务。
 *
 * 立即返回，Launcher 在后台运行。
 *
 * @param cfg          Launcher 配置参数。
 * @param stack_depth  FreeRTOS 任务栈大小（字节，默认 16 KiB）。
 * @param priority     FreeRTOS 任务优先级（默认 5）。
 */
void startTask(const Config& cfg = {}, uint32_t stack_depth = 16384, uint8_t priority = 5);

}  // namespace launcher
