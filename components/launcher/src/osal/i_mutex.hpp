#pragma once

#include <cstdint>

namespace launcher::osal
{

/**
 * @brief Platform-agnostic mutex abstraction.
 *
 * FreeRTOS implementation: freertos/mutex_impl.hpp
 */
class IMutex
{
   public:
    virtual ~IMutex() = default;

    /// Acquire the mutex.  @p timeout_ms == UINT32_MAX blocks forever.
    /// @return true on success, false on timeout.
    virtual bool lock(uint32_t timeout_ms = UINT32_MAX) = 0;

    /// Release the mutex.
    virtual void unlock() = 0;
};

}  // namespace launcher::osal
