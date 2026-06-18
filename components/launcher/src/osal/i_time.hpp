#pragma once

#include <cstdint>

namespace launcher::osal
{

/**
 * @brief 平台无关的时间抽象接口。
 *
 * FreeRTOS 实现： freertos/time_impl.hpp
 */
class ITime
{
   public:
    virtual ~ITime() = default;

    /// 阻塞调用任务至少 @p ms 毫秒。
    virtual void delayMs(uint32_t ms) = 0;

    /// 返回自启动以来的毫秒数（超过 UINT32_MAX 后回绕）。
    virtual uint32_t tickMs() = 0;
};

}  // namespace launcher::osal
