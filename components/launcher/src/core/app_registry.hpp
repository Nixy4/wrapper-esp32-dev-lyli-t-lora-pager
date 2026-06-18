#pragma once

#include <string>
#include <vector>

#include "hal/i_storage.hpp"

namespace launcher::core
{

/**
 * @brief 已安装应用程序的元数据。
 */
struct AppInfo
{
    std::string label;  ///< 物理 OTA 分区标签（如 "ota_0"）
    std::string name;   ///< 用户可见的显示名称
};

/**
 * @brief NVS 支持的已安装应用注册表。
 *
 * NVS 模式（命名空间 CONFIG_LAUNCHER_NVS_APP_NS，默认 "l_apps"）：
 *   key = 分区标签（如 "ota_0"）
 *   value = 显示名称（如 "My App"）
 *
 * 设置存储在单独的命名空间（CONFIG_LAUNCHER_NVS_CFG_NS）中。
 */
class AppRegistry
{
    hal::IStorage& storage_;
    const char* app_ns_;
    const char* cfg_ns_;

   public:
    AppRegistry(hal::IStorage& storage,
                const char* app_ns = CONFIG_LAUNCHER_NVS_APP_NS,
                const char* cfg_ns = CONFIG_LAUNCHER_NVS_CFG_NS);

    /// 从 NVS 加载所有已注册的应用到 @p apps。
    bool load(std::vector<AppInfo>& apps);

    /// 持久化一个应用条目（插入或更新）。
    bool save(const AppInfo& app);

    /// 按分区标签删除一个应用条目。
    bool remove(const std::string& label);

    /// 记录最近一次引导的分区标签。
    bool setLastBooted(const std::string& label);

    /// 获取最近一次引导的分区标签。
    /// 类目不存在时返回 false，@p label 不变。
    bool getLastBooted(std::string& label);
};

}  // namespace launcher::core
