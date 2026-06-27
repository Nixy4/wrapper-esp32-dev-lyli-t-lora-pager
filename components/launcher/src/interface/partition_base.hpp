#pragma once

#include <cstdint>
#include <functional>
#include <string>
#include <string_view>
#include <vector>

namespace launcher
{

/// 标识待写入的 Flash 分区类型。
/// 对应 Reference 中的 LauncherUpdateTarget 枚举。
enum class FlashTarget : uint8_t
{
    App,     ///< OTA 应用分区（写入时校验 esp_image magic）
    Spiffs,  ///< SPIFFS 数据分区
    FatVfs,  ///< label 为 "vfs" 的 FAT 数据分区
    FatSys,  ///< label 为 "sys" 的 FAT 数据分区
};

/// 分区描述符（镜像 esp_partition_t 关键字段）。
struct PartitionInfo
{
    std::string label;
    uint8_t type = 0xFF;
    uint8_t subtype = 0xFF;
    uint32_t offset = 0;
    uint32_t size = 0;
    bool encrypted = false;
};

/// Flash 写入操作的进度回调类型。
/// 对应 Reference 中的 LauncherUpdateProgress。
using FlashProgressCb = std::function<void(size_t written, size_t total)>;

/// Flash 分区管理抽象接口。
/// 覆盖：分区枚举、OTA 写入、原始 I/O、分区表修改和分区转储/恢复。
/// 对应 Reference 中的 idf_update.h + partitioner.h + partition_table_model.h。
class PartitionBase
{
   public:
    virtual ~PartitionBase() = default;

    // ---- 枚举 ----

    virtual std::vector<PartitionInfo> ListAll() const = 0;
    virtual std::vector<PartitionInfo> ListApps() const = 0;
    virtual bool FindByLabel(std::string_view label, PartitionInfo& out) const = 0;

    // ---- 容量查询 ----
    // 对应 Reference 中的 MAX_APP / MAX_SPIFFS / MAX_FAT_vfs / MAX_FAT_sys。

    virtual uint32_t MaxAppSize() const = 0;
    virtual uint32_t MaxSpiffsSize() const = 0;
    virtual uint32_t MaxFatVfsSize() const = 0;
    virtual uint32_t MaxFatSysSize() const = 0;

    // ---- Flash 写入（带 OTA 校验）----
    // 对应 Reference 中的 launcherUpdateStream / launcherRawUpdateStream。

    virtual bool FlashFromBuffer(FlashTarget target,
                                 const uint8_t* data,
                                 size_t size,
                                 FlashProgressCb cb = nullptr) = 0;

    // ---- 原始 Flash I/O（绕过 OTA 校验）----

    virtual bool EraseRegion(uint32_t offset, size_t size) = 0;
    virtual bool WriteRaw(uint32_t offset, const uint8_t* data, size_t len) = 0;
    virtual bool ReadRaw(uint32_t offset, uint8_t* buf, size_t len) const = 0;

    // ---- OTA 启动管理 ----

    virtual bool SetBootPartition(std::string_view label) = 0;
    virtual std::string RunningPartitionLabel() const = 0;
    virtual std::string NextOtaPartitionLabel() const = 0;

    // ---- 分区表修改 ----
    // 对应 Reference 中的 launcherUpdateRepairPartitionTable + wrapper PartitionManager。

    virtual bool CreateOtaPartition(uint32_t image_size,
                                    std::string_view label,
                                    PartitionInfo* created = nullptr) = 0;

    virtual bool CreateDataPartition(uint32_t size,
                                     uint8_t subtype,
                                     std::string_view label,
                                     PartitionInfo* created = nullptr) = 0;

    virtual bool RemovePartition(std::string_view label) = 0;

    /// 将内存中修改后的分区表持久化回 Flash。
    virtual bool CommitPartitionTable() = 0;

    // ---- 分区转储 / 恢复 ----
    // 对应 Reference 中的 dumpPartition() / restorePartition()。

    virtual bool DumpPartition(std::string_view label, std::string_view file_path) = 0;
    virtual bool RestorePartition(std::string_view label, std::string_view file_path) = 0;
};

}  // namespace launcher
