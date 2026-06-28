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
#include "wrapper/logger.hpp"
#include "board/lilygo/t-lora-pager.hpp"

using namespace wrapper;

Logger l_main("LoraPager", "Init");

struct ScanEntry
{
    std::string name;
    std::string full_path;
    bool is_dir;
    long long size;
};

// 递归扫描目录，以 tree 命令风格输出文件树
// prefix: 父级传下来的前缀字符串（空行 / "│   " 等）
static void scan_dir(const char* path, const std::string& prefix = "")
{
    DIR* dir = opendir(path);
    if (!dir)
    {
        l_main.Warning("Cannot open dir: %s", path);
        return;
    }

    // 收集全部条目
    std::vector<ScanEntry> entries;
    struct dirent* de;
    while ((de = readdir(dir)) != nullptr)
    {
        if (strcmp(de->d_name, ".") == 0 || strcmp(de->d_name, "..") == 0)
            continue;

        char full[512];
        snprintf(full, sizeof(full), "%s/%s", path, de->d_name);

        struct stat st;
        if (stat(full, &st) != 0)
            continue;

        ScanEntry e;
        e.name = de->d_name;
        e.full_path = full;
        e.is_dir = S_ISDIR(st.st_mode);
        e.size = e.is_dir ? 0LL : static_cast<long long>(st.st_size);
        entries.push_back(std::move(e));
    }
    closedir(dir);

    // 目录在前，同类按名称排序
    std::sort(entries.begin(), entries.end(),
              [](const ScanEntry& a, const ScanEntry& b)
              {
                  if (a.is_dir != b.is_dir)
                      return a.is_dir > b.is_dir;
                  return a.name < b.name;
              });

    for (size_t i = 0; i < entries.size(); ++i)
    {
        const auto& e = entries[i];
        bool is_last = (i == entries.size() - 1);

        const char* branch = is_last ? "\xe2\x94\x94\xe2\x94\x80\xe2\x94\x80 "   // └──
                                     : "\xe2\x94\x9c\xe2\x94\x80\xe2\x94\x80 ";  // ├──
        const char* child_ext = is_last ? "    " : "\xe2\x94\x82   ";            // "    " / "│   "

        if (e.is_dir)
        {
            l_main.Info("%s%s%s/", prefix.c_str(), branch, e.name.c_str());
            scan_dir(e.full_path.c_str(), prefix + child_ext);
        }
        else
        {
            l_main.Info("%s%s%s  (%lld B)", prefix.c_str(), branch, e.name.c_str(), e.size);
        }
    }
}

// ---------------------------------------------------------------------------
// SD 卡文件系统操作测试
// ---------------------------------------------------------------------------
static void test_sdcard_fs(const SdSpi& sd)
{
    const std::string root = sd.GetBasePath();
    const std::string test_file = root + "/test_wrapper.txt";
    const std::string test_dir = root + "/test_dir";
    const std::string sub_file = test_dir + "/sub.txt";
    const char* kContent = "Hello from ESP32-S3!\nWrapper FS test OK.\n";
    int pass = 0, fail = 0;

    // --- 卡状态查询（SdSpi 接口） ---
    l_main.Info("[FS] card status: %s", sd.GetStatus() ? "OK" : "FAIL");
    l_main.Info("[FS] CanDiscard=%s  CanTrim=%s", sd.CanDiscard() ? "yes" : "no",
                sd.CanTrim() ? "yes" : "no");

    // 1. 写文件
    {
        FILE* f = fopen(test_file.c_str(), "w");
        if (f && fputs(kContent, f) >= 0)
        {
            fclose(f);
            l_main.Info("[FS] PASS  (1) write");
            pass++;
        }
        else
        {
            if (f)
                fclose(f);
            l_main.Error("[FS] FAIL  (1) write  errno=%d", errno);
            fail++;
        }
    }

    // 2. 读回并校验
    {
        FILE* f = fopen(test_file.c_str(), "r");
        if (f)
        {
            char buf[128] = {};
            size_t n = fread(buf, 1, sizeof(buf) - 1, f);
            fclose(f);
            if (n > 0 && strncmp(buf, kContent, strlen(kContent)) == 0)
            {
                l_main.Info("[FS] PASS  (2) read & verify  (%zu B)", n);
                pass++;
            }
            else
            {
                l_main.Error("[FS] FAIL  (2) read mismatch  got %zu B", n);
                fail++;
            }
        }
        else
        {
            l_main.Error("[FS] FAIL  (2) open for read  errno=%d", errno);
            fail++;
        }
    }

    // 3. 追加写
    {
        FILE* f = fopen(test_file.c_str(), "a");
        if (f && fputs("Append line.\n", f) >= 0)
        {
            fclose(f);
            l_main.Info("[FS] PASS  (3) append");
            pass++;
        }
        else
        {
            if (f)
                fclose(f);
            l_main.Error("[FS] FAIL  (3) append  errno=%d", errno);
            fail++;
        }
    }

    // 4. stat 查询文件大小
    {
        struct stat st;
        if (stat(test_file.c_str(), &st) == 0)
        {
            l_main.Info("[FS] PASS  (4) stat  size=%lld B", static_cast<long long>(st.st_size));
            pass++;
        }
        else
        {
            l_main.Error("[FS] FAIL  (4) stat  errno=%d", errno);
            fail++;
        }
    }

    // 5. 创建子目录
    if (mkdir(test_dir.c_str(), 0775) == 0 || errno == EEXIST)
    {
        l_main.Info("[FS] PASS  (5) mkdir");
        pass++;
    }
    else
    {
        l_main.Error("[FS] FAIL  (5) mkdir  errno=%d", errno);
        fail++;
    }

    // 6. 在子目录中写文件
    {
        FILE* f = fopen(sub_file.c_str(), "w");
        if (f && fputs("subdir file\n", f) >= 0)
        {
            fclose(f);
            l_main.Info("[FS] PASS  (6) write in subdir");
            pass++;
        }
        else
        {
            if (f)
                fclose(f);
            l_main.Error("[FS] FAIL  (6) write in subdir  errno=%d", errno);
            fail++;
        }
    }

    // 7. 清理：删除测试文件与目录
    remove(sub_file.c_str());
    rmdir(test_dir.c_str());
    if (remove(test_file.c_str()) == 0)
    {
        l_main.Info("[FS] PASS  (7) cleanup");
        pass++;
    }
    else
    {
        l_main.Error("[FS] FAIL  (7) cleanup  errno=%d", errno);
        fail++;
    }

    l_main.Info("[FS] Result: %d passed, %d failed", pass, fail);
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
    board.InitSdCard();
    board.InitBootButton();

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
    xTaskCreate(task_fn_board_init, "board_init_task", 16384, nullptr, 5, nullptr);
}
