#pragma once

#include "ui/screen_manager.hpp"
#include "core/app_registry.hpp"

namespace launcher::ui
{

/**
 * @brief 设置屏幕。
 *
 * 当前暴露的设置项：
 *   - 自动引导开关（保存为 NVS 键 "auto_boot"，位于 l_cfg 命名空间）
 *   - 背光亮度滑动条 10–100%
 *
 * 导航操作：
 *   NEXT / PREV  → 在设置项之间移动
 *   SELECT       → 切换/确认
 *   BACK         → 返回应用列表
 */
class ScreenSettings
{
    ScreenManager& mgr_;
    core::AppRegistry& registry_;

    lv_obj_t* screen_ = nullptr;
    lv_obj_t* sw_autoboot_ = nullptr;
    lv_obj_t* slider_bl_ = nullptr;

    static void OnBackClicked(lv_event_t* e);
    static void OnAutoBootChanged(lv_event_t* e);
    static void OnBrightnessChanged(lv_event_t* e);

    void BuildWidgets();
    void LoadSettings();
    void SaveAutoboot(bool on);
    void SaveBrightness(int pct);

   public:
    ScreenSettings(ScreenManager& mgr, core::AppRegistry& registry);
    lv_obj_t* Screen() const { return screen_; }
    void HandleInput(const hal::InputEvent& ev);
};

/// 由 ScreenAppList 调用的辅助函数，将此屏幕压入栈。
void PushSettings(ScreenManager& mgr, core::AppRegistry& registry);

}  // namespace launcher::ui
