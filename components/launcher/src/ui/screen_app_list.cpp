#include "screen_app_list.hpp"
#include "screen_sd_browser.hpp"
#include "screen_settings.hpp"

#include <cstdio>
#include <esp_log.h>

static const char* TAG = "Launcher|AppList";

namespace launcher::ui
{

// ─── 通过 lv_event user-data 传递的上下文结构体
// ───────────────────────────────────────────────────────────

struct AppListCtx
{
    ScreenAppList* self;
    int idx;  // -1 表示 "SD Install" / "Settings" 按鈕
};

// ─── 构造 ───────────────────────────────────────────────────────────────────

ScreenAppList::ScreenAppList(ScreenManager& mgr,
                             core::AppRegistry& registry,
                             core::BootManager& boot_mgr)
    : mgr_(mgr), registry_(registry), boot_mgr_(boot_mgr)
{
    hal::IDisplay& disp = mgr_.Display();

    if (!disp.Lock(1000))
    {
        ESP_LOGE(TAG, "Lock timeout — cannot build AppList screen");
        return;
    }

    screen_ = lv_obj_create(nullptr);
    lv_obj_set_style_bg_color(screen_, lv_color_hex(0x1A1A2E), 0);
    lv_obj_set_style_bg_opa(screen_, LV_OPA_COVER, 0);

    BuildWidgets();

    disp.Unlock();

    // Register input handler
    mgr_.Input().SetCallback([this](const hal::InputEvent& ev) { HandleInput(ev); });
}

// ─── 控件构建
// ─────────────────────────────────────────────────────────────────────────────────────────────────────────────────────

void ScreenAppList::BuildWidgets()
{
    hal::IDisplay& disp = mgr_.Display();
    const int W = disp.Width();

    // ── 标题栏 ──────────────────────────────────────────────────────────────────
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

    // ── "SD Install" button ───────────────────────────────────────────────────
    lv_obj_t* btn_sd = lv_button_create(title_bar);
    lv_obj_set_size(btn_sd, 80, 22);
    lv_obj_align(btn_sd, LV_ALIGN_RIGHT_MID, -90, 0);
    lv_obj_set_style_bg_color(btn_sd, lv_color_hex(0x0F3460), 0);

    lv_obj_t* btn_sd_lbl = lv_label_create(btn_sd);
    lv_label_set_text(btn_sd_lbl, "SD Install");
    lv_obj_set_style_text_font(btn_sd_lbl, &lv_font_montserrat_14, 0);
    lv_obj_center(btn_sd_lbl);

    lv_obj_add_event_cb(btn_sd, OnSdInstallClicked, LV_EVENT_CLICKED, this);

    // ── "Settings" button ─────────────────────────────────────────────────────
    lv_obj_t* btn_set = lv_button_create(title_bar);
    lv_obj_set_size(btn_set, 70, 22);
    lv_obj_align(btn_set, LV_ALIGN_RIGHT_MID, -4, 0);
    lv_obj_set_style_bg_color(btn_set, lv_color_hex(0x0F3460), 0);

    lv_obj_t* btn_set_lbl = lv_label_create(btn_set);
    lv_label_set_text(btn_set_lbl, "Settings");
    lv_obj_set_style_text_font(btn_set_lbl, &lv_font_montserrat_14, 0);
    lv_obj_center(btn_set_lbl);

    lv_obj_add_event_cb(btn_set, OnSettingsClicked, LV_EVENT_CLICKED, this);

    // ── Scrollable app list ───────────────────────────────────────────────────
    list_ = lv_list_create(screen_);
    lv_obj_set_size(list_, W, disp.Height() - 50);
    lv_obj_align(list_, LV_ALIGN_TOP_MID, 0, 32);
    lv_obj_set_style_bg_color(list_, lv_color_hex(0x1A1A2E), 0);
    lv_obj_set_style_border_width(list_, 0, 0);
    lv_obj_set_style_radius(list_, 0, 0);

    // ── 状态栏 ──────────────────────────────────────────────────────────────────
    status_lbl_ = lv_label_create(screen_);
    lv_obj_set_style_text_font(status_lbl_, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(status_lbl_, lv_color_hex(0x888888), 0);
    lv_obj_align(status_lbl_, LV_ALIGN_BOTTOM_LEFT, 4, -2);

    RefreshList();
}

void ScreenAppList::RefreshList()
{
    if (!list_)
        return;

    registry_.Load(apps_);

    // Clear existing items
    lv_obj_clean(list_);

    if (apps_.empty())
    {
        lv_obj_t* lbl = lv_list_add_text(list_, "No apps installed");
        lv_obj_set_style_text_color(lbl, lv_color_hex(0x888888), 0);
    }
    else
    {
        for (int i = 0; i < static_cast<int>(apps_.size()); ++i)
        {
            lv_obj_t* btn = lv_list_add_button(list_, LV_SYMBOL_FILE, apps_[i].name.c_str());
            lv_obj_set_style_text_font(lv_obj_get_child(btn, 1), &lv_font_montserrat_14, 0);
            // 将索引存入 user-data
            auto* ctx = new AppListCtx{this, i};
            lv_obj_add_event_cb(btn, OnListItemClicked, LV_EVENT_CLICKED, ctx);
            // 删除时清理 ctx
            lv_obj_add_event_cb(
                btn,
                [](lv_event_t* e) { delete static_cast<AppListCtx*>(lv_event_get_user_data(e)); },
                LV_EVENT_DELETE, ctx);
        }
    }

    UpdateStatus();
    selected_idx_ = 0;
}

void ScreenAppList::UpdateStatus()
{
    if (!status_lbl_)
        return;

    char buf[48];
    snprintf(buf, sizeof(buf), "%zu app(s) installed  [i=up  k=down  ENT=boot]", apps_.size());
    lv_label_set_text(status_lbl_, buf);
}

// ─── 输入处理 ────────────────────────────────────────────────────────────────

void ScreenAppList::HandleInput(const hal::InputEvent& ev)
{
    if (apps_.empty())
        return;

    hal::IDisplay& disp = mgr_.Display();

    if (ev.nav == hal::NavKey::Next)
    {
        selected_idx_ = (selected_idx_ + 1) % static_cast<int>(apps_.size());
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
        selected_idx_ =
            (selected_idx_ - 1 + static_cast<int>(apps_.size())) % static_cast<int>(apps_.size());
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
        ConfirmBoot(selected_idx_);
    }
}

void ScreenAppList::ConfirmBoot(int idx)
{
    if (idx < 0 || idx >= static_cast<int>(apps_.size()))
        return;

    const std::string& label = apps_[idx].label;
    const std::string& name = apps_[idx].name;

    ESP_LOGI(TAG, "User selected app: '%s' ('%s')", name.c_str(), label.c_str());
    boot_mgr_.BootApp(label);  // calls esp_restart() — does not return
}

// ─── LVGL 事件回调 ───────────────────────────────────────────────────────────

void ScreenAppList::OnListItemClicked(lv_event_t* e)
{
    auto* ctx = static_cast<AppListCtx*>(lv_event_get_user_data(e));
    if (ctx && lv_event_get_code(e) == LV_EVENT_CLICKED)
        ctx->self->ConfirmBoot(ctx->idx);
}

void ScreenAppList::OnSdInstallClicked(lv_event_t* e)
{
    if (lv_event_get_code(e) != LV_EVENT_CLICKED)
        return;
    auto* self = static_cast<ScreenAppList*>(lv_event_get_user_data(e));
    if (!self)
        return;

    // 跳转到 ScreenSdBrowser 的导航通过前向声明实现。
    // 我们调用在 screen_sd_browser.cpp 中定义的自由函数，避免屏幕文件间的循环包含。
    extern void PushSdBrowser(ScreenManager&, core::AppRegistry&);
    PushSdBrowser(self->mgr_, self->registry_);
}

void ScreenAppList::OnSettingsClicked(lv_event_t* e)
{
    if (lv_event_get_code(e) != LV_EVENT_CLICKED)
        return;
    auto* self = static_cast<ScreenAppList*>(lv_event_get_user_data(e));
    if (!self)
        return;

    extern void PushSettings(ScreenManager&, core::AppRegistry&);
    PushSettings(self->mgr_, self->registry_);
}

}  // namespace launcher::ui
