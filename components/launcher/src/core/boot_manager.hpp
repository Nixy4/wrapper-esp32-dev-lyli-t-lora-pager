#pragma once

#include <string>

#include "hal/i_partition.hpp"
#include "hal/i_storage.hpp"
#include "osal/i_time.hpp"
#include "core/app_registry.hpp"

namespace launcher::core
{

/**
 * @brief Manages application boot selection and launch.
 *
 * On startup, BootManager decides whether to show the Launcher menu or
 * to boot the last-used application immediately:
 *
 *   shouldShowMenu() == false  →  call bootApp(last_label)
 *   shouldShowMenu() == true   →  show ScreenAppList
 *
 * bootApp() updates the OTA boot partition via IPartition::setBootByLabel(),
 * records the choice in NVS, and calls esp_restart().
 */
class BootManager
{
    hal::IPartition& partition_;
    hal::IStorage& storage_;
    osal::ITime& time_;
    AppRegistry& registry_;
    const char* cfg_ns_;

   public:
    BootManager(hal::IPartition& partition,
                hal::IStorage& storage,
                osal::ITime& time,
                AppRegistry& registry,
                const char* cfg_ns = CONFIG_LAUNCHER_NVS_CFG_NS);

    /**
     * @brief Determine whether the Launcher UI should be shown.
     *
     * Returns true (show menu) when:
     *   - No last-booted app is recorded in NVS, OR
     *   - The recorded partition no longer exists in the partition table.
     *
     * Returns false (auto-boot) when a valid last-booted app is found.
     *
     * @param[out] last_label  Populated with the last-booted label when
     *                         returning false (ready to pass to bootApp()).
     */
    bool shouldShowMenu(std::string& last_label);

    /**
     * @brief Boot the app identified by @p label.
     *
     * Sets the OTA boot partition, saves the label to NVS, then calls
     * esp_restart().  Does NOT return on success.
     */
    void bootApp(const std::string& label);

    /**
     * @brief Get the label of the partition that is currently running.
     */
    bool getCurrentLabel(std::string& label);
};

}  // namespace launcher::core
