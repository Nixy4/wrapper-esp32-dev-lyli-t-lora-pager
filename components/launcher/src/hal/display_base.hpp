#pragma once

#include <cstdint>
#include "lvgl.h"

namespace launcher::hal
{

/**
 * @brief CRTP 静态基类：平台无关的显示接口约束。
 *
 * Derived 须提供以下方法（无 virtual）：
 *   Lock / Unlock / ActiveScreen / LoadScreen / SetRotation / Width / Height
 *
 * ESP32 实现： esp32/display_esp32.hpp
 */
template <typename Derived>
class DisplayBase
{
   protected:
    ~DisplayBase() = default;
};

}  // namespace launcher::hal
