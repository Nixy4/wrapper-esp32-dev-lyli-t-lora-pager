#pragma once

#include <functional>
#include <vector>

#include <esp_log.h>
#include "lvgl.h"

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
template <typename DisplayT, typename InputT>
class ScreenManager
{
   public:
    using display_type = DisplayT;
    using input_type = InputT;
    using DestroyCallback = std::function<void()>;
    using ActivateCallback = std::function<void()>;

   private:
    DisplayT& display_;
    InputT& input_;

    static constexpr const char* kTag = "Launcher|ScreenMgr";

    struct Frame
    {
        lv_obj_t* screen = nullptr;
        DestroyCallback on_destroy;
        ActivateCallback on_activate;
    };

    std::vector<Frame> stack_;

    void LoadTop()
    {
        if (stack_.empty())
            return;

        lv_obj_t* scr = stack_.back().screen;
        if (!scr)
            return;

        if (display_.Lock(500))
        {
            display_.LoadScreen(scr);
            display_.Unlock();

            if (stack_.back().on_activate)
                stack_.back().on_activate();
        }
        else
        {
            ESP_LOGE(kTag, "Display lock timeout while loading screen");
        }
    }

    void DestroyFrame(Frame& frame)
    {
        if (frame.on_destroy)
            frame.on_destroy();

        if (frame.screen)
        {
            if (display_.Lock(200))
            {
                lv_obj_del(frame.screen);
                display_.Unlock();
            }
            frame.screen = nullptr;
        }
    }

   public:
    ScreenManager(DisplayT& display, InputT& input) : display_(display), input_(input) {}

    ~ScreenManager()
    {
        while (!stack_.empty())
        {
            DestroyFrame(stack_.back());
            stack_.pop_back();
        }
    }

    void Push(lv_obj_t* screen,
              DestroyCallback on_destroy = nullptr,
              ActivateCallback on_activate = nullptr)
    {
        stack_.push_back({screen, std::move(on_destroy), std::move(on_activate)});
        LoadTop();
    }

    void Pop()
    {
        if (stack_.size() <= 1)
        {
            ESP_LOGW(kTag, "Cannot pop last screen");
            return;
        }

        DestroyFrame(stack_.back());
        stack_.pop_back();
        LoadTop();
    }

    void Replace(lv_obj_t* screen,
                 DestroyCallback on_destroy = nullptr,
                 ActivateCallback on_activate = nullptr)
    {
        if (!stack_.empty())
        {
            DestroyFrame(stack_.back());
            stack_.pop_back();
        }
        stack_.push_back({screen, std::move(on_destroy), std::move(on_activate)});
        LoadTop();
    }

    size_t Depth() const { return stack_.size(); }

    DisplayT& Display() { return display_; }
    InputT& Input() { return input_; }
};

}  // namespace launcher::ui
