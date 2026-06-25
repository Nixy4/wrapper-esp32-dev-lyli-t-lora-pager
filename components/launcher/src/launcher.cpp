#include "launcher.hpp"

#include <cstring>
#include <esp_log.h>
#include <esp_system.h>
#include <nvs_flash.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

// ── 板级支持（根据 Kconfig
// 条件编译）───────────────────────────────────────────────────────────────────────────────────────────────
#if defined(CONFIG_LAUNCHER_BOARD_LILYGO_T_LORA_PAGER)
#include "board/lilygo/t-lora-pager.hpp"
#include "driver/gpio.h"
// T-LoRa Pager SD 卡 CS 引脚（原理图：SDCARD_CS = GPIO21）
static constexpr gpio_num_t kSdCsPin = GPIO_NUM_21;
// 显示屏物理分辨率
static constexpr int kDisplayW = 480;
static constexpr int kDisplayH = 222;
#else
#error "No launcher board selected.  Set CONFIG_LAUNCHER_BOARD_* in Kconfig."
#endif

// ── OSAL / HAL / Core / UI 头文件包含
// ───────────────────────────────────────────────────────────────────────────────────────────────────
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

// ──
// 模块级全局变量（文件作用域）───────────────────────────────────────────────────────────────────────────────────────────────────────
// 由屏幕辅助函数（screen_sd_browser.cpp、screen_settings.cpp）通过 extern 声明访问。
// screen_settings.cpp) via extern declarations.  They are valid for the
// 生命周期与 launcher 任务相同。

// ── 亮度设置辅助函数（由 screen_settings.cpp
// 前向声明）─────────────────────────────────────────────────────

void LauncherSetBrightness(int pct)
{
#if defined(CONFIG_LAUNCHER_BOARD_LILYGO_T_LORA_PAGER)
    wrapper::LilyGoLoraPager::GetInstance().SetDisplayBrightness(pct);
#endif
}

// ─────────────────────────────────────────────────────────────────────────────
// ─────────────────────────────────────────────────────────────────────────────
//  核心实现
// ─────────────────────────────────────────────────────────────────────────────
// ─────────────────────────────────────────────────────────────────────────────

namespace launcher
{

static void RunLauncher(const Config& cfg)
{
    using Storage_t = hal::StorageEsp32;
    using Display_t = hal::DisplayEsp32;
    using Input_t = hal::InputEsp32;
    using Partition_t = hal::PartitionEsp32;
    using Time_t = osal::FreeRtosTime;
    using Registry_t = core::AppRegistry<Storage_t>;
    using Boot_t = core::BootManager<Partition_t, Storage_t, Time_t>;
    using Installer_t = core::SdInstaller<Storage_t, Partition_t>;
    using ScreenMgr_t = ui::ScreenManager<Display_t, Input_t>;
    using AppList_t = ui::ScreenAppList<ScreenMgr_t, Registry_t, Boot_t, Installer_t>;

    // ── 1. 板级硬件初始化
    // ─────────────────────────────────────────────────────────────────────────────────────────────────────────────────
    ESP_LOGI(TAG, "初始化板级硬件...");

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

    if (!board.InitEncoder())
        ESP_LOGW(TAG, "InitEncoder failed — encoder disabled");
#endif

    // ── 2. NVS Flash 初始化
    // ─────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
    esp_err_t nvs_err = nvs_flash_init();
    if (nvs_err == ESP_ERR_NVS_NO_FREE_PAGES || nvs_err == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_LOGW(TAG, "NVS 需要擦除 — 重新格式化");
        nvs_flash_erase();
        nvs_flash_init();
    }

    // ── 3. 创建 OSAL / HAL 层
    // ─────────────────────────────────────────────────────────────────────────────────────────────────────
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

    // ── 4. 显示方向设置
    // ────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
    display_hal.SetRotation(LV_DISPLAY_ROTATION_180);
    board.SetDisplayBrightness(cfg.brightness);

    // ── 5. 创建 Core 层
    // ────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
    Registry_t app_registry(storage_hal);
    Boot_t boot_mgr(partition_hal, storage_hal, time_impl, app_registry);
    Installer_t sd_installer(storage_hal, partition_hal, app_registry);

    // ── 6. 挂载 SD 卡（尽力而为——可能未插入） ─────────────────
    if (!storage_hal.SdMount())
        ESP_LOGW(TAG, "SD card not present at startup");

    // ── 7. 启动决策
    // ───────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
    if (cfg.boot_to_last_app)
    {
        std::string last_label;
        if (!boot_mgr.ShouldShowMenu(last_label))
        {
            ESP_LOGI(TAG, "Auto-booting '%s'", last_label.c_str());
            boot_mgr.BootApp(last_label);
            // bootApp 返回（setBootPartition 失败），继续显示 UI
            ESP_LOGW(TAG, "自动引导失败 — 显示 Launcher 菜单");
        }
    }

    // ── 8. UI 初始化
    // ───────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
    ScreenMgr_t screen_mgr(display_hal, input_hal);

    auto* app_list_screen = new AppList_t(screen_mgr, app_registry, boot_mgr, sd_installer);
    screen_mgr.Push(
        app_list_screen->Screen(), [app_list_screen]() { delete app_list_screen; },
        [app_list_screen]() { app_list_screen->Activate(); });

    // ── 9. 输入轮询任务
    // ───────────────────────────────────────────────────────────────────────────────────────────────────────────────────
    struct InputTaskArg
    {
        Input_t* input;
    };
    auto* input_arg = new InputTaskArg{&input_hal};

    xTaskCreate(
        [](void* arg)
        {
            auto* a = static_cast<InputTaskArg*>(arg);
            while (true)
            {
                a->input->Poll();
                vTaskDelay(pdMS_TO_TICKS(10));
            }
        },
        "lnch_input", cfg.input_task_stack, input_arg, cfg.input_task_prio, nullptr);

    // ── 10. 主循环
    // ─────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
    ESP_LOGI(TAG, "Launcher 已启动运行");
    while (true)
    {
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// ─────────────────────────────────────────────────────────────────────────────
//  公共 API
// ─────────────────────────────────────────────────────────────────────────────
// ─────────────────────────────────────────────────────────────────────────────

void start(const Config& cfg) { RunLauncher(cfg); }

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
            RunLauncher(local_cfg);
            vTaskDelete(nullptr);
        },
        "launcher", stack_depth, arg, priority, nullptr);
}

}  // namespace launcher
