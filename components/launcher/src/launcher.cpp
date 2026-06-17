#include "launcher.hpp"

#include <cstring>
#include <esp_log.h>
#include <esp_system.h>
#include <nvs_flash.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

// ── Board support (conditional on Kconfig) ────────────────────────────────────
#if defined(CONFIG_LAUNCHER_BOARD_LILYGO_T_LORA_PAGER)
#include "board/lilygo/t-lora-pager.hpp"
#include "driver/gpio.h"
// SD card CS pin on T-LoRa Pager (schematic: SDCARD_CS = GPIO21)
static constexpr gpio_num_t kSdCsPin = GPIO_NUM_21;
// Display physical resolution
static constexpr int kDisplayW = 480;
static constexpr int kDisplayH = 222;
#else
#error "No launcher board selected.  Set CONFIG_LAUNCHER_BOARD_* in Kconfig."
#endif

// ── OSAL / HAL / Core / UI includes ──────────────────────────────────────────
#include "osal/freertos/time_impl.hpp"
#include "hal/esp32/display_esp32.hpp"
#include "hal/esp32/input_esp32.hpp"
#include "hal/esp32/storage_esp32.hpp"
#include "hal/esp32/partition_esp32.hpp"
#include "core/app_registry.hpp"
#include "core/boot_manager.hpp"
#include "core/sd_installer.hpp"
#include "ui/screen_manager.hpp"
#include "ui/screen_app_list.hpp"

static const char* TAG = "Launcher";

// ── Module-level globals (file scope) ────────────────────────────────────────
// These are accessed by screen helpers (screen_sd_browser.cpp,
// screen_settings.cpp) via extern declarations.  They are valid for the
// lifetime of the launcher task.

namespace launcher::ui
{
hal::IStorage* g_storage = nullptr;
core::SdInstaller* g_sd_installer = nullptr;
}  // namespace launcher::ui

// ── Brightness helper (forward-declared by screen_settings.cpp) ──────────────

void launcherSetBrightness(int pct)
{
#if defined(CONFIG_LAUNCHER_BOARD_LILYGO_T_LORA_PAGER)
    wrapper::LilyGoLoraPager::GetInstance().SetDisplayBrightness(pct);
#endif
}

// ─────────────────────────────────────────────────────────────────────────────
//  Core implementation
// ─────────────────────────────────────────────────────────────────────────────

namespace launcher
{

static void runLauncher(const Config& cfg)
{
    // ── 1. Board hardware init ────────────────────────────────────────────────
    ESP_LOGI(TAG, "Initialising board hardware…");

#if defined(CONFIG_LAUNCHER_BOARD_LILYGO_T_LORA_PAGER)
    auto& board = wrapper::LilyGoLoraPager::GetInstance();

    if (!board.InitCoreBusAndIoExpander())
    {
        ESP_LOGE(TAG, "InitCoreBusAndIoExpander failed — halting");
        while (true) vTaskDelay(portMAX_DELAY);
    }
    if (!board.InitDisplay())
    {
        ESP_LOGE(TAG, "InitDisplay failed — halting");
        while (true) vTaskDelay(portMAX_DELAY);
    }
    if (!board.InitKeyboard())
        ESP_LOGW(TAG, "InitKeyboard failed — input disabled");
#endif

    // ── 2. NVS flash init ─────────────────────────────────────────────────────
    esp_err_t nvs_err = nvs_flash_init();
    if (nvs_err == ESP_ERR_NVS_NO_FREE_PAGES || nvs_err == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_LOGW(TAG, "NVS needs erase — reformatting");
        nvs_flash_erase();
        nvs_flash_init();
    }

    // ── 3. OSAL / HAL layer creation ──────────────────────────────────────────
#if defined(CONFIG_LAUNCHER_BOARD_LILYGO_T_LORA_PAGER)
    wrapper::Logger logger_disp("Launcher", "Display");
    wrapper::Logger logger_input("Launcher", "Input");
    wrapper::Logger logger_storage("Launcher", "Storage");
    wrapper::Logger logger_part("Launcher", "Partition");
#endif

    osal::FreeRtosTime time_impl;

#if defined(CONFIG_LAUNCHER_BOARD_LILYGO_T_LORA_PAGER)
    hal::DisplayEsp32 display_hal(logger_disp, board.GetLvglPort(), kDisplayW, kDisplayH);

    hal::InputEsp32 input_hal(logger_input, board.GetKeyboard());

    hal::StorageEsp32 storage_hal(logger_storage, board.GetSpiBus(), kSdCsPin,
                                  CONFIG_LAUNCHER_SD_MOUNT_POINT);

    hal::PartitionEsp32 partition_hal(logger_part);
#endif

    // Expose globals for screen helpers
    ui::g_storage = &storage_hal;

    // ── 4. Display rotation ───────────────────────────────────────────────────
    display_hal.setRotation(LV_DISPLAY_ROTATION_180);
    board.SetDisplayBrightness(cfg.brightness);

    // ── 5. Core layer creation ────────────────────────────────────────────────
    core::AppRegistry app_registry(storage_hal);
    core::BootManager boot_mgr(partition_hal, storage_hal, time_impl, app_registry);
    core::SdInstaller sd_installer(storage_hal, partition_hal, app_registry);
    ui::g_sd_installer = &sd_installer;

    // ── 6. SD card mount (best-effort — may not be inserted) ─────────────────
    if (!storage_hal.sdMount())
        ESP_LOGW(TAG, "SD card not present at startup");

    // ── 7. Boot decision ──────────────────────────────────────────────────────
    if (cfg.boot_to_last_app)
    {
        std::string last_label;
        if (!boot_mgr.shouldShowMenu(last_label))
        {
            ESP_LOGI(TAG, "Auto-booting '%s'", last_label.c_str());
            boot_mgr.bootApp(last_label);
            // If bootApp returns (setBootPartition failed), fall through to UI
            ESP_LOGW(TAG, "Auto-boot failed — showing Launcher menu");
        }
    }

    // ── 8. UI setup ───────────────────────────────────────────────────────────
    ui::ScreenManager screen_mgr(display_hal, input_hal);

    auto* app_list_screen = new ui::ScreenAppList(screen_mgr, app_registry, boot_mgr);
    screen_mgr.push(app_list_screen->screen(), [app_list_screen]() { delete app_list_screen; });

    // ── 9. Input polling task ─────────────────────────────────────────────────
    struct InputTaskArg
    {
        hal::IInput* input;
    };
    auto* input_arg = new InputTaskArg{&input_hal};

    xTaskCreate(
        [](void* arg)
        {
            auto* a = static_cast<InputTaskArg*>(arg);
            while (true)
            {
                a->input->poll();
                vTaskDelay(pdMS_TO_TICKS(10));
            }
        },
        "lnch_input", cfg.input_task_stack, input_arg, cfg.input_task_prio, nullptr);

    // ── 10. Main loop ─────────────────────────────────────────────────────────
    ESP_LOGI(TAG, "Launcher running");
    while (true)
    {
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

// ─────────────────────────────────────────────────────────────────────────────
//  Public API
// ─────────────────────────────────────────────────────────────────────────────

void start(const Config& cfg) { runLauncher(cfg); }

struct StartTaskArg
{
    Config cfg;
};

void startTask(const Config& cfg, uint32_t stack_depth, uint8_t priority)
{
    auto* arg = new StartTaskArg{cfg};
    xTaskCreate(
        [](void* a)
        {
            auto* ta = static_cast<StartTaskArg*>(a);
            Config local_cfg = ta->cfg;
            delete ta;
            runLauncher(local_cfg);
            vTaskDelete(nullptr);
        },
        "launcher", stack_depth, arg, priority, nullptr);
}

}  // namespace launcher
