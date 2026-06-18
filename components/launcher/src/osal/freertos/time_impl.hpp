#pragma once

#include "osal/i_time.hpp"
#include "wrapper/freertos.hpp"

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

namespace launcher::osal
{

/**
 * @brief ITime 的 FreeRTOS 实现。
 *        委托给 wrapper::Delay() 和 xTaskGetTickCount()。
 */
class FreeRtosTime : public ITime
{
   public:
    void delayMs(uint32_t ms) override { wrapper::Delay(ms); }

    uint32_t tickMs() override { return static_cast<uint32_t>(pdTICKS_TO_MS(xTaskGetTickCount())); }
};

}  // namespace launcher::osal
