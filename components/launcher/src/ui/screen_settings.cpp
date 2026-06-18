#include "screen_settings.hpp"

#include <string>
#include <esp_log.h>

static const char* TAG = "Launcher|Settings";

// 设置界面中亮度的外部函数声明（由 launcher.cpp 的 extern 解析）
extern void launcherSetBrightness(int pct);

namespace launcher::ui
{

// ─── 构造 ───────────────────────────────────────────────────────────────────

ScreenSettings::ScreenSettings(ScreenManager& mgr, core::AppRegistry& registry)
    : mgr_(mgr), registry_(registry)
{
    hal::IDisplay& disp = mgr_.display();

    if (!disp.lock(1000))
    {
        ESP_LOGE(TAG, "Lock timeout");
        return;
    }

    screen_ = lv_obj_create(nullptr);
    lv_obj_set_style_bg_color(screen_, lv_color_hex(0x1A1A2E), 0);
    lv_obj_set_style_bg_opa(screen_, LV_OPA_COVER, 0);

    buildWidgets();
    disp.unlock();

    mgr_.input().setCallback([this](const hal::InputEvent& ev) { handleInput(ev); });
}

// ─── 控件 ────────────────────────────────────────────────────────────────────

void ScreenSettings::buildWidgets()
{
    hal::IDisplay& disp = mgr_.display();
    const int W = disp.width();

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
    lv_obj_add_event_cb(back_btn, onBackClicked, LV_EVENT_CLICKED, this);

    lv_obj_t* title = lv_label_create(bar);
    lv_label_set_text(title, "Settings");
    lv_obj_set_style_text_font(title, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(title, lv_color_hex(0xE2E2E2), 0);
    lv_obj_align(title, LV_ALIGN_CENTER, 20, 0);

    // Content container
    lv_obj_t* cont = lv_obj_create(screen_);
    lv_obj_set_size(cont, W - 20, disp.height() - 50);
    lv_obj_align(cont, LV_ALIGN_TOP_MID, 0, 36);
    lv_obj_set_style_bg_color(cont, lv_color_hex(0x1A1A2E), 0);
    lv_obj_set_style_border_width(cont, 0, 0);
    lv_obj_set_flex_flow(cont, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(cont, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    // ── 自动引导开关
    // ─────────────────────────────────────────────────────────────────────────────────────────────────────────────────
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
    lv_obj_add_event_cb(sw_autoboot_, onAutoBootChanged, LV_EVENT_VALUE_CHANGED, this);

    // ── 背光滑动条
    // ──────────────────────────────────────────────────────────────────────────────────────────────────────────────────
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
    lv_obj_add_event_cb(slider_bl_, onBrightnessChanged, LV_EVENT_VALUE_CHANGED, this);

    loadSettings();
}

void ScreenSettings::loadSettings()
{
    extern hal::IStorage* g_storage;

    // 加载自动引导设置
    bool auto_boot = true;
    if (g_storage)
    {
        std::string val;
        if (g_storage->nvsGet(CONFIG_LAUNCHER_NVS_CFG_NS, "auto_boot", val))
            auto_boot = (val != "0");
    }
    lv_obj_set_state(sw_autoboot_, LV_STATE_CHECKED, auto_boot);

    // 加载亮度设置
    int brightness = 70;
    if (g_storage)
    {
        std::string bval;
        if (g_storage->nvsGet(CONFIG_LAUNCHER_NVS_CFG_NS, "brightness", bval))
            brightness = std::stoi(bval);
    }
    lv_slider_set_value(slider_bl_, brightness, LV_ANIM_OFF);
}

void ScreenSettings::saveAutoboot(bool on)
{
    extern hal::IStorage* g_storage;
    if (g_storage)
        g_storage->nvsSet(CONFIG_LAUNCHER_NVS_CFG_NS, "auto_boot", on ? "1" : "0");
    ESP_LOGI(TAG, "auto_boot = %s", on ? "on" : "off");
}

void ScreenSettings::saveBrightness(int pct)
{
    extern hal::IStorage* g_storage;
    if (g_storage)
        g_storage->nvsSet(CONFIG_LAUNCHER_NVS_CFG_NS, "brightness", std::to_string(pct));
    launcherSetBrightness(pct);
    ESP_LOGI(TAG, "brightness = %d%%", pct);
}

// ─── 输入处理 ────────────────────────────────────────────────────────────────

void ScreenSettings::handleInput(const hal::InputEvent& ev)
{
    if (ev.nav == hal::NavKey::BACK)
        mgr_.pop();
}

// ─── LVGL 回调 ───────────────────────────────────────────────────────────────

void ScreenSettings::onBackClicked(lv_event_t* e)
{
    if (lv_event_get_code(e) != LV_EVENT_CLICKED)
        return;
    auto* self = static_cast<ScreenSettings*>(lv_event_get_user_data(e));
    if (self)
        self->mgr_.pop();
}

void ScreenSettings::onAutoBootChanged(lv_event_t* e)
{
    auto* self = static_cast<ScreenSettings*>(lv_event_get_user_data(e));
    if (!self)
        return;
    bool on = lv_obj_has_state(self->sw_autoboot_, LV_STATE_CHECKED);
    self->saveAutoboot(on);
}

void ScreenSettings::onBrightnessChanged(lv_event_t* e)
{
    auto* self = static_cast<ScreenSettings*>(lv_event_get_user_data(e));
    if (!self)
        return;
    int pct = static_cast<int>(lv_slider_get_value(self->slider_bl_));
    self->saveBrightness(pct);
}

// ─── 辅助函数 ────────────────────────────────────────────────────────────────

void pushSettings(ScreenManager& mgr, core::AppRegistry& registry)
{
    auto* scr = new ScreenSettings(mgr, registry);
    mgr.push(scr->screen(), [scr]() { delete scr; });
}

}  // namespace launcher::ui
