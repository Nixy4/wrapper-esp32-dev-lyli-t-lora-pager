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
#include "local/spiffs.hpp"
#include "board/lilygo/t-lora-pager.hpp"
#include "registry/elf_loader.hpp"

using namespace wrapper;

Logger l_main("LoraPager", "Init");

SpiffsConfig spiffs_cfg("/storage", "storage", 5, false);
Spiffs spiffs(l_main);

ElfLoader elf_loader(l_main);

bool test_elf_loader_load_from_sdcard(ElfLoader& loader, const char* path)
{
    FILE* fp = fopen(path, "rb");
    if (!fp)
    {
        l_main.Error("Failed to open ELF on SD card: %s (errno=%d)", path, errno);
        return false;
    }

    if (fseek(fp, 0, SEEK_END) != 0)
    {
        l_main.Error("fseek end failed for %s", path);
        fclose(fp);
        return false;
    }

    long size = ftell(fp);
    if (size <= 0)
    {
        l_main.Error("Invalid ELF size (%ld) for %s", size, path);
        fclose(fp);
        return false;
    }

    if (fseek(fp, 0, SEEK_SET) != 0)
    {
        l_main.Error("fseek set failed for %s", path);
        fclose(fp);
        return false;
    }

    std::vector<uint8_t> elf_data(static_cast<size_t>(size));
    size_t read_bytes = fread(elf_data.data(), 1, elf_data.size(), fp);
    fclose(fp);

    if (read_bytes != elf_data.size())
    {
        l_main.Error("Short read for %s: read=%zu expected=%zu", path, read_bytes, elf_data.size());
        return false;
    }

    if (elf_data.size() < 4 || elf_data[0] != 0x7f || elf_data[1] != 'E' || elf_data[2] != 'L' ||
        elf_data[3] != 'F')
    {
        l_main.Error("Invalid ELF magic in %s", path);
        return false;
    }

    l_main.Info("Read ELF from SD card: %s (%zu bytes)", path, elf_data.size());

    if (!loader.Load(elf_data.data(), elf_data.size()))
    {
        l_main.Error("ElfLoader::Load failed for SD file %s", path);
        return false;
    }

    l_main.Info("ELF relocation test from SD card passed: %s", path);
    return true;
}

void task_fn_elf_loader_test(void* arg)
{
    auto& board = LilyGoLoraPager::GetInstance();

    if (!elf_loader.Init())
    {
        l_main.Error("ElfLoader init failed");
        return;
    }

    // 从spiffs加载ELF文件, 并运行

    // if (!elf_loader.LoadFromFile("hello_world.app.elf"))
    // {
    //     l_main.Error("Failed to load hello_world.app.elf from /storage");
    //     return;
    // }

    // if (!elf_loader.Run())
    // {
    //     l_main.Error("Failed to run hello_world.app.elf");
    //     return;
    // }

    // 从SD卡加载ELF文件, 并运行
    if (!test_elf_loader_load_from_sdcard(elf_loader, "/sdcard/hello_world.app.elf"))
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
