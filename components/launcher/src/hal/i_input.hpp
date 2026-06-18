#pragma once

#include <functional>

namespace launcher::hal
{

/**
 * @brief 导航按键标识符。
 *
 * 从设备的按鈕/编码器/键盘按键映射而来。
 * 具体映射关系位于 HAL ESP32 实现（input_esp32.hpp），
 * 因此移植到其他平台时只需修改该文件。
 */
enum class NavKey
{
    NONE,    ///< 非导航按键 —— 通过 char 字段传递字符
    NEXT,    ///< 向下/向前移动选择
    PREV,    ///< 向上/向后移动选择
    SELECT,  ///< 确认/进入
    BACK,    ///< 取消/返回
};

/**
 * @brief 单个解码后的输入事件。
 */
struct InputEvent
{
    NavKey nav = NavKey::NONE;  ///< 导航操作（NONE 表示文本输入）
    char ch = '\0';             ///< nav == NONE 时的可打印字符
    bool long_press = false;    ///< true 表示按键被长按
};

using InputCallback = std::function<void(const InputEvent&)>;

/**
 * @brief 平台无关的输入抽象接口。
 *
 * 调用方注册回调后，周期性调用 poll()（通常在专用 FreeRTOS
 * 任务中每 10ms 调用一次）。
 *
 * ESP32 实现： esp32/input_esp32.hpp
 */
class IInput
{
   public:
    virtual ~IInput() = default;

    /// 注册事件回调（传入 nullptr 可取消注册）。
    virtual void setCallback(InputCallback cb) = 0;

    /// 读取待处理输入并将事件分发给已注册的回调。
    virtual void poll() = 0;
};

}  // namespace launcher::hal
