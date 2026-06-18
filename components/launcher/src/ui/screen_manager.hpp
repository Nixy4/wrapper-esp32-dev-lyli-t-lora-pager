#pragma once

#include <functional>
#include <vector>

#include "hal/i_display.hpp"
#include "hal/i_input.hpp"

namespace launcher::ui
{

/**
 * @brief 基于栈的屏幕导航管理器。
 *
 * 每个“屏幕”都是由 lv_obj_create(NULL) 创建的 lv_obj_t*。
 * ScreenManager 负责管理屏幕栈中屏幕的生命周期：
 *   - push()    创建并加载新屏幕到栈顶。
 *   - pop()     销毁栈顶屏幕并重新加载前一个。
 *   - replace() 一步完成 pop + push。
 *
 * 所有 LVGL 调用必须在持有显示锁的情况下进行。
 * ScreenManager 内部在 loadScreen() 和 lv_obj_del() 操作周围
 * 自动获取/释放锁。
 *
 * 屏幕通过在 IInput 上注册的 InputCallback 接收输入事件；
 * 每次屏幕变为激活状态时都会更新该注册。
 */
class ScreenManager
{
   public:
    using DestroyCallback = std::function<void()>;

   private:
    hal::IDisplay& display_;
    hal::IInput& input_;

    struct Frame
    {
        lv_obj_t* screen = nullptr;
        DestroyCallback on_destroy;
    };

    std::vector<Frame> stack_;

    void loadTop();
    void destroyFrame(Frame& frame);

   public:
    ScreenManager(hal::IDisplay& display, hal::IInput& input);
    ~ScreenManager();

    /// 将预先创建的 LVGL 屏幕压入栈并使其激活。
    /// @param on_destroy  屏幕对象删除前调用的回调（如需释放用户数据）。
    void push(lv_obj_t* screen, DestroyCallback on_destroy = nullptr);

    /// 删除栈顶屏幕并返回上一个屏幕。
    /// 只剩一个屏幕时不操作。
    void pop();

    /// 替换栈顶屏幕（pop + push 不产生中间重绘）。
    void replace(lv_obj_t* screen, DestroyCallback on_destroy = nullptr);

    /// 栈中屏幕数量。
    size_t depth() const { return stack_.size(); }

    hal::IDisplay& display() { return display_; }
    hal::IInput& input() { return input_; }
};

}  // namespace launcher::ui
