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
    // Destroy all screens from top to bottom
    while (!stack_.empty())
    {
        destroyFrame(stack_.back());
        stack_.pop_back();
    }
}

void ScreenManager::push(lv_obj_t* screen, DestroyCallback on_destroy)
{
    stack_.push_back({screen, std::move(on_destroy)});
    loadTop();
}

void ScreenManager::pop()
{
    if (stack_.size() <= 1)
    {
        ESP_LOGW(TAG, "Cannot pop last screen");
        return;
    }

    destroyFrame(stack_.back());
    stack_.pop_back();
    loadTop();
}

void ScreenManager::replace(lv_obj_t* screen, DestroyCallback on_destroy)
{
    if (!stack_.empty())
    {
        destroyFrame(stack_.back());
        stack_.pop_back();
    }
    stack_.push_back({screen, std::move(on_destroy)});
    loadTop();
}

void ScreenManager::loadTop()
{
    if (stack_.empty())
        return;

    lv_obj_t* scr = stack_.back().screen;
    if (!scr)
        return;

    if (display_.lock(500))
    {
        display_.loadScreen(scr);
        display_.unlock();
    }
    else
    {
        ESP_LOGE(TAG, "Display lock timeout while loading screen");
    }
}

void ScreenManager::destroyFrame(Frame& frame)
{
    if (frame.on_destroy)
        frame.on_destroy();

    if (frame.screen)
    {
        if (display_.lock(200))
        {
            lv_obj_del(frame.screen);
            display_.unlock();
        }
        frame.screen = nullptr;
    }
}

}  // namespace launcher::ui
