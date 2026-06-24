#pragma once

#include <cstdint>

namespace launcher::osal
{

/**
 * @brief CRTP 静态基类：平台无关的互斥锁接口约束。
 *
 * Derived 须提供以下方法（无 virtual）：
 *   Lock / Unlock
 *
 * FreeRTOS 实现： freertos/mutex_impl.hpp
 */
template <typename Derived>
class MutexBase
{
   protected:
    ~MutexBase() = default;
};

}  // namespace launcher::osal
