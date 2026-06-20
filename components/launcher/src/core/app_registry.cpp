#include "app_registry.hpp"

#include <esp_log.h>

static const char* TAG = "Launcher|AppRegistry";

namespace launcher::core
{

AppRegistry::AppRegistry(hal::IStorage& storage, const char* app_ns, const char* cfg_ns)
    : storage_(storage),
      app_ns_(app_ns),
      cfg_ns_(cfg_ns)
{
}

bool AppRegistry::Load(std::vector<AppInfo>& apps)
{
    apps.clear();

    std::vector<std::string> keys;
    if (!storage_.NvsIterateKeys(app_ns_, keys))
    {
        ESP_LOGW(TAG, "未找到应用命名空间（首次运行？）");
        return true;  // 空列表是合法的
    }

    for (const auto& key : keys)
    {
        std::string name;
        if (storage_.NvsGet(app_ns_, key.c_str(), name))
        {
            apps.push_back({key, name});
            ESP_LOGD(TAG, "Loaded: '%s' → '%s'", key.c_str(), name.c_str());
        }
    }

    ESP_LOGI(TAG, "Loaded %zu installed app(s)", apps.size());
    return true;
}

bool AppRegistry::Save(const AppInfo& app)
{
    if (app.label.empty() || app.name.empty())
    {
        ESP_LOGE(TAG, "save: label and name must not be empty");
        return false;
    }

    bool ok = storage_.NvsSet(app_ns_, app.label.c_str(), app.name);
    if (ok)
        ESP_LOGI(TAG, "Saved: '%s' → '%s'", app.label.c_str(), app.name.c_str());
    else
        ESP_LOGE(TAG, "Failed to save app '%s'", app.label.c_str());
    return ok;
}

bool AppRegistry::Remove(const std::string& label)
{
    bool ok = storage_.NvsDel(app_ns_, label.c_str());
    if (ok)
        ESP_LOGI(TAG, "Removed: '%s'", label.c_str());
    else
        ESP_LOGE(TAG, "Failed to remove '%s'", label.c_str());
    return ok;
}

bool AppRegistry::SetLastBooted(const std::string& label)
{
    return storage_.NvsSet(cfg_ns_, "last_boot", label);
}

bool AppRegistry::GetLastBooted(std::string& label)
{
    return storage_.NvsGet(cfg_ns_, "last_boot", label);
}

}  // namespace launcher::core
