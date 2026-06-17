#pragma once

#include <string>
#include <vector>

#include "ui/screen_manager.hpp"
#include "core/app_registry.hpp"
#include "core/sd_installer.hpp"

namespace launcher::ui
{

/**
 * @brief Screen that lists .bin files on the SD card and lets the user
 *        select one to install.
 *
 * Layout:
 *   ┌──────────────────────────────────────────────────────┐
 *   │  [← Back]   SD Card — select firmware to install    │
 *   ├──────────────────────────────────────────────────────┤
 *   │  myapp.bin                                           │
 *   │  anotherapp.bin                                      │
 *   │  (No .bin files found)                               │
 *   └──────────────────────────────────────────────────────┘
 *
 * Navigation:
 *   NEXT / PREV  → move list selection
 *   SELECT       → begin install of selected file
 *   BACK         → pop back to AppList
 */
class ScreenSdBrowser
{
    ScreenManager& mgr_;
    core::AppRegistry& registry_;
    core::SdInstaller& installer_;

    lv_obj_t* screen_ = nullptr;
    lv_obj_t* list_ = nullptr;
    lv_obj_t* status_lbl_ = nullptr;

    std::vector<std::string> files_;
    int selected_idx_ = 0;

    static void onItemClicked(lv_event_t* e);
    static void onBackClicked(lv_event_t* e);

    void buildWidgets();
    void refreshList();
    void startInstall(int idx);

   public:
    ScreenSdBrowser(ScreenManager& mgr, core::AppRegistry& registry, core::SdInstaller& installer);

    lv_obj_t* screen() const { return screen_; }
    void handleInput(const hal::InputEvent& ev);

    /// Populate the file list (call after construction, before push).
    void setFiles(std::vector<std::string> files) { files_ = std::move(files); }
};

/// Helper called by ScreenAppList to push this screen without a circular include.
void pushSdBrowser(ScreenManager& mgr, core::AppRegistry& registry);

}  // namespace launcher::ui
