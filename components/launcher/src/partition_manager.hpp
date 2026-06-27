#pragma once

#include "app_info.hpp"
#include "interface/partition_base.hpp"
#include "interface/storage_base.hpp"

#include <functional>
#include <string>
#include <string_view>
#include <vector>

namespace launcher
{

using InstallProgressCb = std::function<void(size_t written, size_t total)>;

/// Launcher 层的分区生命周期管理器。
/// 编排应用安装、移除和分区表更新等高层操作。
/// 对应 Reference 中 partitioner.cpp + partition_install_layout.cpp 的职责。
class PartitionManager
{
   public:
    /// @param partition  底层分区接口。
    /// @param storage    存储接口，用于打开源二进制文件。
    explicit PartitionManager(PartitionBase& partition, StorageBase& storage);
    ~PartitionManager();

    // ---- 从文件安装 ----

    /// 从存储上的文件路径安装应用二进制。
    /// 若有空间则创建新 OTA 分区并写入镜像。
    /// 成功后将创建的 AppInfo 写入 installed。
    bool InstallAppFromFile(std::string_view file_path,
                            std::string_view display_name,
                            AppInfo& installed,
                            InstallProgressCb cb = nullptr);

    /// 从文件安装数据分区镜像（SPIFFS / FAT）。
    bool InstallDataFromFile(std::string_view file_path,
                             FlashTarget target,
                             InstallProgressCb cb = nullptr);

    // ---- 移除 ----

    /// 擦除 OTA 分区内容并从分区表中移除其条目。
    bool RemoveAppPartition(std::string_view label);

    /// 按标签移除数据分区（FAT / SPIFFS）条目。
    bool RemoveDataPartition(std::string_view label);

    // ---- 扫描 ----
    // 对应 Reference 中的 partitionCrawler()。

    /// 扫描所有 OTA 分区中的有效 esp_image 头。
    /// 返回填充了 label、size、version 字段的 AppInfo 列表。
    std::vector<AppInfo> CrawlInstalledApps() const;

    // ---- 转储 / 恢复 ----
    // 对应 Reference 中的 dumpPartition() / restorePartition()。

    /// 将分区镜像备份到存储上的文件。
    bool DumpPartition(std::string_view label, std::string_view dest_path);

    /// 从存储上的文件恢复分区镜像。
    bool RestorePartition(std::string_view label, std::string_view src_path);

    // ---- 空间估算 ----

    /// 估算是否有足够的空闲 Flash 空间容纳指定大小的应用。
    bool CanFitApp(uint32_t size) const;

    /// 估算是否有足够的空闲 Flash 空间容纳指定大小的数据分区。
    bool CanFitData(uint32_t size) const;

   private:
    PartitionBase& partition_;
    StorageBase& storage_;
};

}  // namespace launcher
