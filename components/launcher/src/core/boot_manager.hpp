#pragma once

#include <string>

#include "hal/i_partition.hpp"
#include "hal/i_storage.hpp"
#include "osal/i_time.hpp"
#include "core/app_registry.hpp"

namespace launcher::core
{

/**
 * @brief 管理应用启动选择与引导逻辑。
 *
 * 启动时，BootManager 决定是显示 Launcher 菜单还是直接
 * 引导上次使用的应用：
 *
 *   shouldShowMenu() == false  →  调用 bootApp(last_label)
 *   shouldShowMenu() == true   →  显示 ScreenAppList
 *
 * bootApp() 通过 IPartition::setBootByLabel() 更新 OTA 引导分区，
 * 将选择记录到 NVS，然后调用 esp_restart()。
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
     * @brief 决定是否应显示 Launcher UI。
     *
     * 以下情况返回 true（显示菜单）：
     *   - NVS 中没有记录上次启动的应用，或
     *   - 记录的分区已不存在于分区表中。
     *
     * 找到有效的上次启动应用时返回 false（自动引导）。
     *
     * @param[out] last_label  返回 false 时填充上次启动的标签
     *                         （可直接传入 bootApp()）。
     */
    bool shouldShowMenu(std::string& last_label);

    /**
     * @brief 引导由 @p label 标识的应用。
     *
     * 设置 OTA 引导分区，将标签写入 NVS，然后
     * 调用 esp_restart()。成功时不返回。
     */
    void bootApp(const std::string& label);

    /**
     * @brief 获取当前运行分区的标签。
     */
    bool getCurrentLabel(std::string& label);
};

}  // namespace launcher::core
