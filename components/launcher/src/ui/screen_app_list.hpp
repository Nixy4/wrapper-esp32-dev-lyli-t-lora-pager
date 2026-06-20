#pragma once

#include <vector>

#include "ui/screen_manager.hpp"
#include "core/app_registry.hpp"
#include "core/boot_manager.hpp"

namespace launcher::ui
{

/**
 * @brief Launcher 主屏幕：显示已安装的应用列表。
 *
 * 布局（480 × 222 横屏）：
 *   ┌──────────────────────────────────────────────────────┐
 *   │  Launcher              [SD Install]  [Settings]  ... │  标题栏
 *   ├──────────────────────────────────────────────────────┤
 *   │  ▶ My App 1                                          │
 *   │    My App 2                                          │  可滚动列表
 *   │    My App 3                                          │
 *   │                                                      │
 *   └──────────────────────────────────────────────────────┘
 *   状态栏：“3 apps installed”
 *
 * 导航操作：
 *   NEXT / PREV  → 移动列表选中项
 *   SELECT       → 确认引导（显示确认对话框）
 *   BACK (长按) → 无操作（已在根屏幕）
 */
class ScreenAppList
{
    ScreenManager& mgr_;
    core::AppRegistry& registry_;
    core::BootManager& boot_mgr_;

    lv_obj_t* screen_ = nullptr;
    lv_obj_t* list_ = nullptr;
    lv_obj_t* status_lbl_ = nullptr;

    std::vector<core::AppInfo> apps_;
    int selected_idx_ = 0;

    // ─── LVGL 事件回调 ────────────────────────────────────────────────────────────────────────────────────────────────────────────
    static void OnListItemClicked(lv_event_t* e);
    static void OnSdInstallClicked(lv_event_t* e);
    static void OnSettingsClicked(lv_event_t* e);

    void BuildWidgets();
    void RefreshList();
    void UpdateStatus();
    void ConfirmBoot(int idx);

   public:
    ScreenAppList(ScreenManager& mgr, core::AppRegistry& registry, core::BootManager& boot_mgr);

    /// @return 底层 LVGL 屏幕对象（传入 ScreenManager::push）。
    lv_obj_t* Screen() const { return screen_; }

    /// Handle a keyboard navigation event.
    void HandleInput(const hal::InputEvent& ev);
};

}  // namespace launcher::ui
