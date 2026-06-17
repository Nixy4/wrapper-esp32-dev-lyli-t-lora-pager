#pragma once

#include <string>
#include <vector>

#include "hal/i_storage.hpp"

namespace launcher::core
{

/**
 * @brief Metadata for an installed application.
 */
struct AppInfo
{
    std::string label;  ///< Physical OTA partition label (e.g. "ota_0")
    std::string name;   ///< User-visible display name
};

/**
 * @brief NVS-backed registry of installed applications.
 *
 * NVS schema (namespace CONFIG_LAUNCHER_NVS_APP_NS, default "l_apps"):
 *   key = partition label (e.g. "ota_0")
 *   value = display name (e.g. "My App")
 *
 * Settings are stored in a separate namespace (CONFIG_LAUNCHER_NVS_CFG_NS).
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

    /// Load all registered apps from NVS into @p apps.
    bool load(std::vector<AppInfo>& apps);

    /// Persist an app entry (insert or update).
    bool save(const AppInfo& app);

    /// Remove an app entry by partition label.
    bool remove(const std::string& label);

    /// Record the most-recently-booted partition label.
    bool setLastBooted(const std::string& label);

    /// Retrieve the most-recently-booted partition label.
    /// Returns false (and leaves @p label unchanged) if no entry exists.
    bool getLastBooted(std::string& label);
};

}  // namespace launcher::core
