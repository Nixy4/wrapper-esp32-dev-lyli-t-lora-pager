#pragma once

#include <cstdint>

namespace launcher
{

/// 单次导航脉冲事件（单次触发，读后清零）。
/// 对应 Reference 中的 volatile bool 全局变量
/// (NextPress / PrevPress / UpPress / DownPress / SelPress / EscPress)。
struct NavEvent
{
    bool next = false;  ///< 编码器顺时针 / 键盘 'd'
    bool prev = false;  ///< 编码器逆时针 / 键盘 'a'
    bool up = false;    ///< 键盘 'w'
    bool down = false;  ///< 键盘 's'
    bool sel = false;   ///< 编码器按键 / Enter
    bool esc = false;   ///< 返回键 (GPIO 0) / Backspace

    bool HasNavigation() const { return next || prev || up || down || sel || esc; }
    void Clear() { next = prev = up = down = sel = esc = false; }
};

/// 已解码的键盘字符事件（来自 TCA8418 矩阵）。
/// 对应 Reference 中的 keyStroke 结构体。
struct KeyEvent
{
    bool pressed = false;
    char ch = '\0';         ///< 可打印字符；特殊键为 '\0'
    bool is_enter = false;  ///< Enter / KEY_ENTER
    bool is_del = false;    ///< Backspace / KEY_BACKSPACE
    bool is_fn = false;     ///< FN/ALT 修饰键激活
    bool cap_mode = false;  ///< 本次事件后的 CAPS 锁状态
    bool sym_mode = false;  ///< 本次事件后的 SYM 模式状态

    void Clear() { *this = KeyEvent{}; }
};

/// 系统级硬件抽象接口，供 launcher 业务层使用。
/// 覆盖：输入事件（编码器 + TCA8418 键盘 + 物理按键）、
/// 显示亮度、电池状态和电源管理。
/// 对应 Reference 中的 interface.h + powerSave.h API。
class SystemBase
{
   public:
    virtual ~SystemBase() = default;

    // ---- 时间 ----

    virtual uint32_t GetMillis() const = 0;
    virtual void DelayMs(uint32_t ms) const = 0;

    // ---- 显示亮度 ----

    /// @param percent  0–100；0 = 关闭背光
    virtual void SetBrightness(uint8_t percent) = 0;
    virtual void SetBacklight(bool on) = 0;
    virtual uint8_t GetBrightness() const = 0;

    // ---- 电池 ----

    /// @return 0–100；-1 = 不可用（BQ27220 读取错误）
    virtual int GetBatteryPercent() const = 0;
    virtual bool IsCharging() const = 0;

    // ---- 电源管理 ----

    /// 通过 BQ25896 PPM.shutdown() 关机
    virtual void PowerOff() = 0;
    virtual void Reboot() = 0;

    // ---- 输入轮询 ----
    // 需在专用 FreeRTOS 任务中周期性调用（约每 10 ms 一次）。
    // 对应 Reference 中的 taskInputHandler + InputHandler()。

    virtual void PollInput() = 0;

    // ---- 导航事件（编码器 + 物理按键）----

    /// 返回并清除所有待处理导航标志。
    virtual NavEvent GetAndClearNavigation() = 0;

    /// 自上次 Get/Clear 调用后是否有任意输入。
    virtual bool HasAnyInput() const = 0;

    /// 清除所有待处理的导航事件和键盘事件。
    virtual void ClearAllInput() = 0;

    // ---- 键盘事件（TCA8418，每次调用返回一个）----

    /// 从内部 FIFO 弹出下一条已解码键盘事件。
    /// @return false = FIFO 为空。
    virtual bool GetAndClearKeyEvent(KeyEvent& out) = 0;

    // ---- 省电管理 ----

    /// @param seconds  空闲多少秒后调暗；0 = 禁用
    virtual void SetDimTimeoutSec(uint32_t seconds) = 0;

    /// 通知用户活动 — 重置省电倒计时。
    virtual void NotifyActivity() = 0;
};

}  // namespace launcher
