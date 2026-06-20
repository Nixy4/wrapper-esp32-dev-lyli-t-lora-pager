#include "display_esp32.hpp"
#include <esp_log.h>

static const char* TAG = "Launcher|Display";

namespace launcher::hal
{

DisplayEsp32::DisplayEsp32(wrapper::Logger& logger, wrapper::LvglPort& lvgl, int width, int height)
    : logger_(logger), lvgl_(lvgl), width_(width), height_(height)
{
}

bool DisplayEsp32::Lock(uint32_t timeout_ms) { return lvgl_.Lock(timeout_ms); }

void DisplayEsp32::Unlock() { lvgl_.Unlock(); }

lv_obj_t* DisplayEsp32::ActiveScreen() { return lv_scr_act(); }

void DisplayEsp32::LoadScreen(lv_obj_t* screen) { lv_scr_load(screen); }

void DisplayEsp32::SetRotation(lv_display_rotation_t rot) { lvgl_.SetRotation(rot); }

}  // namespace launcher::hal
