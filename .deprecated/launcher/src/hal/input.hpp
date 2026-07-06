#pragma once
#include <cstdint>
#include <vector>

namespace launcher::hal
{

enum class InputType : uint8_t
{
    None = 0,
    Navigation,
    Keyboard,
    Touch,
};

enum class NavigationCode : uint8_t
{
    None = 0,
    Up,
    Down,
    Left,
    Right,
    Enter,
    Back,
    Menu,
    Home,
    VolumeUp,
    VolumeDown,
    Power,
};

union InputEvent
{
    struct raw
    {
        uint8_t type;
        uint8_t data[7];
    } raw;

    struct
    {
        uint8_t type;
        NavigationCode Code;
        uint8_t pressed;
    } navigation;

    struct
    {
        uint8_t type;
        uint8_t keycode;
        uint8_t pressed;
    } keyboard;

    struct
    {
        uint8_t type;
        uint16_t x;
        uint16_t y;
        uint8_t pressure;
    } touch;
};

class InputBase
{
   private:
    bool availability_;

   public:
    bool IsAvailable() { return availability_; }

    InputBase() = default;
    virtual ~InputBase() = default;

    virtual bool CheckAvailability() = 0;
    virtual InputEvent Poll() = 0;
};

}  // namespace launcher::hal
