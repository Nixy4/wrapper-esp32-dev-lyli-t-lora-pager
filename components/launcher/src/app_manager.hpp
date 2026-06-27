#pragma once

#include "app_info.hpp"
#include "interface/partition_base.hpp"

#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace launcher
{

/// 已安装应用注册表管理器（NVS）及 OTA 启动选择。
/// 对应 Reference 中 app_registry.cpp 的职责。
class AppManager
{
   public:
    /// @param partition  分区接口，用于启动标签查询和 OTA 操作。
    explicit AppManager(PartitionBase& partition);
    ~AppManager();

    // ---- 注册表（NVS）----

    /// 从 NVS 加载所有已注册的应用元数据。
    std::vector<AppInfo> LoadRegistry() const;

    /// 按标签插入或更新 NVS 中的应用元数据。
    bool Save(const AppInfo& app);

    /// 从 NVS 中删除指定 OTA 分区标签的元数据条目。
    bool Remove(std::string_view label);

    // ---- 已安装应用列表 ----

    /// 枚举同时拥有 NVS 元数据和有效 OTA 分区的应用。
    std::vector<AppInfo> ListInstalledApps() const;

    /// 按标签查找单个应用；未找到返回 nullopt。
    std::optional<AppInfo> FindByLabel(std::string_view label) const;

    /// 从文件路径派生显示名称（如 "/sdcard/app.bin" → "app"）。
    static std::string NameFromFilePath(std::string_view path);

    // ---- 启动控制 ----

    /// 将指定分区设为下次 OTA 启动目标。
    bool SetBootApp(std::string_view label);

    /// 返回当前配置的启动应用标签（可能为空）。
    std::string GetBootAppLabel() const;

    /// 重启进入指定标签的应用（调用 SetBootApp + esp_restart）。
    bool BootApp(std::string_view label);

    /// 重启进入当前运行的分区。
    bool BootCurrentApp();

    // ---- 生命周期 ----

    /// 删除应用：移除 NVS 元数据并擦除 OTA 分区内容。
    /// 若存在关联 FAT 分区，一并移除。
    bool DeleteApp(std::string_view label);

    /// 重命名应用（仅更新 NVS 显示名称）。
    bool RenameApp(std::string_view label, std::string_view new_name);

    /// 返回标签对应的显示名称；未找到时回退为标签本身。
    std::string GetDisplayName(std::string_view label) const;

   private:
    PartitionBase& partition_;
};

}  // namespace launcher
