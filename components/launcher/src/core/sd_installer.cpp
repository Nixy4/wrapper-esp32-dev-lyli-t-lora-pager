#include "sd_installer.hpp"

#include <cstring>
#include <esp_log.h>

static const char* TAG = "Launcher|SdInstaller";

namespace launcher::core
{

SdInstaller::SdInstaller(hal::IStorage& storage, hal::IPartition& partition, AppRegistry& registry)
    : storage_(storage),
      partition_(partition),
      registry_(registry)
{
}

bool SdInstaller::Install(const char* sd_path, const char* display_name, InstallProgressCb progress)
{
    // ── 1. 打开文件 ────────────────────────────────────────────────────────────
    size_t file_size = 0;
    if (!storage_.SdFileSize(sd_path, file_size) || file_size == 0)
    {
        ESP_LOGE(TAG, "Cannot stat '%s'", sd_path);
        return false;
    }
    ESP_LOGI(TAG, "Installing '%s' (%zu B) as '%s'", sd_path, file_size, display_name);

    FILE* f = storage_.SdOpenRead(sd_path);
    if (!f)
    {
        ESP_LOGE(TAG, "Cannot open '%s' for reading", sd_path);
        return false;
    }

    // ── 2. 选择 OTA 槽位 ───────────────────────────────────────────────────
    if (progress)
        progress("Selecting slot…", 0, file_size);

    hal::InstallSlot slot;
    if (!partition_.PlanInstall(nullptr, file_size, slot))
    {
        ESP_LOGE(TAG, "No suitable OTA slot for %zu B", file_size);
        storage_.SdClose(f);
        return false;
    }

    // ── 3. 开始 OTA 会话 ─────────────────────────────────────────────────
    if (progress)
        progress("Erasing…", 0, file_size);

    if (!partition_.FlashBegin(slot, file_size))
    {
        ESP_LOGE(TAG, "flashBegin failed");
        storage_.SdClose(f);
        return false;
    }

    // ── 4. 流式传输固件块 ────────────────────────────────────────────────
    static uint8_t buf[kChunkSize];
    size_t written = 0;
    bool ok = true;

    while (written < file_size)
    {
        size_t to_read = std::min(kChunkSize, file_size - written);
        size_t got = fread(buf, 1, to_read, f);
        if (got == 0)
        {
            ESP_LOGE(TAG, "Read error at offset %zu", written);
            ok = false;
            break;
        }

        if (!partition_.FlashWrite(buf, got))
        {
            ESP_LOGE(TAG, "flashWrite failed at offset %zu", written);
            ok = false;
            break;
        }

        written += got;
        if (progress)
            progress("Writing…", written, file_size);
    }

    storage_.SdClose(f);

    if (!ok)
    {
        partition_.FlashAbort();
        return false;
    }

    // ── 5. 终结 OTA 写入 ─────────────────────────────────────────────────
    if (progress)
        progress("Verifying…", written, file_size);

    if (!partition_.FlashEnd())
    {
        ESP_LOGE(TAG, "flashEnd failed (image invalid?)");
        return false;
    }

    // ── 6. 将应用写入 NVS 注册表 ────────────────────────────────────────
    AppInfo info{slot.label, display_name};
    if (!registry_.Save(info))
    {
        ESP_LOGE(TAG, "保存应用注册表条目失败");
        // 非致命错误——分区已正确刷入
    }

    if (progress)
        progress("Done", file_size, file_size);

    ESP_LOGI(TAG, "Installation complete: '%s' → '%s'", slot.label, display_name);
    return true;
}

}  // namespace launcher::core
