#pragma once

#include <functional>

namespace launcher::hal
{

/**
 * @brief Navigation key identifiers.
 *
 * Mapped from physical device buttons/encoder/keyboard keys.
 * The concrete mapping lives in the HAL ESP32 implementation
 * (input_esp32.hpp) so that only this file needs changing when
 * porting to another platform.
 */
enum class NavKey
{
    NONE,    ///< Not a navigation key — carry the char field instead
    NEXT,    ///< Move selection downward / forward
    PREV,    ///< Move selection upward / backward
    SELECT,  ///< Confirm / enter
    BACK,    ///< Cancel / go back
};

/**
 * @brief A single decoded input event.
 */
struct InputEvent
{
    NavKey nav = NavKey::NONE;  ///< Navigation action (NONE → text input)
    char ch = '\0';             ///< Printable character when nav == NONE
    bool long_press = false;    ///< true if the key was held
};

using InputCallback = std::function<void(const InputEvent&)>;

/**
 * @brief Platform-agnostic input abstraction.
 *
 * Callers register a callback and then call poll() periodically
 * (typically from a dedicated FreeRTOS task every 10 ms).
 *
 * ESP32 implementation: esp32/input_esp32.hpp
 */
class IInput
{
   public:
    virtual ~IInput() = default;

    /// Register the event callback (pass nullptr to deregister).
    virtual void setCallback(InputCallback cb) = 0;

    /// Read pending input and dispatch events to the registered callback.
    virtual void poll() = 0;
};

}  // namespace launcher::hal
