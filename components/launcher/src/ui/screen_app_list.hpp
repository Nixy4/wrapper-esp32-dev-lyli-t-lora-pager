#pragma once

#include <cstdio>
#include <cctype>
#include <string>
#include <vector>
#include <esp_log.h>

#include "lvgl.h"
#include "hal/input_base.hpp"
#include "ui/screen_manager.hpp"
#include "ui/screen_sd_browser.hpp"
#include "ui/screen_settings.hpp"
#include "core/app_registry.hpp"
#include "core/boot_manager.hpp"

namespace launcher::ui
{

/**
 * @brief Launcher 主屏幕：显示已安装的应用列表。（CRTP 零开销模板版本）
 *
 * 布局（480 × 222 横屏）：
 *   ┌──────────────────────────────────────────────────────┐
 *   │  Launcher              [SD Install]  [Settings]  ... │  标题栏
 *   ├──────────────────────────────────────────────────────┤
 *   │  ▶ My App 1                                          │
 *   │    My App 2                                          │  可滚动列表
 *   └──────────────────────────────────────────────────────┘
 *   状态栏："3 apps installed"
 *
 * 导航操作：
 *   NEXT / PREV  → 移动列表选中项
 *   SELECT       → 确认引导
 *   BACK (长按) → 无操作（已在根屏幕）
 */
template <typename ScreenMgrT, typename RegistryT, typename BootT, typename InstallerT>
class ScreenAppList
{
    ScreenMgrT& mgr_;
    RegistryT& registry_;
    BootT& boot_mgr_;
    InstallerT& installer_;

    lv_obj_t* screen_ = nullptr;
    lv_obj_t* list_ = nullptr;
    lv_obj_t* status_lbl_ = nullptr;

    std::vector<core::AppInfo> apps_;
    std::vector<lv_obj_t*> app_rows_;
    int selected_idx_ = 0;

    static constexpr const char* kTag = "Launcher|AppList";

    void BuildWidgets()
    {
        auto& disp = mgr_.Display();
        const int W = disp.Width();

        // ── 标题栏
        lv_obj_t* title_bar = lv_obj_create(screen_);
        lv_obj_set_size(title_bar, W, 30);
        lv_obj_align(title_bar, LV_ALIGN_TOP_MID, 0, 0);
        lv_obj_set_style_bg_color(title_bar, lv_color_hex(0x16213E), 0);
        lv_obj_set_style_border_width(title_bar, 0, 0);
        lv_obj_set_style_radius(title_bar, 0, 0);
        lv_obj_set_style_pad_all(title_bar, 4, 0);

        lv_obj_t* title = lv_label_create(title_bar);
        lv_label_set_text(title, "Launcher");
        lv_obj_set_style_text_font(title, &lv_font_montserrat_14, 0);
        lv_obj_set_style_text_color(title, lv_color_hex(0xE2E2E2), 0);
        lv_obj_align(title, LV_ALIGN_LEFT_MID, 4, 0);

        lv_obj_t* hints = lv_label_create(title_bar);
        lv_label_set_text(hints, "d: SD Install   s: Settings");
        lv_obj_set_style_text_font(hints, &lv_font_montserrat_14, 0);
        lv_obj_set_style_text_color(hints, lv_color_hex(0x9FB7D6), 0);
        lv_obj_align(hints, LV_ALIGN_RIGHT_MID, -4, 0);

        // ── Scrollable app list
        list_ = lv_obj_create(screen_);
        lv_obj_set_size(list_, W, disp.Height() - 50);
        lv_obj_align(list_, LV_ALIGN_TOP_MID, 0, 32);
        lv_obj_set_style_bg_color(list_, lv_color_hex(0x1A1A2E), 0);
        lv_obj_set_style_border_width(list_, 0, 0);
        lv_obj_set_style_radius(list_, 0, 0);
        lv_obj_set_style_pad_all(list_, 4, 0);
        lv_obj_set_scroll_dir(list_, LV_DIR_VER);

        // ── 状态栏
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

        registry_.Load(apps_);
        lv_obj_clean(list_);
        app_rows_.clear();
        selected_idx_ = 0;

        if (apps_.empty())
        {
            lv_obj_t* lbl = lv_label_create(list_);
            lv_label_set_text(lbl, "No apps installed");
            lv_obj_set_style_text_font(lbl, &lv_font_montserrat_14, 0);
            lv_obj_set_style_text_color(lbl, lv_color_hex(0x888888), 0);
            lv_obj_align(lbl, LV_ALIGN_TOP_LEFT, 4, 4);
        }
        else
        {
            for (int i = 0; i < static_cast<int>(apps_.size()); ++i)
            {
                lv_obj_t* row = lv_label_create(list_);
                lv_obj_set_size(row, lv_obj_get_width(list_) - 12, 24);
                lv_obj_align(row, LV_ALIGN_TOP_LEFT, 4, i * 26);
                lv_label_set_long_mode(row, LV_LABEL_LONG_DOT);
                lv_obj_set_style_text_font(row, &lv_font_montserrat_14, 0);
                lv_obj_set_style_pad_left(row, 6, 0);
                lv_obj_set_style_pad_right(row, 6, 0);
                lv_obj_set_style_pad_top(row, 4, 0);
                lv_obj_set_style_radius(row, 4, 0);
                app_rows_.push_back(row);
            }
        }

        UpdateStatus();
        UpdateSelection();
    }

    void UpdateStatus()
    {
        if (!status_lbl_)
            return;
        char buf[64];
        snprintf(buf, sizeof(buf), "%zu app(s)  [i=up  k=down  ENT=boot  d=sd  s=settings]",
                 apps_.size());
        lv_label_set_text(status_lbl_, buf);
    }

    void UpdateSelection()
    {
        for (int i = 0; i < static_cast<int>(app_rows_.size()); ++i)
        {
            lv_obj_t* row = app_rows_[i];
            const bool selected = (i == selected_idx_);
            std::string text = selected ? "> " : "  ";
            text += apps_[i].name;
            lv_label_set_text(row, text.c_str());
            lv_obj_set_style_text_color(row, selected ? lv_color_hex(0xFFFFFF)
                                                      : lv_color_hex(0xC9D1E3),
                                        0);
            lv_obj_set_style_bg_color(row, selected ? lv_color_hex(0x0F3460)
                                                    : lv_color_hex(0x1A1A2E),
                                      0);
            lv_obj_set_style_bg_opa(row, selected ? LV_OPA_COVER : LV_OPA_TRANSP, 0);
        }
    }

    void MoveSelection(int delta)
    {
        if (apps_.empty() || app_rows_.empty())
            return;

        auto& disp = mgr_.Display();
        selected_idx_ = (selected_idx_ + delta + static_cast<int>(apps_.size())) %
                        static_cast<int>(apps_.size());

        if (disp.Lock(100))
        {
            UpdateSelection();
            lv_obj_t* row = app_rows_[selected_idx_];
            if (row)
                lv_obj_scroll_to_view(row, LV_ANIM_ON);
            disp.Unlock();
        }
    }

    void HandleInput(const hal::InputEvent& ev)
    {
        if (ev.nav == hal::NavKey::None && ev.ch != '\0')
        {
            const char lc = static_cast<char>(tolower(static_cast<unsigned char>(ev.ch)));
            if (lc == 'd')
                PushSdBrowser(mgr_, registry_, installer_);
            else if (lc == 's')
                PushSettings(mgr_, registry_);
            return;
        }

        if (apps_.empty())
            return;

        if (ev.nav == hal::NavKey::Next)
        {
            MoveSelection(1);
        }
        else if (ev.nav == hal::NavKey::Prev)
        {
            MoveSelection(-1);
        }
        else if (ev.nav == hal::NavKey::Select)
        {
            ConfirmBoot(selected_idx_);
        }
    }

    void ConfirmBoot(int idx)
    {
        if (idx < 0 || idx >= static_cast<int>(apps_.size()))
            return;
        ESP_LOGI(kTag, "User selected app: '%s' ('%s')", apps_[idx].name.c_str(),
                 apps_[idx].label.c_str());
        boot_mgr_.BootApp(apps_[idx].label);
    }

   public:
    ScreenAppList(ScreenMgrT& mgr, RegistryT& registry, BootT& boot_mgr, InstallerT& installer)
        : mgr_(mgr), registry_(registry), boot_mgr_(boot_mgr), installer_(installer)
    {
        auto& disp = mgr_.Display();

        if (!disp.Lock(1000))
        {
            ESP_LOGE(kTag, "Lock timeout — cannot build AppList screen");
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

    void Activate()
    {
        mgr_.Input().SetCallback([this](const hal::InputEvent& ev) { HandleInput(ev); });
    }
};

}  // namespace launcher::ui
