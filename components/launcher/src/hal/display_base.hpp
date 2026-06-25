#pragma once

#include <cstdint>
#include <type_traits>
#include <utility>
#include "lvgl.h"

namespace launcher::hal
{

/**
 * @brief CRTP + SFINAE 静态基类：平台无关的显示接口约束。
 *
 * 编译期验证 Derived 须实现：
 *   bool Lock(uint32_t timeout_ms)
 *   void Unlock()
 *   lv_obj_t* ActiveScreen()
 *   void LoadScreen(lv_obj_t* screen)
 *   void SetRotation(lv_display_rotation_t rot)
 *   int Width()
 *   int Height()
 */
template <typename Derived>
class DisplayBase
{
    template <typename T, typename = void>
    struct HasLock : std::false_type
    {
    };
    template <typename T>
    struct HasLock<T,
                   std::void_t<std::enable_if_t<std::is_convertible_v<
                       decltype(std::declval<T&>().Lock(std::declval<uint32_t>())),
                       bool>>>> : std::true_type
    {
    };

    template <typename T, typename = void>
    struct HasUnlock : std::false_type
    {
    };
    template <typename T>
    struct HasUnlock<
        T,
        std::void_t<std::enable_if_t<std::is_void_v<decltype(std::declval<T&>().Unlock())>>>>
        : std::true_type
    {
    };

    template <typename T, typename = void>
    struct HasActiveScreen : std::false_type
    {
    };
    template <typename T>
    struct HasActiveScreen<T,
                           std::void_t<std::enable_if_t<
                               std::is_pointer_v<decltype(std::declval<T&>().ActiveScreen())>>>>
        : std::true_type
    {
    };

    template <typename T, typename = void>
    struct HasLoadScreen : std::false_type
    {
    };
    template <typename T>
    struct HasLoadScreen<T,
                         std::void_t<std::enable_if_t<std::is_void_v<
                             decltype(std::declval<T&>().LoadScreen(std::declval<lv_obj_t*>()))>>>>
        : std::true_type
    {
    };

    template <typename T, typename = void>
    struct HasSetRotation : std::false_type
    {
    };
    template <typename T>
    struct HasSetRotation<
        T,
        std::void_t<std::enable_if_t<std::is_void_v<decltype(std::declval<T&>().SetRotation(
            std::declval<lv_display_rotation_t>()))>>>> : std::true_type
    {
    };

    template <typename T, typename = void>
    struct HasWidth : std::false_type
    {
    };
    template <typename T>
    struct HasWidth<T,
                    std::void_t<std::enable_if_t<
                        std::is_convertible_v<decltype(std::declval<T&>().Width()), int>>>>
        : std::true_type
    {
    };

    template <typename T, typename = void>
    struct HasHeight : std::false_type
    {
    };
    template <typename T>
    struct HasHeight<T,
                     std::void_t<std::enable_if_t<
                         std::is_convertible_v<decltype(std::declval<T&>().Height()), int>>>>
        : std::true_type
    {
    };

   protected:
    ~DisplayBase() noexcept
    {
        static_assert(HasLock<Derived>::value,
                      "DisplayBase<D>: D 须实现 bool Lock(uint32_t timeout_ms)");
        static_assert(HasUnlock<Derived>::value, "DisplayBase<D>: D 须实现 void Unlock()");
        static_assert(HasActiveScreen<Derived>::value,
                      "DisplayBase<D>: D 须实现 lv_obj_t* ActiveScreen()");
        static_assert(HasLoadScreen<Derived>::value,
                      "DisplayBase<D>: D 须实现 void LoadScreen(lv_obj_t* screen)");
        static_assert(HasSetRotation<Derived>::value,
                      "DisplayBase<D>: D 须实现 void SetRotation(lv_display_rotation_t rot)");
        static_assert(HasWidth<Derived>::value, "DisplayBase<D>: D 须实现 int Width()");
        static_assert(HasHeight<Derived>::value, "DisplayBase<D>: D 须实现 int Height()");
    }
};

}  // namespace launcher::hal
