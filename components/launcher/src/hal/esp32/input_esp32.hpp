#pragma once

#include "hal/i_input.hpp"
#include "wrapper/logger.hpp"
#include "device/lilygo_t_lora_pager_keyboard.hpp"

namespace launcher::hal
{

/**
 * @brief IInput implementation for the LilyGo T-LoRa Pager keyboard.
 *
 * Wraps wrapper::LilyGoLoRaPagerKeyboard (TCA8418 4×10 matrix) and
 * maps key events to launcher::hal::NavKey:
 *
 *   Key   | NavKey  | Rationale
 *   ------+---------+----------------------------------
 *   ENT   | SELECT  | Confirm selection
 *   BS    | BACK    | Cancel / go back
 *   i / I | PREV    | Scroll up (vim-style)
 *   k / K | NEXT    | Scroll down (vim-style)
 *   Other | NONE    | Pass-through as char (text input)
 *
 * Only key-down (pressed) events are dispatched; key-up events are
 * silently discarded.
 */
class InputEsp32 : public IInput
{
    wrapper::Logger& logger_;
    wrapper::LilyGoLoRaPagerKeyboard kb_;
    InputCallback user_cb_;

    /// Translate a decoded keyboard event to NavKey.
    static NavKey mapKey(const wrapper::LilyGoLoRaPagerKeyEvent& ev);

   public:
    /// @param logger  Logger reference.
    /// @param tca     Already-initialised TCA8418 reference.
    InputEsp32(wrapper::Logger& logger, wrapper::Tca8418& tca);

    void setCallback(InputCallback cb) override;
    void poll() override;
};

}  // namespace launcher::hal
