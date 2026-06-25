#pragma once

#include <string>
#include <esp_log.h>

#include "lvgl.h"
#include "hal/input_base.hpp"
#include "ui/screen_manager.hpp"
#include "core/app_registry.hpp"

// 由 launcher.cpp 提供
extern void LauncherSetBrightness(int pct);

namespace launcher::ui
{

/**
 * @brief 设置屏幕。（CRTP 零开销模板版本）
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
template <typename ScreenMgrT, typename RegistryT>
class ScreenSettings
{
    ScreenMgrT& mgr_;
    RegistryT& registry_;

    lv_obj_t* screen_ = nullptr;
    lv_obj_t* sw_autoboot_ = nullptr;
    lv_obj_t* slider_bl_ = nullptr;

    static constexpr const char* kTag = "Launcher|Settings";

    static void OnBackClicked(lv_event_t* e)
    {
        if (lv_event_get_code(e) != LV_EVENT_CLICKED)
            return;
        auto* self = static_cast<ScreenSettings*>(lv_event_get_user_data(e));
        if (self)
            self->mgr_.Pop();
    }

    static void OnAutoBootChanged(lv_event_t* e)
    {
        auto* self = static_cast<ScreenSettings*>(lv_event_get_user_data(e));
        if (!self)
            return;
        bool on = lv_obj_has_state(self->sw_autoboot_, LV_STATE_CHECKED);
        self->SaveAutoboot(on);
    }

    static void OnBrightnessChanged(lv_event_t* e)
    {
        auto* self = static_cast<ScreenSettings*>(lv_event_get_user_data(e));
        if (!self)
            return;
        int pct = static_cast<int>(lv_slider_get_value(self->slider_bl_));
        self->SaveBrightness(pct);
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
        lv_obj_t* bl = lv_label_create(back_btn);
        lv_label_set_text(bl, LV_SYMBOL_LEFT " Back");
        lv_obj_set_style_text_font(bl, &lv_font_montserrat_14, 0);
        lv_obj_center(bl);
        lv_obj_add_event_cb(back_btn, OnBackClicked, LV_EVENT_CLICKED, this);

        lv_obj_t* title = lv_label_create(bar);
        lv_label_set_text(title, "Settings");
        lv_obj_set_style_text_font(title, &lv_font_montserrat_14, 0);
        lv_obj_set_style_text_color(title, lv_color_hex(0xE2E2E2), 0);
        lv_obj_align(title, LV_ALIGN_CENTER, 20, 0);

        // Content container
        lv_obj_t* cont = lv_obj_create(screen_);
        lv_obj_set_size(cont, W - 20, disp.Height() - 50);
        lv_obj_align(cont, LV_ALIGN_TOP_MID, 0, 36);
        lv_obj_set_style_bg_color(cont, lv_color_hex(0x1A1A2E), 0);
        lv_obj_set_style_border_width(cont, 0, 0);
        lv_obj_set_flex_flow(cont, LV_FLEX_FLOW_COLUMN);
        lv_obj_set_flex_align(cont, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER,
                              LV_FLEX_ALIGN_CENTER);

        // ── 自动引导开关
        lv_obj_t* row1 = lv_obj_create(cont);
        lv_obj_set_size(row1, LV_PCT(100), 40);
        lv_obj_set_style_bg_opa(row1, LV_OPA_TRANSP, 0);
        lv_obj_set_style_border_width(row1, 0, 0);
        lv_obj_set_flex_flow(row1, LV_FLEX_FLOW_ROW);
        lv_obj_set_flex_align(row1, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER,
                              LV_FLEX_ALIGN_CENTER);

        lv_obj_t* lbl1 = lv_label_create(row1);
        lv_label_set_text(lbl1, "Auto-boot last app");
        lv_obj_set_style_text_font(lbl1, &lv_font_montserrat_14, 0);

        sw_autoboot_ = lv_switch_create(row1);
        lv_obj_add_event_cb(sw_autoboot_, OnAutoBootChanged, LV_EVENT_VALUE_CHANGED, this);

        // ── 背光滑动条
        lv_obj_t* row2 = lv_obj_create(cont);
        lv_obj_set_size(row2, LV_PCT(100), 50);
        lv_obj_set_style_bg_opa(row2, LV_OPA_TRANSP, 0);
        lv_obj_set_style_border_width(row2, 0, 0);
        lv_obj_set_flex_flow(row2, LV_FLEX_FLOW_ROW);
        lv_obj_set_flex_align(row2, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER,
                              LV_FLEX_ALIGN_CENTER);

        lv_obj_t* lbl2 = lv_label_create(row2);
        lv_label_set_text(lbl2, "Backlight");
        lv_obj_set_style_text_font(lbl2, &lv_font_montserrat_14, 0);

        slider_bl_ = lv_slider_create(row2);
        lv_obj_set_width(slider_bl_, 200);
        lv_slider_set_range(slider_bl_, 10, 100);
        lv_obj_add_event_cb(slider_bl_, OnBrightnessChanged, LV_EVENT_VALUE_CHANGED, this);

        LoadSettings();
    }

    void LoadSettings()
    {
        bool auto_boot = true;
        {
            std::string val;
            if (registry_.GetStorage().NvsGet(CONFIG_LAUNCHER_NVS_CFG_NS, "auto_boot", val))
                auto_boot = (val != "0");
        }
        lv_obj_set_state(sw_autoboot_, LV_STATE_CHECKED, auto_boot);

        int brightness = 70;
        {
            std::string bval;
            if (registry_.GetStorage().NvsGet(CONFIG_LAUNCHER_NVS_CFG_NS, "brightness", bval))
                brightness = std::stoi(bval);
        }
        lv_slider_set_value(slider_bl_, brightness, LV_ANIM_OFF);
    }

    void SaveAutoboot(bool on)
    {
        registry_.GetStorage().NvsSet(CONFIG_LAUNCHER_NVS_CFG_NS, "auto_boot", on ? "1" : "0");
        ESP_LOGI(kTag, "auto_boot = %s", on ? "on" : "off");
    }

    void SaveBrightness(int pct)
    {
        registry_.GetStorage().NvsSet(CONFIG_LAUNCHER_NVS_CFG_NS, "brightness",
                                      std::to_string(pct));
        LauncherSetBrightness(pct);
        ESP_LOGI(kTag, "brightness = %d%%", pct);
    }

    void HandleInput(const hal::InputEvent& ev)
    {
        if (ev.nav == hal::NavKey::Back)
            mgr_.Pop();
    }

   public:
    ScreenSettings(ScreenMgrT& mgr, RegistryT& registry) : mgr_(mgr), registry_(registry)
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

    void Activate()
    {
        mgr_.Input().SetCallback([this](const hal::InputEvent& ev) { HandleInput(ev); });
    }
};

/// 由 ScreenAppList 调用，将设置屏幕压入导航栈。
template <typename ScreenMgrT, typename RegistryT>
void PushSettings(ScreenMgrT& mgr, RegistryT& registry)
{
    auto* scr = new ScreenSettings<ScreenMgrT, RegistryT>(mgr, registry);
    mgr.Push(scr->Screen(), [scr]() { delete scr; }, [scr]() { scr->Activate(); });
}

}  // namespace launcher::ui
