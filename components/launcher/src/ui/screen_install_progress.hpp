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
 * @brief Screen that shows firmware installation progress.
 *
 * A background FreeRTOS task performs the actual flash write while
 * the LVGL task updates the progress bar and log label.
 *
 * On completion the screen automatically pops back two levels
 * (to AppList) and triggers a registry refresh.
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
