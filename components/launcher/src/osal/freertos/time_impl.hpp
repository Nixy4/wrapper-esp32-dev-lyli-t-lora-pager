#pragma once

#include "osal/time_base.hpp"
#include "wrapper/freertos.hpp"

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

namespace launcher::osal
{

/**
 * @brief ITime 的 FreeRTOS 实现。
 *        委托给 wrapper::Delay() 和 xTaskGetTickCount()。
 */
class FreeRtosTime : public TimeBase<FreeRtosTime>
{
   public:
    void DelayMs(uint32_t ms) { wrapper::Delay(ms); }

    uint32_t TickMs() { return static_cast<uint32_t>(pdTICKS_TO_MS(xTaskGetTickCount())); }
};

}  // namespace launcher::osal
