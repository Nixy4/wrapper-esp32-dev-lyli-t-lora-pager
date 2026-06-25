#pragma once

#include <cstdint>
#include <type_traits>
#include <utility>

namespace launcher::osal
{

/**
 * @brief CRTP + SFINAE 静态基类：平台无关的互斥锁接口约束。
 *
 * 编译期验证 Derived 须实现：
 *   bool Lock(uint32_t timeout_ms)
 *   void Unlock()
 */
template<typename Derived>
class MutexBase
{
    template<typename T, typename = void>
    struct HasLock : std::false_type {};
    template<typename T>
    struct HasLock<T, std::void_t<std::enable_if_t<std::is_convertible_v<
        decltype(std::declval<T&>().Lock(std::declval<uint32_t>())),
        bool>>>> : std::true_type {};

    template<typename T, typename = void>
    struct HasUnlock : std::false_type {};
    template<typename T>
    struct HasUnlock<T, std::void_t<std::enable_if_t<std::is_void_v<
        decltype(std::declval<T&>().Unlock())>>>> : std::true_type {};

   protected:
    ~MutexBase() noexcept
    {
        static_assert(HasLock<Derived>::value,
            "MutexBase<D>: D 须实现 bool Lock(uint32_t timeout_ms)");
        static_assert(HasUnlock<Derived>::value,
            "MutexBase<D>: D 须实现 void Unlock()");
    }
};

}  // namespace launcher::osal
