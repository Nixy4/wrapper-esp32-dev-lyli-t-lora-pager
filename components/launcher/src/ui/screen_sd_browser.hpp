#pragma once

#include <string>
#include <vector>
#include <cstring>
#include <esp_log.h>

#include "lvgl.h"
#include "hal/input_base.hpp"
#include "ui/screen_manager.hpp"
#include "ui/screen_install_progress.hpp"
#include "core/app_registry.hpp"
#include "core/sd_installer.hpp"

namespace launcher::ui
{

/**
 * @brief 列出 SD 卡上 .bin 文件的屏幕，允许用户选择一个进行安装。（CRTP 模板版本）
 *
 * 布局：
 *   ┌──────────────────────────────────────────────────────┐
 *   │  [← Back]   SD Card — select firmware to install    │
 *   ├──────────────────────────────────────────────────────┤
 *   │  myapp.bin                                           │
 *   │  anotherapp.bin                                      │
 *   └──────────────────────────────────────────────────────┘
 *
 * 导航操作：
 *   NEXT / PREV  → 移动列表选中项
 *   SELECT       → 开始安装所选文件
 *   BACK         → 返回应用列表
 */
template <typename ScreenMgrT, typename RegistryT, typename InstallerT>
class ScreenSdBrowser
{
    ScreenMgrT& mgr_;
    RegistryT& registry_;
    InstallerT& installer_;

    lv_obj_t* screen_ = nullptr;
    lv_obj_t* list_ = nullptr;
    lv_obj_t* status_lbl_ = nullptr;

    std::vector<std::string> files_;
    int selected_idx_ = 0;

    static constexpr const char* kTag = "Launcher|SdBrowser";

    struct SdBrowserCtx
    {
        ScreenSdBrowser* self;
        int idx;
    };

    static void OnItemClicked(lv_event_t* e)
    {
        auto* ctx = static_cast<SdBrowserCtx*>(lv_event_get_user_data(e));
        if (ctx && lv_event_get_code(e) == LV_EVENT_CLICKED)
            ctx->self->StartInstall(ctx->idx);
    }

    static void OnBackClicked(lv_event_t* e)
    {
        if (lv_event_get_code(e) != LV_EVENT_CLICKED)
            return;
        auto* self = static_cast<ScreenSdBrowser*>(lv_event_get_user_data(e));
        if (self)
            self->mgr_.Pop();
    }

    void BuildWidgets()
    {
        auto& disp = mgr_.Display();
        const int W = disp.Width();

        // ── 标题栏
        lv_obj_t* bar = lv_obj_create(screen_);
        lv_obj_set_size(bar, W, 30);
        lv_obj_align(bar, LV_ALIGN_TOP_MID, 0, 0);
        lv_obj_set_style_bg_color(bar, lv_color_hex(0x16213E), 0);
        lv_obj_set_style_border_width(bar, 0, 0);
        lv_obj_set_style_radius(bar, 0, 0);
        lv_obj_set_style_pad_all(bar, 4, 0);

        lv_obj_t* back_btn = lv_button_create(bar);
        lv_obj_set_size(back_btn, 60, 22);
        lv_obj_align(back_btn, LV_ALIGN_LEFT_MID, 0, 0);
        lv_obj_set_style_bg_color(back_btn, lv_color_hex(0x0F3460), 0);
        lv_obj_t* back_lbl = lv_label_create(back_btn);
        lv_label_set_text(back_lbl, LV_SYMBOL_LEFT " Back");
        lv_obj_set_style_text_font(back_lbl, &lv_font_montserrat_14, 0);
        lv_obj_center(back_lbl);
        lv_obj_add_event_cb(back_btn, OnBackClicked, LV_EVENT_CLICKED, this);

        lv_obj_t* title = lv_label_create(bar);
        lv_label_set_text(title, "SD Card \xe2\x80\x94 select .bin to install");
        lv_obj_set_style_text_font(title, &lv_font_montserrat_14, 0);
        lv_obj_set_style_text_color(title, lv_color_hex(0xE2E2E2), 0);
        lv_obj_align(title, LV_ALIGN_CENTER, 20, 0);

        list_ = lv_list_create(screen_);
        lv_obj_set_size(list_, W, disp.Height() - 50);
        lv_obj_align(list_, LV_ALIGN_TOP_MID, 0, 32);
        lv_obj_set_style_bg_color(list_, lv_color_hex(0x1A1A2E), 0);
        lv_obj_set_style_border_width(list_, 0, 0);
        lv_obj_set_style_radius(list_, 0, 0);

        status_lbl_ = lv_label_create(screen_);
        lv_obj_set_style_text_font(status_lbl_, &lv_font_montserrat_14, 0);
        lv_obj_set_style_text_color(status_lbl_, lv_color_hex(0x888888), 0);
        lv_obj_align(status_lbl_, LV_ALIGN_BOTTOM_LEFT, 4, -2);

        RefreshList();
    }

    void RefreshList()
    {
        if (!list_)
            return;

        lv_obj_clean(list_);

        if (files_.empty())
        {
            lv_obj_t* lbl = lv_list_add_text(list_, "No .bin files found on SD card");
            lv_obj_set_style_text_color(lbl, lv_color_hex(0x888888), 0);
            lv_label_set_text(status_lbl_, "Insert SD card with .bin files");
            return;
        }

        for (int i = 0; i < static_cast<int>(files_.size()); ++i)
        {
            const char* slash = strrchr(files_[i].c_str(), '/');
            const char* name = slash ? slash + 1 : files_[i].c_str();

            lv_obj_t* btn = lv_list_add_button(list_, LV_SYMBOL_FILE, name);
            lv_obj_set_style_text_font(lv_obj_get_child(btn, 1), &lv_font_montserrat_14, 0);

            auto* ctx = new SdBrowserCtx{this, i};
            lv_obj_add_event_cb(btn, OnItemClicked, LV_EVENT_CLICKED, ctx);
            lv_obj_add_event_cb(
                btn,
                [](lv_event_t* e) { delete static_cast<SdBrowserCtx*>(lv_event_get_user_data(e)); },
                LV_EVENT_DELETE, ctx);
        }

        char buf[48];
        snprintf(buf, sizeof(buf), "%zu file(s) found", files_.size());
        lv_label_set_text(status_lbl_, buf);
        selected_idx_ = 0;
    }

    void HandleInput(const hal::InputEvent& ev)
    {
        auto& disp = mgr_.Display();

        if (ev.nav == hal::NavKey::Back)
        {
            mgr_.Pop();
            return;
        }

        if (files_.empty())
            return;

        if (ev.nav == hal::NavKey::Next)
        {
            selected_idx_ = (selected_idx_ + 1) % static_cast<int>(files_.size());
            if (disp.Lock(100))
            {
                lv_obj_t* btn = lv_obj_get_child(list_, selected_idx_);
                if (btn)
                    lv_obj_scroll_to_view(btn, LV_ANIM_ON);
                disp.Unlock();
            }
        }
        else if (ev.nav == hal::NavKey::Prev)
        {
            selected_idx_ = (selected_idx_ - 1 + static_cast<int>(files_.size())) %
                            static_cast<int>(files_.size());
            if (disp.Lock(100))
            {
                lv_obj_t* btn = lv_obj_get_child(list_, selected_idx_);
                if (btn)
                    lv_obj_scroll_to_view(btn, LV_ANIM_ON);
                disp.Unlock();
            }
        }
        else if (ev.nav == hal::NavKey::Select)
        {
            StartInstall(selected_idx_);
        }
    }

    void StartInstall(int idx)
    {
        if (idx < 0 || idx >= static_cast<int>(files_.size()))
            return;

        const std::string& path = files_[idx];

        // 从文件名派生显示名称（去除路径和 .bin 扩展名）
        const char* slash = strrchr(path.c_str(), '/');
        std::string name = slash ? slash + 1 : path;
        if (name.size() > 4)
        {
            std::string ext = name.substr(name.size() - 4);
            if (ext == ".bin" || ext == ".BIN")
                name = name.substr(0, name.size() - 4);
        }

        PushInstallProgress(mgr_, registry_, installer_, path, name);
    }

   public:
    ScreenSdBrowser(ScreenMgrT& mgr, RegistryT& registry, InstallerT& installer)
        : mgr_(mgr), registry_(registry), installer_(installer)
    {
        auto& disp = mgr_.Display();

        if (!disp.Lock(1000))
        {
            ESP_LOGE(kTag, "Lock timeout");
            return;
        }

        screen_ = lv_obj_create(nullptr);
        lv_obj_set_style_bg_color(screen_, lv_color_hex(0x1A1A2E), 0);
        lv_obj_set_style_bg_opa(screen_, LV_OPA_COVER, 0);

        BuildWidgets();
        disp.Unlock();

        mgr_.Input().SetCallback([this](const hal::InputEvent& ev) { HandleInput(ev); });
    }

    lv_obj_t* Screen() const { return screen_; }

    void SetFiles(std::vector<std::string> files)
    {
        files_ = std::move(files);
        RefreshList();
    }
};

/// 由 ScreenAppList 调用，将 SD 浏览屏幕压入导航栈。
template <typename ScreenMgrT, typename RegistryT, typename InstallerT>
void PushSdBrowser(ScreenMgrT& mgr, RegistryT& registry, InstallerT& installer)
{
    auto* scr = new ScreenSdBrowser<ScreenMgrT, RegistryT, InstallerT>(mgr, registry, installer);
    scr->SetFiles(registry.GetStorage().SdListFiles(CONFIG_LAUNCHER_SD_MOUNT_POINT, ".bin"));
    mgr.Push(scr->Screen(), [scr]() { delete scr; });
}

}  // namespace launcher::ui
