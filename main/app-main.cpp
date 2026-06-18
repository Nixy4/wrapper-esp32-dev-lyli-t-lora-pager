#include "sdkconfig.h"
#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include "launcher.hpp"

static const char* TAG = "App";

//! Application entry point
//! ----------------------------------------------------------------------------------------
extern "C" void app_main(void)
{
    ESP_LOGI(TAG, "Starting Launcher…");

    launcher::Config cfg;
    cfg.boot_to_last_app = true;
    cfg.brightness = 70;
    cfg.input_task_stack = 4096;
    cfg.input_task_prio = 4;

    // startTask() returns immediately; the Launcher runs in its own task.
    launcher::startTask(cfg, 24576, 5);
}
