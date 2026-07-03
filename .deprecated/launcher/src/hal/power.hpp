#pragma once
#include <cstdint>
namespace launcher::hal
{
class PowerBase
{
    enum class State
    {
        Awake,
        Sleeping,
    };

    State core_{State::Awake};
    State bus_{State::Awake};
    State display_{State::Awake};
    State wifi_{State::Awake};

   private:
    bool availability_;

   public:
    bool IsAvailable() { return availability_; }

    PowerBase() = default;
    virtual ~PowerBase() = default;

    virtual bool CheckAvailability() = 0;

    virtual void CoreSleep(int delay) = 0;
    virtual void CoreWakeup() = 0;

    virtual void BusSleep(int delay) = 0;
    virtual void BusWakeup() = 0;

    virtual void DisplaySleep(int delay) = 0;
    virtual void DisplayWakeup() = 0;

    virtual void WifiSleep() = 0;
    virtual void WifiWakeup() = 0;
};
}  // namespace launcher::hal