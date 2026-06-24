#pragma once

#include "hal/display_base.hpp"
#include "wrapper/lvgl.hpp"
#include "wrapper/logger.hpp"

namespace launcher::hal
{

/**
 * @brief 基于 wrapper::LvglPort 的 IDisplay 实现。
 *
 * 将 lock/unlock 转发给 LvglPort::Lock/Unlock，
 * 并通过 lv_scr_act() 暴露当前 LVGL 屏幕。
 */
class DisplayEsp32 : public DisplayBase<DisplayEsp32>
{
    wrapper::Logger& logger_;
    wrapper::LvglPort& lvgl_;
    int width_;
    int height_;

   public:
    DisplayEsp32(wrapper::Logger& logger, wrapper::LvglPort& lvgl, int width, int height);

    bool Lock(uint32_t timeout_ms);
    void Unlock();
    lv_obj_t* ActiveScreen();
    void LoadScreen(lv_obj_t* screen);
    void SetRotation(lv_display_rotation_t rot);
    int Width() { return width_; }
    int Height() { return height_; }
};

}  // namespace launcher::hal
