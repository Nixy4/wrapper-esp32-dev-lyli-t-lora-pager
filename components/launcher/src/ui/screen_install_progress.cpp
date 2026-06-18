#include "screen_install_progress.hpp"

#include <cstring>
#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

static const char* TAG = "Launcher|InstallProg";

namespace launcher::ui
{

// ─── 安装任务 ────────────────────────────────────────────────────────────────

void ScreenInstallProgress::installTask(void* arg)
{
    auto* self = static_cast<ScreenInstallProgress*>(arg);

    self->success_ = self->installer_.install(
        self->sd_path_.c_str(), self->display_name_.c_str(),
        [self](const char* stage, size_t written, size_t total)
        {
            // 更新共享的进度数据
            snprintf(self->prog_data_.stage, sizeof(self->prog_data_.stage), "%s", stage);
            self->prog_data_.written = written;
            self->prog_data_.total = total;
        });

    self->finished_ = true;
    vTaskDelete(nullptr);
}

// ─── LVGL 定时器回调 ─────────────────────────────────────────────────────────

void ScreenInstallProgress::onTimerTick(lv_timer_t* timer)
{
    auto* self = static_cast<ScreenInstallProgress*>(lv_timer_get_user_data(timer));
    if (!self)
        return;

    // 从共享进度数据更新 UI
    const size_t written = self->prog_data_.written;
    const size_t total = self->prog_data_.total;
    const char* stage = self->prog_data_.stage;

    if (total > 0)
    {
        int pct = static_cast<int>((written * 100ULL) / total);
        lv_bar_set_value(self->bar_, pct, LV_ANIM_OFF);

        char buf[24];
        snprintf(buf, sizeof(buf), "%d%%", pct);
        lv_label_set_text(self->pct_lbl_, buf);
    }

    lv_label_set_text(self->stage_lbl_, stage);

    if (self->finished_)
    {
        lv_timer_delete(timer);

        if (self->success_)
        {
            // 安装完成，稍后返回两级（到应用列表）
            lv_label_set_text(self->stage_lbl_, "Installation complete!");
            lv_bar_set_value(self->bar_, 100, LV_ANIM_OFF);
            lv_label_set_text(self->pct_lbl_, "100%");

            // 延迟后返回（弹出安装进度屏幕 + SD 浏览屏幕）
            lv_timer_t* back_timer = lv_timer_create(
                [](lv_timer_t* t)
                {
                    auto* s = static_cast<ScreenInstallProgress*>(lv_timer_get_user_data(t));
                    lv_timer_delete(t);
                    // 先后弹出：安装进度屏幕 + SD 浏览屏幕
                    s->mgr_.pop();
                    s->mgr_.pop();
                },
                1500, self);
            lv_timer_set_repeat_count(back_timer, 1);
        }
        else
        {
            lv_label_set_text(self->stage_lbl_, "Installation FAILED");
            lv_obj_set_style_text_color(self->stage_lbl_, lv_color_hex(0xFF4444), 0);
        }
    }
}

// ─── 构造 ───────────────────────────────────────────────────────────────────

ScreenInstallProgress::ScreenInstallProgress(ScreenManager& mgr,
                                             core::AppRegistry& registry,
                                             core::SdInstaller& installer,
                                             const std::string& sd_path,
                                             const std::string& display_name)
    : mgr_(mgr),
      registry_(registry),
      installer_(installer),
      sd_path_(sd_path),
      display_name_(display_name)
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

    // 安装期间禁用输入（BACK 键无效）
    mgr_.input().setCallback(nullptr);

    // 启动安装任务（优先级 3，8KB 栈）
    xTaskCreate(installTask, "lnch_install", 8192, this, 3, &task_handle_);

    // LVGL 定时器每 300ms 轮询进度
    lv_timer_create(onTimerTick, 300, this);
}

// ─── 控件 ────────────────────────────────────────────────────────────────────

void ScreenInstallProgress::buildWidgets()
{
    hal::IDisplay& disp = mgr_.display();
    const int W = disp.width();
    const int H = disp.height();

    lv_obj_t* title = lv_label_create(screen_);
    lv_label_set_text_fmt(title, "Installing: %s", display_name_.c_str());
    lv_obj_set_style_text_font(title, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(title, lv_color_hex(0xE2E2E2), 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 12);

    bar_ = lv_bar_create(screen_);
    lv_obj_set_size(bar_, W - 40, 20);
    lv_obj_align(bar_, LV_ALIGN_CENTER, 0, -20);
    lv_bar_set_range(bar_, 0, 100);
    lv_bar_set_value(bar_, 0, LV_ANIM_OFF);
    lv_obj_set_style_bg_color(lv_obj_get_child(bar_, 0), lv_color_hex(0x00B4D8), 0);

    pct_lbl_ = lv_label_create(screen_);
    lv_label_set_text(pct_lbl_, "0%");
    lv_obj_set_style_text_font(pct_lbl_, &lv_font_montserrat_14, 0);
    lv_obj_align(pct_lbl_, LV_ALIGN_CENTER, 0, 10);

    stage_lbl_ = lv_label_create(screen_);
    lv_label_set_text(stage_lbl_, "Starting…");
    lv_obj_set_style_text_font(stage_lbl_, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(stage_lbl_, lv_color_hex(0xAAAAAA), 0);
    lv_obj_align(stage_lbl_, LV_ALIGN_CENTER, 0, 35);
    lv_label_set_long_mode(stage_lbl_, LV_LABEL_LONG_SCROLL_CIRCULAR);
    lv_obj_set_width(stage_lbl_, W - 40);
}

// ─── 辅助函数 ────────────────────────────────────────────────────────────────

void pushInstallProgress(ScreenManager& mgr,
                         core::AppRegistry& registry,
                         core::SdInstaller& installer,
                         const std::string& sd_path,
                         const std::string& display_name)
{
    auto* scr = new ScreenInstallProgress(mgr, registry, installer, sd_path, display_name);
    mgr.push(scr->screen(), [scr]() { delete scr; });
}

}  // namespace launcher::ui
