#pragma once

#include <functional>
#include <string>
#include <algorithm>
#include <cstdio>
#include <cstring>

#include <esp_log.h>

#include "hal/partition_base.hpp"
#include "core/app_registry.hpp"

namespace launcher::core
{

/**
 * @brief 进度通知回调：(阶段描述, 已写入字节数, 总字节数)。
 *        total_bytes == 0 表示总量尚未知。
 */
using InstallProgressCb = std::function<void(const char* stage, size_t written, size_t total)>;

/**
 * @brief 将 SD 卡上的固件二进制安装到 OTA 槽位。（CRTP 零开销模板版本）
 *
 * @tparam StorageT    实现 StorageBase<StorageT> 接口约束的存储类型。
 * @tparam PartitionT  实现 PartitionBase<PartitionT> 接口约束的分区类型。
 *
 * 典型调用时序：
 *   Install(sd_path, display_name, progress_cb)
 *   → planInstall / flashBegin / flashWrite / flashEnd
 *   → 将（分区标签 → display_name）保存到 AppRegistry
 */
template <typename StorageT, typename PartitionT>
class SdInstaller
{
    StorageT& storage_;
    PartitionT& partition_;
    AppRegistry<StorageT>& registry_;

    static constexpr size_t kChunkSize = 4096;
    static constexpr const char* kTag = "Launcher|SdInstaller";

   public:
    SdInstaller(StorageT& storage, PartitionT& partition, AppRegistry<StorageT>& registry)
        : storage_(storage), partition_(partition), registry_(registry)
    {
    }

    /**
     * @brief 从 SD 卡安装 .bin 固件。
     *
     * @param sd_path      VFS 完整路径（如 "/sdcard/myapp.bin"）。
     * @param display_name 用户可见的应用名称（存入 NVS）。
     * @param progress     可选的进度回调。
     * @return 成功返回 true；任何错误自动中止 OTA 并返回 false。
     */
    bool Install(const char* sd_path,
                 const char* display_name,
                 InstallProgressCb progress = nullptr)
    {
        // ── 1. 打开文件 ──────────────────────────────────────────────────────
        size_t file_size = 0;
        if (!storage_.SdFileSize(sd_path, file_size) || file_size == 0)
        {
            ESP_LOGE(kTag, "Cannot stat '%s'", sd_path);
            return false;
        }
        ESP_LOGI(kTag, "Installing '%s' (%zu B) as '%s'", sd_path, file_size, display_name);

        FILE* f = storage_.SdOpenRead(sd_path);
        if (!f)
        {
            ESP_LOGE(kTag, "Cannot open '%s' for reading", sd_path);
            return false;
        }

        // ── 2. 选择 OTA 槽位 ────────────────────────────────────────────────
        if (progress)
            progress("Selecting slot…", 0, file_size);

        hal::InstallSlot slot;
        if (!partition_.PlanInstall(nullptr, file_size, slot))
        {
            ESP_LOGE(kTag, "No suitable OTA slot for %zu B", file_size);
            storage_.SdClose(f);
            return false;
        }

        // ── 3. 开始 OTA 会话 ────────────────────────────────────────────────
        if (progress)
            progress("Erasing…", 0, file_size);

        if (!partition_.FlashBegin(slot, file_size))
        {
            ESP_LOGE(kTag, "flashBegin failed");
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
                ESP_LOGE(kTag, "Read error at offset %zu", written);
                ok = false;
                break;
            }

            if (!partition_.FlashWrite(buf, got))
            {
                ESP_LOGE(kTag, "flashWrite failed at offset %zu", written);
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

        // ── 5. 终结 OTA 写入 ────────────────────────────────────────────────
        if (progress)
            progress("Verifying…", written, file_size);

        if (!partition_.FlashEnd())
        {
            ESP_LOGE(kTag, "flashEnd failed (image invalid?)");
            return false;
        }

        // ── 6. 将应用写入 NVS 注册表 ────────────────────────────────────────
        AppInfo info{slot.label, display_name};
        if (!registry_.Save(info))
            ESP_LOGE(kTag, "保存应用注册表条目失败");  // 非致命错误

        if (progress)
            progress("Done", file_size, file_size);

        ESP_LOGI(kTag, "Installation complete: '%s' → '%s'", slot.label, display_name);
        return true;
    }
};

}  // namespace launcher::core
