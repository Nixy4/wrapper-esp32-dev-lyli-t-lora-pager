#pragma once

#include "osal/i_mutex.hpp"

#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

namespace launcher::osal
{

/**
 * @brief IMutex 的 FreeRTOS 实现。
 */
class FreeRtosMutex : public IMutex
{
    SemaphoreHandle_t handle_;

   public:
    FreeRtosMutex() : handle_(xSemaphoreCreateMutex()) {}

    ~FreeRtosMutex()
    {
        if (handle_)
            vSemaphoreDelete(handle_);
    }

    bool Lock(uint32_t timeout_ms = UINT32_MAX) override
    {
        TickType_t ticks = (timeout_ms == UINT32_MAX) ? portMAX_DELAY : pdMS_TO_TICKS(timeout_ms);
        return xSemaphoreTake(handle_, ticks) == pdTRUE;
    }

    void Unlock() override { xSemaphoreGive(handle_); }
};

}  // namespace launcher::osal
