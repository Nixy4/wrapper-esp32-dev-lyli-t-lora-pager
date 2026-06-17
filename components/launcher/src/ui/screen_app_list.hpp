#pragma once

#include <vector>

#include "ui/screen_manager.hpp"
#include "core/app_registry.hpp"
#include "core/boot_manager.hpp"

namespace launcher::ui
{

/**
 * @brief The main Launcher screen: shows the list of installed apps.
 *
 * Layout (480 × 222 landscape):
 *   ┌──────────────────────────────────────────────────────┐
 *   │  Launcher              [SD Install]  [Settings]  ... │  title bar
 *   ├──────────────────────────────────────────────────────┤
 *   │  ▶ My App 1                                          │
 *   │    My App 2                                          │  scrollable list
 *   │    My App 3                                          │
 *   │                                                      │
 *   └──────────────────────────────────────────────────────┘
 *   Status: "3 apps installed"
 *
 * Navigation:
 *   NEXT / PREV  → move list selection
 *   SELECT       → confirm boot (shows confirmation dialog)
 *   BACK (long)  → no-op (already at root)
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

    // ── LVGL event callbacks ──────────────────────────────────────────────────
    static void onListItemClicked(lv_event_t* e);
    static void onSdInstallClicked(lv_event_t* e);
    static void onSettingsClicked(lv_event_t* e);

    void buildWidgets();
    void refreshList();
    void updateStatus();
    void confirmBoot(int idx);

   public:
    ScreenAppList(ScreenManager& mgr, core::AppRegistry& registry, core::BootManager& boot_mgr);

    /// @return The underlying LVGL screen object (to be passed to ScreenManager::push).
    lv_obj_t* screen() const { return screen_; }

    /// Handle a keyboard navigation event.
    void handleInput(const hal::InputEvent& ev);
};

}  // namespace launcher::ui
