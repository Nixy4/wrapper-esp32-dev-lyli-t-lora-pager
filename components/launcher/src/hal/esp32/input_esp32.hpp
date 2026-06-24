#pragma once

#include "hal/input_base.hpp"
#include "wrapper/logger.hpp"
#include "device/lilygo_t_lora_pager_keyboard.hpp"

namespace launcher::hal
{

/**
 * @brief 针对 LilyGo T-LoRa Pager 键盘的 IInput 实现。
 *
 * 封装 wrapper::LilyGoLoRaPagerKeyboard（TCA8418 4×10 矩阵），
 * 并将按键事件映射到 launcher::hal::NavKey：
 *
 *   按键  | NavKey  | 说明
 *   -----+---------+----------------------------------
 *   ENT  | SELECT  | 确认选择
 *   BS   | BACK    | 取消/返回
 *   i/I  | PREV    | 向上滚动（vim 风格）
 *   k/K  | NEXT    | 向下滚动（vim 风格）
 *   其他  | NONE    | 直接传递字符（文本输入）
 *
 * 只分发按键按下（pressed）事件；按键松开事件会被静默丢弃。
 */
class InputEsp32 : public InputBase<InputEsp32>
{
    wrapper::Logger& logger_;
    wrapper::LilyGoLoRaPagerKeyboard kb_;
    InputCallback user_cb_;

    /// 将键盘解码事件转换为 NavKey。
    static NavKey MapKey(const wrapper::LilyGoLoRaPagerKeyEvent& ev);

   public:
    /// @param logger  日志引用。
    /// @param tca     已初始化的 TCA8418 引用。
    InputEsp32(wrapper::Logger& logger, wrapper::Tca8418& tca);

    void SetCallback(InputCallback cb);
    void Poll();
};

}  // namespace launcher::hal
