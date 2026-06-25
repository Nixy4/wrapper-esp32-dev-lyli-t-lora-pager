#pragma once

#include <cstdint>
#include <type_traits>
#include <utility>

namespace launcher::osal
{

/**
 * @brief CRTP + SFINAE 静态基类：平台无关的时间接口约束。
 *
 * 编译期验证 Derived 须实现：
 *   void DelayMs(uint32_t ms)
 *   uint32_t TickMs()
 */
template<typename Derived>
class TimeBase
{
    template<typename T, typename = void>
    struct HasDelayMs : std::false_type {};
    template<typename T>
    struct HasDelayMs<T, std::void_t<std::enable_if_t<std::is_void_v<
        decltype(std::declval<T&>().DelayMs(
            std::declval<uint32_t>()))>>>> : std::true_type {};

    template<typename T, typename = void>
    struct HasTickMs : std::false_type {};
    template<typename T>
    struct HasTickMs<T, std::void_t<std::enable_if_t<std::is_convertible_v<
        decltype(std::declval<T&>().TickMs()), uint32_t>>>> : std::true_type {};

   protected:
    ~TimeBase() noexcept
    {
        static_assert(HasDelayMs<Derived>::value,
            "TimeBase<D>: D 须实现 void DelayMs(uint32_t ms)");
        static_assert(HasTickMs<Derived>::value,
            "TimeBase<D>: D 须实现 uint32_t TickMs()");
    }
};

}  // namespace launcher::osal
