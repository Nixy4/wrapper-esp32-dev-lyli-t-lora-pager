#pragma once

#include <string>

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include "ui/screen_manager.hpp"
#include "core/app_registry.hpp"
#include "core/sd_installer.hpp"

namespace launcher::ui
{

/**
 * @brief 显示固件安装进度的屏幕。
 *
 * 后台 FreeRTOS 任务执行实际刷写操作，
 * 而 LVGL 任务通过定时器轮询更新进度条和日志标签。
 *
 * 完成后屏幕自动返回两级（到应用列表）并触发注册表刷新。
 */
class ScreenInstallProgress
{
    ScreenManager& mgr_;
    core::AppRegistry& registry_;
    core::SdInstaller& installer_;
    std::string sd_path_;
    std::string display_name_;

    lv_obj_t* screen_ = nullptr;
    lv_obj_t* bar_ = nullptr;
    lv_obj_t* stage_lbl_ = nullptr;
    lv_obj_t* pct_lbl_ = nullptr;

    // FreeRTOS task handle
    TaskHandle_t task_handle_ = nullptr;
    bool finished_ = false;
    bool success_ = false;

    // Progress data shared between install task and LVGL task
    struct ProgressData
    {
        char stage[64] = {};
        size_t written = 0;
        size_t total = 0;
    };
    ProgressData prog_data_{};

    static void installTask(void* arg);
    static void onTimerTick(lv_timer_t* timer);

    void buildWidgets();
    void updateProgress(const char* stage, size_t written, size_t total);

   public:
    ScreenInstallProgress(ScreenManager& mgr,
                          core::AppRegistry& registry,
                          core::SdInstaller& installer,
                          const std::string& sd_path,
                          const std::string& display_name);

    lv_obj_t* screen() const { return screen_; }
};

/// Helper called by ScreenSdBrowser to push this screen.
void pushInstallProgress(ScreenManager& mgr,
                         core::AppRegistry& registry,
                         core::SdInstaller& installer,
                         const std::string& sd_path,
                         const std::string& display_name);

}  // namespace launcher::ui
