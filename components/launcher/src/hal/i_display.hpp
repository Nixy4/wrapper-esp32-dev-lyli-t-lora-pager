#pragma once

#include <cstdint>

// LVGL is the display framework used by the ESP32 HAL implementation.
// Portable implementations targeting other GUI frameworks should
// replace lv_obj_t* with their own widget/surface handle.
#include "lvgl.h"

namespace launcher::hal
{

/**
 * @brief Platform-agnostic display abstraction.
 *
 * Exposes the minimal surface needed by the Launcher UI:
 *   - Thread-safe lock/unlock (delegated to the underlying framework's mutex)
 *   - Access to the currently-active screen root object
 *   - Screen loading and display rotation
 *   - Display dimensions
 *
 * ESP32 implementation: esp32/display_esp32.hpp
 */
class IDisplay
{
   public:
    virtual ~IDisplay() = default;

    /// Acquire the display mutex.  Must be held while creating/modifying widgets.
    /// @return true on success, false on timeout.
    virtual bool lock(uint32_t timeout_ms) = 0;

    /// Release the display mutex.
    virtual void unlock() = 0;

    /// @return The currently-active LVGL screen (lv_scr_act()).
    virtual lv_obj_t* activeScreen() = 0;

    /// Load @p screen as the active display.
    virtual void loadScreen(lv_obj_t* screen) = 0;

    /// Set the display rotation.
    virtual void setRotation(lv_display_rotation_t rot) = 0;

    /// Display width in pixels.
    virtual int width() = 0;

    /// Display height in pixels.
    virtual int height() = 0;
};

}  // namespace launcher::hal
