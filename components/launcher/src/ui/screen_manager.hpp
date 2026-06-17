#pragma once

#include <functional>
#include <vector>

#include "hal/i_display.hpp"
#include "hal/i_input.hpp"

namespace launcher::ui
{

/**
 * @brief Stack-based screen navigation manager.
 *
 * Each "screen" is an lv_obj_t* created with lv_obj_create(NULL).
 * ScreenManager owns the lifecycle of screens on its stack:
 *   - push() creates and loads a new screen on top.
 *   - pop()  destroys the top screen and reloads the previous one.
 *   - replace() is a combined pop + push in one step.
 *
 * All LVGL calls must be made while holding the display lock.
 * ScreenManager acquires/releases the lock internally around
 * loadScreen() and lv_obj_del() operations.
 *
 * Screens receive input events via InputCallback registered on IInput;
 * screens update that registration each time they become active.
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

    /// Push a pre-created LVGL screen onto the stack and make it active.
    /// @param on_destroy  Optional callback invoked just before the screen
    ///                    object is deleted (e.g. to free user data).
    void push(lv_obj_t* screen, DestroyCallback on_destroy = nullptr);

    /// Remove the top screen and return to the previous one.
    /// No-op if only one screen remains.
    void pop();

    /// Replace the top screen (pop + push without an intermediate redraw).
    void replace(lv_obj_t* screen, DestroyCallback on_destroy = nullptr);

    /// Number of screens on the stack.
    size_t depth() const { return stack_.size(); }

    hal::IDisplay& display() { return display_; }
    hal::IInput& input() { return input_; }
};

}  // namespace launcher::ui
