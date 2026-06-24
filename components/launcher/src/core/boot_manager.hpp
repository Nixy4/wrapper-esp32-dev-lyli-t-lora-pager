#pragma once

#include <string>
#include <cstring>

#include <esp_log.h>
#include <esp_ota_ops.h>
#include <esp_partition.h>
#include <esp_system.h>

#include "core/app_registry.hpp"

namespace launcher::core
{

/**
 * @brief 管理应用启动选择与引导逻辑。（CRTP 零开销模板版本）
 *
 * @tparam PartitionT  实现 PartitionBase<PartitionT> 接口约束的分区类型。
 * @tparam StorageT    实现 StorageBase<StorageT> 接口约束的存储类型。
 * @tparam TimeT       实现 TimeBase<TimeT> 接口约束的时间类型。
 *
 * 启动时，BootManager 决定是显示 Launcher 菜单还是直接
 * 引导上次使用的应用：
 *
 *   ShouldShowMenu() == false  →  调用 BootApp(last_label)
 *   ShouldShowMenu() == true   →  显示 ScreenAppList
 */
template <typename PartitionT, typename StorageT, typename TimeT>
class BootManager
{
    PartitionT& partition_;
    StorageT& storage_;
    TimeT& time_;
    AppRegistry<StorageT>& registry_;
    const char* cfg_ns_;

    static constexpr const char* kTag = "Launcher|BootMgr";

   public:
    BootManager(PartitionT& partition,
                StorageT& storage,
                TimeT& time,
                AppRegistry<StorageT>& registry,
                const char* cfg_ns = CONFIG_LAUNCHER_NVS_CFG_NS)
        : partition_(partition),
          storage_(storage),
          time_(time),
          registry_(registry),
          cfg_ns_(cfg_ns)
    {
    }

    /**
     * @brief 决定是否应显示 Launcher UI。
     *
     * 以下情况返回 true（显示菜单）：
     *   - NVS 中没有记录上次启动的应用，或
     *   - 记录的分区已不存在于分区表中。
     *
     * @param[out] last_label  返回 false 时填充上次启动的标签。
     */
    bool ShouldShowMenu(std::string& last_label)
    {
        if (!registry_.GetLastBooted(last_label) || last_label.empty())
        {
            ESP_LOGI(kTag, "No last-booted app in NVS → show menu");
            return true;
        }

        // 验证保存的分区是否仍然存在
        const esp_partition_t* p = esp_partition_find_first(
            ESP_PARTITION_TYPE_APP, ESP_PARTITION_SUBTYPE_ANY, last_label.c_str());

        if (!p)
        {
            ESP_LOGW(kTag, "Last-booted partition '%s' no longer exists → show menu",
                     last_label.c_str());
            last_label.clear();
            return true;
        }

        ESP_LOGI(kTag, "Auto-booting '%s'", last_label.c_str());
        return false;
    }

    /**
     * @brief 引导由 @p label 标识的应用。
     *
     * 设置 OTA 引导分区，将标签写入 NVS，然后调用 esp_restart()。
     * 成功时不返回。
     */
    void BootApp(const std::string& label)
    {
        ESP_LOGI(kTag, "Booting app: '%s'", label.c_str());

        if (!partition_.SetBootByLabel(label.c_str()))
        {
            ESP_LOGE(kTag, "Failed to set boot partition to '%s'", label.c_str());
            return;
        }

        registry_.SetLastBooted(label);

        ESP_LOGI(kTag, "Restarting…");
        time_.DelayMs(100);
        esp_restart();
        // Not reached
    }

    /**
     * @brief 获取当前运行分区的标签。
     */
    bool GetCurrentLabel(std::string& label)
    {
        char buf[17] = {};
        if (!partition_.GetBootLabel(buf, sizeof(buf)))
            return false;
        label = buf;
        return true;
    }
};

}  // namespace launcher::core
