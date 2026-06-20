#pragma once

#include <cstdint>

// LVGL 是 ESP32 HAL 实现所使用的显示框架。
// 针对其他 GUI 框架的移植实现应将 lv_obj_t* 替换为自定义的控件/表面句柄。
#include "lvgl.h"

namespace launcher::hal
{

/**
 * @brief 平台无关的显示抽象接口。
 *
 * 为 Launcher UI 提供所需的最小接口集：
 *   - 线程安全的加锁/解锁（委托给底层框架的互斥锁）
 *   - 访问当前激活的屏幕根对象
 *   - 屏幕加载与旋转设置
 *   - 显示尺寸获取
 *
 * ESP32 实现： esp32/display_esp32.hpp
 */
class IDisplay
{
   public:
    virtual ~IDisplay() = default;

    /// 获取显示互斥锁。在创建/修改控件时必须持有此锁。
    /// @return 成功返回 true，超时返回 false。
    virtual bool Lock(uint32_t timeout_ms) = 0;

    /// 释放显示互斥锁。
    virtual void Unlock() = 0;

    /// @return 当前激活的 LVGL 屏幕（lv_scr_act()）。
    virtual lv_obj_t* ActiveScreen() = 0;

    /// 将 @p screen 加载为激活显示屏幕。
    virtual void LoadScreen(lv_obj_t* screen) = 0;

    /// 设置显示方向。
    virtual void SetRotation(lv_display_rotation_t rot) = 0;

    /// 显示宽度（像素）。
    virtual int Width() = 0;

    /// 显示高度（像素）。
    virtual int Height() = 0;
};

}  // namespace launcher::hal
