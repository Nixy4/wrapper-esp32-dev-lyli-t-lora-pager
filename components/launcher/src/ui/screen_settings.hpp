#pragma once

#include "ui/screen_manager.hpp"
#include "core/app_registry.hpp"

namespace launcher::ui
{

/**
 * @brief Settings screen.
 *
 * Currently exposes:
 *   - Auto-boot toggle (saved as NVS key "auto_boot" in l_cfg namespace)
 *   - Backlight brightness slider 10–100%
 *
 * Navigation:
 *   NEXT / PREV  → move between settings items
 *   SELECT       → toggle / confirm
 *   BACK         → pop back to AppList
 */
class ScreenSettings
{
    ScreenManager& mgr_;
    core::AppRegistry& registry_;

    lv_obj_t* screen_ = nullptr;
    lv_obj_t* sw_autoboot_ = nullptr;
    lv_obj_t* slider_bl_ = nullptr;

    static void onBackClicked(lv_event_t* e);
    static void onAutoBootChanged(lv_event_t* e);
    static void onBrightnessChanged(lv_event_t* e);

    void buildWidgets();
    void loadSettings();
    void saveAutoboot(bool on);
    void saveBrightness(int pct);

   public:
    ScreenSettings(ScreenManager& mgr, core::AppRegistry& registry);
    lv_obj_t* screen() const { return screen_; }
    void handleInput(const hal::InputEvent& ev);
};

/// Helper called by ScreenAppList to push this screen.
void pushSettings(ScreenManager& mgr, core::AppRegistry& registry);

}  // namespace launcher::ui
