#include "sdkconfig.h"
#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

static const char* TAG = "App";

//! Application entry point
//! ----------------------------------------------------------------------------------------
extern "C" void app_main(void) { ESP_LOGI(TAG, "Starting Launcher…"); }
