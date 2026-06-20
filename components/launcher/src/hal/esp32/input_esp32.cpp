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

void InputEsp32::SetCallback(InputCallback cb)
{
    user_cb_ = std::move(cb);

    kb_.SetKeyCallback(
        [this](const wrapper::LilyGoLoRaPagerKeyEvent& ev)
        {
            // 只分发按键按下事件
            if (!ev.pressed || !user_cb_)
                return;

            InputEvent ie;
            ie.nav = MapKey(ev);
            // 非导航按键才传递字符
            ie.ch = (ie.nav == NavKey::None) ? ev.ch : '\0';
            ie.long_press = false;

            user_cb_(ie);
        });
}

void InputEsp32::Poll() { kb_.Poll(); }

// static
NavKey InputEsp32::MapKey(const wrapper::LilyGoLoRaPagerKeyEvent& ev)
{
    if (ev.ch == '\n')
        return NavKey::Select;
    if (ev.ch == '\b')
        return NavKey::Back;

    const char lc = static_cast<char>(tolower(static_cast<unsigned char>(ev.ch)));

    if (lc == 'i')
        return NavKey::Prev;
    if (lc == 'k')
        return NavKey::Next;

    return NavKey::None;
}

}  // namespace launcher::hal
