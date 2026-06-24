#pragma once

#include <cstdint>

namespace launcher::osal
{

/**
 * @brief CRTP 静态基类：平台无关的时间接口约束。
 *
 * Derived 须提供以下方法（无 virtual）：
 *   DelayMs / TickMs
 *
 * FreeRTOS 实现： freertos/time_impl.hpp
 */
template <typename Derived>
class TimeBase
{
   protected:
    ~TimeBase() = default;
};

}  // namespace launcher::osal
