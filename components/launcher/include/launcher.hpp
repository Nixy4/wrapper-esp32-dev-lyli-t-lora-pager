#pragma once

#include <cstdint>

/**
 * @file launcher.hpp
 * @brief Public API for the Launcher component.
 *
 * The Launcher manages installed ESP32 applications:
 *   - Displays a list of installed apps and allows booting them
 *   - Browses the SD card for firmware binaries and installs them
 *   - Persists app metadata in NVS
 *   - Provides a settings screen (backlight, auto-boot, etc.)
 *
 * Architecture:
 *   ┌─────────────────────────────────────────┐
 *   │  launcher::start() / startTask()        │  ← Public API (this file)
 *   ├───────────────────────┬─────────────────┤
 *   │       UI Layer        │   Core Layer     │
 *   │  (LVGL screens)       │  (AppRegistry,  │
 *   │                       │   BootManager,  │
 *   │                       │   SdInstaller)  │
 *   ├───────────────────────┴─────────────────┤
 *   │  HAL Interfaces  │  OSAL Interfaces     │
 *   │  i_display.hpp   │  i_time.hpp          │
 *   │  i_input.hpp     │  i_mutex.hpp         │
 *   │  i_storage.hpp   │                      │
 *   │  i_partition.hpp │                      │
 *   ├──────────────────┴──────────────────────┤
 *   │  HAL ESP32 Impl  │  OSAL FreeRTOS Impl  │
 *   │  (wrapper-esp32  │  (wrapper::Task,     │
 *   │   board/device)  │   wrapper::Delay)    │
 *   └─────────────────────────────────────────┘
 *
 * Usage:
 * @code
 *   // In app_main():
 *   launcher::Config cfg;
 *   cfg.boot_to_last_app = true;
 *   launcher::startTask(cfg);   // returns immediately
 *   // ... or:
 *   launcher::start(cfg);       // blocks forever
 * @endcode
 */

namespace launcher
{

/**
 * @brief Launcher configuration.
 */
struct Config
{
    /// If true and a last-booted app is recorded in NVS, boot it on start.
    bool boot_to_last_app = true;

    /// Display brightness 0–100 % (applied on init).
    int brightness = 70;

    /// FreeRTOS stack size (bytes) for the input-polling task.
    uint32_t input_task_stack = 4096;

    /// FreeRTOS priority for the input-polling task.
    uint8_t input_task_prio = 4;
};

/**
 * @brief Start the Launcher in the calling task context.
 *
 * Initialises board hardware (if not already done), wires up all
 * HAL/OSAL/Core layers, shows the initial screen, and then blocks
 * indefinitely running the event loop.
 *
 * @note Never returns under normal operation.  The device reboots
 *       whenever the user selects an app to boot.
 */
void start(const Config& cfg = {});

/**
 * @brief Spawn a FreeRTOS task that runs the Launcher.
 *
 * Returns immediately; the Launcher runs in the background.
 *
 * @param cfg          Launcher configuration.
 * @param stack_depth  FreeRTOS task stack in bytes (default: 16 KiB).
 * @param priority     FreeRTOS task priority (default: 5).
 */
void startTask(const Config& cfg = {}, uint32_t stack_depth = 16384, uint8_t priority = 5);

}  // namespace launcher
