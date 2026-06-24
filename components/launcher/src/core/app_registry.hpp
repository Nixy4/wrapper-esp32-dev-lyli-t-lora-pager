#pragma once

#include <string>
#include <vector>
#include <esp_log.h>

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
 * @brief NVS 支持的已安装应用注册表。（CRTP 零开销模板版本）
 *
 * @tparam StorageT  实现 StorageBase<StorageT> 接口约束的存储类型。
 *
 * NVS 模式（命名空间 CONFIG_LAUNCHER_NVS_APP_NS，默认 "l_apps"）：
 *   key = 分区标签（如 "ota_0"）
 *   value = 显示名称（如 "My App"）
 *
 * 设置存储在单独的命名空间（CONFIG_LAUNCHER_NVS_CFG_NS）中。
 */
template <typename StorageT>
class AppRegistry
{
    StorageT& storage_;
    const char* app_ns_;
    const char* cfg_ns_;

    static constexpr const char* kTag = "Launcher|AppRegistry";

   public:
    using storage_type = StorageT;

    AppRegistry(StorageT& storage,
                const char* app_ns = CONFIG_LAUNCHER_NVS_APP_NS,
                const char* cfg_ns = CONFIG_LAUNCHER_NVS_CFG_NS)
        : storage_(storage), app_ns_(app_ns), cfg_ns_(cfg_ns)
    {
    }

    /// 返回底层存储引用（供需要直接访问存储的 UI 层使用）。
    StorageT& GetStorage() { return storage_; }

    /// 从 NVS 加载所有已注册的应用到 @p apps。
    bool Load(std::vector<AppInfo>& apps)
    {
        apps.clear();

        std::vector<std::string> keys;
        if (!storage_.NvsIterateKeys(app_ns_, keys))
        {
            ESP_LOGW(kTag, "未找到应用命名空间（首次运行？）");
            return true;  // 空列表是合法的
        }

        for (const auto& key : keys)
        {
            std::string name;
            if (storage_.NvsGet(app_ns_, key.c_str(), name))
            {
                apps.push_back({key, name});
                ESP_LOGD(kTag, "Loaded: '%s' → '%s'", key.c_str(), name.c_str());
            }
        }

        ESP_LOGI(kTag, "Loaded %zu installed app(s)", apps.size());
        return true;
    }

    /// 持久化一个应用条目（插入或更新）。
    bool Save(const AppInfo& app)
    {
        if (app.label.empty() || app.name.empty())
        {
            ESP_LOGE(kTag, "save: label and name must not be empty");
            return false;
        }

        bool ok = storage_.NvsSet(app_ns_, app.label.c_str(), app.name);
        if (ok)
            ESP_LOGI(kTag, "Saved: '%s' → '%s'", app.label.c_str(), app.name.c_str());
        else
            ESP_LOGE(kTag, "Failed to save app '%s'", app.label.c_str());
        return ok;
    }

    /// 按分区标签删除一个应用条目。
    bool Remove(const std::string& label)
    {
        bool ok = storage_.NvsDel(app_ns_, label.c_str());
        if (ok)
            ESP_LOGI(kTag, "Removed: '%s'", label.c_str());
        else
            ESP_LOGE(kTag, "Failed to remove '%s'", label.c_str());
        return ok;
    }

    /// 记录最近一次引导的分区标签。
    bool SetLastBooted(const std::string& label)
    {
        return storage_.NvsSet(cfg_ns_, "last_boot", label);
    }

    /// 获取最近一次引导的分区标签。
    /// 条目不存在时返回 false，@p label 不变。
    bool GetLastBooted(std::string& label) { return storage_.NvsGet(cfg_ns_, "last_boot", label); }
};

}  // namespace launcher::core
