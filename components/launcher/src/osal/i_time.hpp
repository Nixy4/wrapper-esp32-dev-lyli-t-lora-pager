#pragma once

#include <cstdint>

namespace launcher::osal
{

/**
 * @brief Platform-agnostic time abstraction.
 *
 * FreeRTOS implementation: freertos/time_impl.hpp
 */
class ITime
{
   public:
    virtual ~ITime() = default;

    /// Block the calling task for at least @p ms milliseconds.
    virtual void delayMs(uint32_t ms) = 0;

    /// Return elapsed milliseconds since boot (wraps at UINT32_MAX).
    virtual uint32_t tickMs() = 0;
};

}  // namespace launcher::osal
