#pragma once

namespace launcher::hal
{
class DisplayBase
{
   private:
    bool availability_;
    int width_;
    int height_;

   public:
    bool IsAvailable() { return availability_; }
    int GetWidth() const { return width_; }
    int GetHeight() const { return height_; }

    DisplayBase(int width, int height) : width_(width), height_(height) {}
    virtual ~DisplayBase() = default;

    virtual bool CheckAvailability() = 0;
    virtual int GetBrightness() = 0;
    virtual bool SetBrightness(int percent) = 0;
    virtual bool GetBacklight() = 0;
    virtual bool SetBacklight(bool enable) = 0;
    virtual bool GetPowerSleep() = 0;
    virtual bool SetPowerSleep(bool enable) = 0;
};
}  // namespace launcher::hal