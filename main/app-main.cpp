#include "sdkconfig.h"
#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <algorithm>
#include <cerrno>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

//! Application entry point
//! ----------------------------------------------------------------------------------------
#include "local/logger.hpp"
#include "board/lilygo/t-lora-pager.hpp"
#include "registry/elf_loader.hpp"

using namespace wrapper;

Logger l_main("LoraPager", "Init");
ElfLoader elf_loader(l_main);

// ---------------------------------------------------------------------------
void task_fn_board_init(void* arg)
{
    elf_loader.Init();

    auto& board = LilyGoLoraPager::GetInstance();
    board.InitBootButton();
    board.InitCoreBusAndIoExpander();
    board.InitDisplay();
    board.InitAudio();
    board.InitKeyboard();
    board.InitEncoder();
    board.InitSdCard();

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
