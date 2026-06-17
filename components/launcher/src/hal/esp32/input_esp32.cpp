#include "input_esp32.hpp"

#include <cctype>
#include <esp_log.h>

static const char* TAG = "Launcher|Input";

namespace launcher::hal
{

InputEsp32::InputEsp32(wrapper::Logger& logger, wrapper::Tca8418& tca)
    : logger_(logger), kb_(logger, tca)
{
}

void InputEsp32::setCallback(InputCallback cb)
{
    user_cb_ = std::move(cb);

    kb_.SetKeyCallback(
        [this](const wrapper::LilyGoLoRaPagerKeyEvent& ev)
        {
            // Only dispatch key-down events
            if (!ev.pressed || !user_cb_)
                return;

            InputEvent ie;
            ie.nav = mapKey(ev);
            // Carry the character only for non-navigation keys
            ie.ch = (ie.nav == NavKey::NONE) ? ev.ch : '\0';
            ie.long_press = false;

            user_cb_(ie);
        });
}

void InputEsp32::poll() { kb_.Poll(); }

// static
NavKey InputEsp32::mapKey(const wrapper::LilyGoLoRaPagerKeyEvent& ev)
{
    if (ev.ch == '\n')
        return NavKey::SELECT;
    if (ev.ch == '\b')
        return NavKey::BACK;

    const char lc = static_cast<char>(tolower(static_cast<unsigned char>(ev.ch)));

    if (lc == 'i')
        return NavKey::PREV;
    if (lc == 'k')
        return NavKey::NEXT;

    return NavKey::NONE;
}

}  // namespace launcher::hal
