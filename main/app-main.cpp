#include "sdkconfig.h"
#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

//! Application entry point
//! ----------------------------------------------------------------------------------------
#include "local/logger.hpp"
#include "local/spiffs.hpp"
#include "board/lilygo/t-lora-pager.hpp"
#include "registry/elf_loader.hpp"

using namespace wrapper;

Logger l_main("LoraPager", "Init");

SpiffsConfig spiffs_cfg("/storage", "storage", 5, false);
Spiffs spiffs(l_main);

ElfLoader elf_loader(l_main);

void task_fn_elf_loader_test(void* arg)
{
    if (!elf_loader.Init())
    {
        l_main.Error("ElfLoader init failed");
        return;
    }

    // 从spiffs加载ELF文件, 并运行

    if (!elf_loader.LoadFromSpiffs("hello_world.app.elf"))
    {
        l_main.Error("Failed to load hello_world.app.elf from /storage");
        return;
    }

    if (!elf_loader.Run())
    {
        l_main.Error("Failed to run hello_world.app.elf");
        return;
    }

    // 从SD卡加载ELF文件, 并运行
    if (!elf_loader.LoadFromSdCard("hello_world.app.elf"))
    {
        l_main.Error("Failed to load hello_world.app.elf from /sdcard");
        return;
    }

    if (!elf_loader.Run())
    {
        l_main.Error("Failed to run hello_world.app.elf from /sdcard");
        return;
    }

    vTaskDelete(nullptr);
}

// ---------------------------------------------------------------------------
void task_fn_board_init(void* arg)
{
    auto& board = LilyGoLoraPager::GetInstance();

    board.InitCoreBusAndIoExpander();
    board.InitDisplay();
    board.InitAudio();
    board.InitKeyboard();
    board.InitEncoder();

    spiffs.Init(spiffs_cfg);
    board.InitSdCard();

    board.InitBootButton();

    xTaskCreate(task_fn_elf_loader_test, "elf_loader_test_task", 8192, nullptr, 5, nullptr);

    for (;;)
    {
        board.BootButtonHandler();
        board.KeyboardHandler();
        vTaskDelay(pdMS_TO_TICKS(10));
    }

    vTaskDelete(nullptr);
}

extern "C" void app_main(void)
{
    xTaskCreate(task_fn_board_init, "board_init_task", 8192, nullptr, 5, nullptr);
}
