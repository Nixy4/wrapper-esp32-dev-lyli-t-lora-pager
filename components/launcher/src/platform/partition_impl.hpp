#pragma once

#include "interface/partition_base.hpp"
#include "wrapper/logger.hpp"
#include "wrapper/ota.hpp"
#include "wrapper/partition.hpp"

namespace launcher
{

/// PartitionBase 的 ESP-IDF 具体实现。
/// 将操作委托给:
///   - wrapper::OtaManager    — OTA 应用/数据写入
///   - wrapper::PartitionManager — 分区表读取/修改/写入
class PartitionImpl : public PartitionBase
{
   public:
    /// @param ota        板级 wrapper 中的 OTA 管理器（提供 Begin/Write/End）。
    /// @param partition  板级 wrapper 中的分区表管理器。
    /// @param logger     日志实例。
    PartitionImpl(wrapper::OtaManager& ota,
                  wrapper::PartitionManager& partition,
                  wrapper::Logger& logger);
    ~PartitionImpl() override = default;

    // ---- 枚举 ----

    std::vector<PartitionInfo> ListAll() const override;
    std::vector<PartitionInfo> ListApps() const override;
    bool FindByLabel(std::string_view label, PartitionInfo& out) const override;

    // ---- 容量查询 ----

    uint32_t MaxAppSize() const override;
    uint32_t MaxSpiffsSize() const override;
    uint32_t MaxFatVfsSize() const override;
    uint32_t MaxFatSysSize() const override;

    // ---- Flash 写入 ----

    bool FlashFromBuffer(FlashTarget target,
                         const uint8_t* data,
                         size_t size,
                         FlashProgressCb cb = nullptr) override;

    // ---- 原始 Flash I/O ----

    bool EraseRegion(uint32_t offset, size_t size) override;
    bool WriteRaw(uint32_t offset, const uint8_t* data, size_t len) override;
    bool ReadRaw(uint32_t offset, uint8_t* buf, size_t len) const override;

    // ---- OTA 启动管理 ----

    bool SetBootPartition(std::string_view label) override;
    std::string RunningPartitionLabel() const override;
    std::string NextOtaPartitionLabel() const override;

    // ---- 分区表修改 ----

    bool CreateOtaPartition(uint32_t image_size,
                            std::string_view label,
                            PartitionInfo* created = nullptr) override;

    bool CreateDataPartition(uint32_t size,
                             uint8_t subtype,
                             std::string_view label,
                             PartitionInfo* created = nullptr) override;

    bool RemovePartition(std::string_view label) override;
    bool CommitPartitionTable() override;

    // ---- 分区转储 / 恢复 ----

    bool DumpPartition(std::string_view label, std::string_view file_path) override;
    bool RestorePartition(std::string_view label, std::string_view file_path) override;

   private:
    wrapper::OtaManager& ota_;
    wrapper::PartitionManager& partition_;
    wrapper::Logger& logger_;
};

}  // namespace launcher
