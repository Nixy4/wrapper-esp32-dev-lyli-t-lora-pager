#include "screen_manager.hpp"

#include <esp_log.h>

static const char* TAG = "Launcher|ScreenMgr";

namespace launcher::ui
{

ScreenManager::ScreenManager(hal::IDisplay& display, hal::IInput& input)
    : display_(display), input_(input)
{
}

ScreenManager::~ScreenManager()
{
    // 从栈顶到栈底销毁所有屏幕
    while (!stack_.empty())
    {
        DestroyFrame(stack_.back());
        stack_.pop_back();
    }
}

void ScreenManager::Push(lv_obj_t* screen, DestroyCallback on_destroy)
{
    stack_.push_back({screen, std::move(on_destroy)});
    LoadTop();
}

void ScreenManager::Pop()
{
    if (stack_.size() <= 1)
    {
        ESP_LOGW(TAG, "Cannot pop last screen");
        return;
    }

    DestroyFrame(stack_.back());
    stack_.pop_back();
    LoadTop();
}

void ScreenManager::Replace(lv_obj_t* screen, DestroyCallback on_destroy)
{
    if (!stack_.empty())
    {
        DestroyFrame(stack_.back());
        stack_.pop_back();
    }
    stack_.push_back({screen, std::move(on_destroy)});
    LoadTop();
}

void ScreenManager::LoadTop()
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
    }
    else
    {
        ESP_LOGE(TAG, "Display lock timeout while loading screen");
    }
}

void ScreenManager::DestroyFrame(Frame& frame)
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

}  // namespace launcher::ui
