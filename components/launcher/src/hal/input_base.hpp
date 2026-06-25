#pragma once

#include <functional>
#include <type_traits>
#include <utility>

namespace launcher::hal
{

/**
 * @brief 导航按键标识符。
 *
 * 从设备的按钮/编码器/键盘按键映射而来。
 * 具体映射关系位于 HAL ESP32 实现（input_esp32.hpp），
 * 因此移植到其他平台时只需修改该文件。
 */
enum class NavKey
{
    None,    ///< 非导航按键 —— 通过 char 字段传递字符
    Next,    ///< 向下/向前移动选择
    Prev,    ///< 向上/向后移动选择
    Select,  ///< 确认/进入
    Back,    ///< 取消/返回
};

/**
 * @brief 单个解码后的输入事件。
 */
struct InputEvent
{
    NavKey nav = NavKey::None;  ///< 导航操作（NONE 表示文本输入）
    char ch = '\0';             ///< nav == NONE 时的可打印字符
    bool long_press = false;    ///< true 表示按键被长按
};

using InputCallback = std::function<void(const InputEvent&)>;

/**
 * @brief CRTP + SFINAE 静态基类：平台无关的输入接口约束。
 *
 * 编译期验证 Derived 须实现：
 *   void SetCallback(InputCallback cb)
 *   void Poll()
 */
template <typename Derived>
class InputBase
{
    template <typename T, typename = void>
    struct HasSetCallback : std::false_type
    {
    };
    template <typename T>
    struct HasSetCallback<
        T,
        std::void_t<std::enable_if_t<std::is_void_v<decltype(std::declval<T&>().SetCallback(
            std::declval<InputCallback>()))>>>> : std::true_type
    {
    };

    template <typename T, typename = void>
    struct HasPoll : std::false_type
    {
    };
    template <typename T>
    struct HasPoll<
        T,
        std::void_t<std::enable_if_t<std::is_void_v<decltype(std::declval<T&>().Poll())>>>>
        : std::true_type
    {
    };

   protected:
    ~InputBase() noexcept
    {
        static_assert(HasSetCallback<Derived>::value,
                      "InputBase<D>: D 须实现 void SetCallback(InputCallback cb)");
        static_assert(HasPoll<Derived>::value, "InputBase<D>: D 须实现 void Poll()");
    }
};

}  // namespace launcher::hal
