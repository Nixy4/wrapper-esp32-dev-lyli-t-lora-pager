#pragma once

#include <string>
#include <cstring>
#include <cstdio>
#include <esp_log.h>

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include "lvgl.h"
#include "hal/input_base.hpp"
#include "ui/screen_manager.hpp"
#include "core/app_registry.hpp"
#include "core/sd_installer.hpp"

namespace launcher::ui
{

/**
 * @brief 显示固件安装进度的屏幕。（CRTP 零开销模板版本）
 *
 * 后台 FreeRTOS 任务执行实际刷写操作，
 * 而 LVGL 任务通过定时器轮询更新进度条和日志标签。
 *
 * 完成后屏幕自动返回两级（到应用列表）并触发注册表刷新。
 */
template <typename ScreenMgrT, typename RegistryT, typename InstallerT>
class ScreenInstallProgress
{
    ScreenMgrT& mgr_;
    RegistryT& registry_;
    InstallerT& installer_;
    std::string sd_path_;
    std::string display_name_;

    lv_obj_t* screen_ = nullptr;
    lv_obj_t* bar_ = nullptr;
    lv_obj_t* stage_lbl_ = nullptr;
    lv_obj_t* pct_lbl_ = nullptr;

    TaskHandle_t task_handle_ = nullptr;
    bool finished_ = false;
    bool success_ = false;

    struct ProgressData
    {
        char stage[64] = {};
        size_t written = 0;
        size_t total = 0;
    };
    ProgressData prog_data_{};

    static constexpr const char* kTag = "Launcher|InstallProg";

    static void InstallTask(void* arg)
    {
        auto* self = static_cast<ScreenInstallProgress*>(arg);
        self->success_ = self->installer_.Install(
            self->sd_path_.c_str(), self->display_name_.c_str(),
            [self](const char* stage, size_t written, size_t total)
            {
                snprintf(self->prog_data_.stage, sizeof(self->prog_data_.stage), "%s", stage);
                self->prog_data_.written = written;
                self->prog_data_.total = total;
            });
        self->finished_ = true;
        vTaskDelete(nullptr);
    }

    static void OnTimerTick(lv_timer_t* timer)
    {
        auto* self = static_cast<ScreenInstallProgress*>(lv_timer_get_user_data(timer));
        if (!self)
            return;

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
                lv_label_set_text(self->stage_lbl_, "Installation complete!");
                lv_bar_set_value(self->bar_, 100, LV_ANIM_OFF);
                lv_label_set_text(self->pct_lbl_, "100%");

                lv_timer_t* back_timer = lv_timer_create(
                    [](lv_timer_t* t)
                    {
                        auto* s = static_cast<ScreenInstallProgress*>(lv_timer_get_user_data(t));
                        lv_timer_delete(t);
                        s->mgr_.Pop();
                        s->mgr_.Pop();
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

    void BuildWidgets()
    {
        auto& disp = mgr_.Display();
        const int W = disp.Width();

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
        lv_label_set_text(stage_lbl_, "Starting\xe2\x80\xa6");
        lv_obj_set_style_text_font(stage_lbl_, &lv_font_montserrat_14, 0);
        lv_obj_set_style_text_color(stage_lbl_, lv_color_hex(0xAAAAAA), 0);
        lv_obj_align(stage_lbl_, LV_ALIGN_CENTER, 0, 35);
        lv_label_set_long_mode(stage_lbl_, LV_LABEL_LONG_SCROLL_CIRCULAR);
        lv_obj_set_width(stage_lbl_, W - 40);
    }

   public:
    ScreenInstallProgress(ScreenMgrT& mgr,
                          RegistryT& registry,
                          InstallerT& installer,
                          const std::string& sd_path,
                          const std::string& display_name)
        : mgr_(mgr),
          registry_(registry),
          installer_(installer),
          sd_path_(sd_path),
          display_name_(display_name)
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

        mgr_.Input().SetCallback(nullptr);  // 安装期间禁用输入

        xTaskCreate(InstallTask, "lnch_install", 8192, this, 3, &task_handle_);
        lv_timer_create(OnTimerTick, 300, this);
    }

    lv_obj_t* Screen() const { return screen_; }
};

/// 辅助函数：创建安装进度屏幕并压入导航栈。
template <typename ScreenMgrT, typename RegistryT, typename InstallerT>
void PushInstallProgress(ScreenMgrT& mgr,
                         RegistryT& registry,
                         InstallerT& installer,
                         const std::string& sd_path,
                         const std::string& display_name)
{
    auto* scr = new ScreenInstallProgress<ScreenMgrT, RegistryT, InstallerT>(
        mgr, registry, installer, sd_path, display_name);
    mgr.Push(scr->Screen(), [scr]() { delete scr; });
}

}  // namespace launcher::ui
