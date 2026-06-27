#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace launcher
{

/// 单个已安装应用的持久化元数据。
/// 存储于 NVS，镜像 Reference 中的 LauncherAppMetadata + esp_app_desc_t 字段。
struct AppInfo
{
    std::string name;                     ///< 用户可见的显示名称
    std::string label;                    ///< OTA 分区标签（如 "ota_0"）
    std::vector<std::string> fat_labels;  ///< 关联的 FAT 数据分区标签列表
    std::string version;                  ///< 来自 esp_app_desc_t 的语义化版本字符串
    uint32_t size = 0;                    ///< 二进制大小（字节）；0 = 未知
    bool is_current = false;              ///< 当前运行分区
    bool is_boot = false;                 ///< 已选为下次启动目标
};

}  // namespace launcher
