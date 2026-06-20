#pragma once

#include <cstdint>

namespace launcher::osal
{

/**
 * @brief 平台无关的互斥锁抽象接口。
 *
 * FreeRTOS 实现： freertos/mutex_impl.hpp
 */
class IMutex
{
   public:
    virtual ~IMutex() = default;

    /// 获取互斥锁。@p timeout_ms == UINT32_MAX 表示永久阻塞。
    /// @return 成功返回 true，超时返回 false。
    virtual bool Lock(uint32_t timeout_ms = UINT32_MAX) = 0;

    /// 释放互斥锁。
    virtual void Unlock() = 0;
};

}  // namespace launcher::osal
