#include "sd_installer.hpp"

#include <cstring>
#include <esp_log.h>

static const char* TAG = "Launcher|SdInstaller";

namespace launcher::core
{

SdInstaller::SdInstaller(hal::IStorage& storage, hal::IPartition& partition, AppRegistry& registry)
    : storage_(storage), partition_(partition), registry_(registry)
{
}

bool SdInstaller::install(const char* sd_path, const char* display_name, InstallProgressCb progress)
{
    // ── 1. Open file ──────────────────────────────────────────────────────────
    size_t file_size = 0;
    if (!storage_.sdFileSize(sd_path, file_size) || file_size == 0)
    {
        ESP_LOGE(TAG, "Cannot stat '%s'", sd_path);
        return false;
    }
    ESP_LOGI(TAG, "Installing '%s' (%zu B) as '%s'", sd_path, file_size, display_name);

    FILE* f = storage_.sdOpenRead(sd_path);
    if (!f)
    {
        ESP_LOGE(TAG, "Cannot open '%s' for reading", sd_path);
        return false;
    }

    // ── 2. Select OTA slot ───────────────────────────────────────────────────
    if (progress)
        progress("Selecting slot…", 0, file_size);

    hal::InstallSlot slot;
    if (!partition_.planInstall(nullptr, file_size, slot))
    {
        ESP_LOGE(TAG, "No suitable OTA slot for %zu B", file_size);
        storage_.sdClose(f);
        return false;
    }

    // ── 3. Begin OTA session ─────────────────────────────────────────────────
    if (progress)
        progress("Erasing…", 0, file_size);

    if (!partition_.flashBegin(slot, file_size))
    {
        ESP_LOGE(TAG, "flashBegin failed");
        storage_.sdClose(f);
        return false;
    }

    // ── 4. Stream firmware chunks ────────────────────────────────────────────
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

        if (!partition_.flashWrite(buf, got))
        {
            ESP_LOGE(TAG, "flashWrite failed at offset %zu", written);
            ok = false;
            break;
        }

        written += got;
        if (progress)
            progress("Writing…", written, file_size);
    }

    storage_.sdClose(f);

    if (!ok)
    {
        partition_.flashAbort();
        return false;
    }

    // ── 5. Finalise OTA write ────────────────────────────────────────────────
    if (progress)
        progress("Verifying…", written, file_size);

    if (!partition_.flashEnd())
    {
        ESP_LOGE(TAG, "flashEnd failed (image invalid?)");
        return false;
    }

    // ── 6. Register in NVS ───────────────────────────────────────────────────
    AppInfo info{slot.label, display_name};
    if (!registry_.save(info))
    {
        ESP_LOGE(TAG, "Failed to save app registry entry");
        // Not fatal — partition is flashed correctly
    }

    if (progress)
        progress("Done", file_size, file_size);

    ESP_LOGI(TAG, "Installation complete: '%s' → '%s'", slot.label, display_name);
    return true;
}

}  // namespace launcher::core
