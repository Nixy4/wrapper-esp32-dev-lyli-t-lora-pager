#include "screen_sd_browser.hpp"
#include "screen_install_progress.hpp"

#include <cstring>
#include <esp_log.h>

static const char* TAG = "Launcher|SdBrowser";

namespace launcher::ui
{

// ── 上下文结构体
// ───────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────

struct SdBrowserCtx
{
    ScreenSdBrowser* self;
    int idx;
};

// ─── 构造 ───────────────────────────────────────────────────────────────────

ScreenSdBrowser::ScreenSdBrowser(ScreenManager& mgr,
                                 core::AppRegistry& registry,
                                 core::SdInstaller& installer)
    : mgr_(mgr), registry_(registry), installer_(installer)
{
    hal::IDisplay& disp = mgr_.Display();

    if (!disp.Lock(1000))
    {
        ESP_LOGE(TAG, "Lock timeout");
        return;
    }

    screen_ = lv_obj_create(nullptr);
    lv_obj_set_style_bg_color(screen_, lv_color_hex(0x1A1A2E), 0);
    lv_obj_set_style_bg_opa(screen_, LV_OPA_COVER, 0);

    BuildWidgets();
    disp.Unlock();

    mgr_.Input().SetCallback([this](const hal::InputEvent& ev) { HandleInput(ev); });
}

// ─── 控件 ────────────────────────────────────────────────────────────────────

void ScreenSdBrowser::BuildWidgets()
{
    hal::IDisplay& disp = mgr_.Display();
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
    lv_label_set_text(title, "SD Card — select .bin to install");
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

void ScreenSdBrowser::RefreshList()
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
        // 只显示文件名，不显示完整路径
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

// ─── 输入处理 ────────────────────────────────────────────────────────────────

void ScreenSdBrowser::HandleInput(const hal::InputEvent& ev)
{
    hal::IDisplay& disp = mgr_.Display();

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
        selected_idx_ =
            (selected_idx_ - 1 + static_cast<int>(files_.size())) % static_cast<int>(files_.size());
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

void ScreenSdBrowser::StartInstall(int idx)
{
    if (idx < 0 || idx >= static_cast<int>(files_.size()))
        return;

    const std::string& path = files_[idx];

    // 从文件名派生显示名称（去除路径和 .bin 扩展名）
    const char* slash = strrchr(path.c_str(), '/');
    std::string name = slash ? slash + 1 : path;
    // 删除 .bin 后缀（大小写不敏感）
    if (name.size() > 4)
    {
        std::string ext = name.substr(name.size() - 4);
        if (ext == ".bin" || ext == ".BIN")
            name = name.substr(0, name.size() - 4);
    }

    // 展示进度屏幕
    extern void PushInstallProgress(ScreenManager&, core::AppRegistry&, core::SdInstaller&,
                                    const std::string&, const std::string&);
    PushInstallProgress(mgr_, registry_, installer_, path, name);
}

// ─── LVGL 回调 ───────────────────────────────────────────────────────────────

void ScreenSdBrowser::OnItemClicked(lv_event_t* e)
{
    auto* ctx = static_cast<SdBrowserCtx*>(lv_event_get_user_data(e));
    if (ctx && lv_event_get_code(e) == LV_EVENT_CLICKED)
        ctx->self->StartInstall(ctx->idx);
}

void ScreenSdBrowser::OnBackClicked(lv_event_t* e)
{
    if (lv_event_get_code(e) != LV_EVENT_CLICKED)
        return;
    auto* self = static_cast<ScreenSdBrowser*>(lv_event_get_user_data(e));
    if (self)
        self->mgr_.Pop();
}

// ─── 前向声明辅助函数 ────────────────────────────────────────────────────────

// 屈名在 screen_sd_browser.hpp 中；由 ScreenAppList 调用。
// launcher.cpp 存储 g_sd_installer 使本函数可以访问它。
extern core::SdInstaller* g_sd_installer;

void PushSdBrowser(ScreenManager& mgr, core::AppRegistry& registry)
{
    if (!g_sd_installer)
    {
        ESP_LOGE(TAG, "g_sd_installer is null — SD browsing unavailable");
        return;
    }

    auto* scr = new ScreenSdBrowser(mgr, registry, *g_sd_installer);
    // 现在填充文件列表（存储已由 launcher.cpp 挂载）
    extern hal::IStorage* g_storage;
    if (g_storage)
        scr->SetFiles(g_storage->SdListFiles(CONFIG_LAUNCHER_SD_MOUNT_POINT, ".bin"));
    mgr.Push(scr->Screen(), [scr]() { delete scr; });
}

}  // namespace launcher::ui
