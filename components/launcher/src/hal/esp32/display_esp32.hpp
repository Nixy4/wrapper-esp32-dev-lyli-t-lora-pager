#pragma once

#include "hal/i_display.hpp"
#include "wrapper/lvgl.hpp"
#include "wrapper/logger.hpp"

namespace launcher::hal
{

/**
 * @brief IDisplay implementation backed by wrapper::LvglPort.
 *
 * Forwards lock/unlock to LvglPort::Lock/Unlock and exposes the
 * current LVGL screen via lv_scr_act().
 */
class DisplayEsp32 : public IDisplay
{
    wrapper::Logger& logger_;
    wrapper::LvglPort& lvgl_;
    int width_;
    int height_;

   public:
    DisplayEsp32(wrapper::Logger& logger, wrapper::LvglPort& lvgl, int width, int height);

    bool lock(uint32_t timeout_ms) override;
    void unlock() override;
    lv_obj_t* activeScreen() override;
    void loadScreen(lv_obj_t* screen) override;
    void setRotation(lv_display_rotation_t rot) override;
    int width() override { return width_; }
    int height() override { return height_; }
};

}  // namespace launcher::hal
