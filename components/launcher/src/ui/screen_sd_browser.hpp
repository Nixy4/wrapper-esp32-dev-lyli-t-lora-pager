#pragma once

#include <string>
#include <vector>

#include "ui/screen_manager.hpp"
#include "core/app_registry.hpp"
#include "core/sd_installer.hpp"

namespace launcher::ui
{

/**
 * @brief 列出 SD 卡上 .bin 文件的屏幕，允许用户选择一个进行安装。
 *
 * 布局：
 *   ┌──────────────────────────────────────────────────────┐
 *   │  [← Back]   SD Card — select firmware to install    │
 *   ├──────────────────────────────────────────────────────┤
 *   │  myapp.bin                                           │
 *   │  anotherapp.bin                                      │
 *   │  (未找到 .bin 文件)                               │
 *   └──────────────────────────────────────────────────────┘
 *
 * 导航操作：
 *   NEXT / PREV  → 移动列表选中项
 *   SELECT       → 开始安装所选文件
 *   BACK         → 返回应用列表
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

    static void OnItemClicked(lv_event_t* e);
    static void OnBackClicked(lv_event_t* e);

    void BuildWidgets();
    void RefreshList();
    void StartInstall(int idx);

   public:
    ScreenSdBrowser(ScreenManager& mgr, core::AppRegistry& registry, core::SdInstaller& installer);

    lv_obj_t* Screen() const { return screen_; }
    void HandleInput(const hal::InputEvent& ev);

    /// 填充文件列表（构造完成后、push 之前调用）。
    void SetFiles(std::vector<std::string> files) { files_ = std::move(files); }
};

/// 由 ScreenAppList 调用的辅助函数，避免循环包含。
void PushSdBrowser(ScreenManager& mgr, core::AppRegistry& registry);

}  // namespace launcher::ui
