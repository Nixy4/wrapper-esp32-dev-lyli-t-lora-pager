#include "boot_manager.hpp"

#include <cstring>
#include <esp_log.h>
#include <esp_ota_ops.h>
#include <esp_system.h>

static const char* TAG = "Launcher|BootMgr";

namespace launcher::core
{

BootManager::BootManager(hal::IPartition& partition,
                         hal::IStorage& storage,
                         osal::ITime& time,
                         AppRegistry& registry,
                         const char* cfg_ns)
    : partition_(partition),
      storage_(storage),
      time_(time),
      registry_(registry),
      cfg_ns_(cfg_ns)
{
}

bool BootManager::ShouldShowMenu(std::string& last_label)
{
    if (!registry_.GetLastBooted(last_label) || last_label.empty())
    {
        ESP_LOGI(TAG, "No last-booted app in NVS → show menu");
        return true;
    }

    // 验证保存的分区是否仍然存在
    const esp_partition_t* p = esp_partition_find_first(
        ESP_PARTITION_TYPE_APP, ESP_PARTITION_SUBTYPE_ANY, last_label.c_str());

    if (!p)
    {
        ESP_LOGW(TAG, "Last-booted partition '%s' no longer exists → show menu",
                 last_label.c_str());
        last_label.clear();
        return true;
    }

    ESP_LOGI(TAG, "Auto-booting '%s'", last_label.c_str());
    return false;
}

void BootManager::BootApp(const std::string& label)
{
    ESP_LOGI(TAG, "Booting app: '%s'", label.c_str());

    if (!partition_.SetBootByLabel(label.c_str()))
    {
        ESP_LOGE(TAG, "Failed to set boot partition to '%s'", label.c_str());
        return;
    }

    registry_.SetLastBooted(label);

    ESP_LOGI(TAG, "Restarting…");
    // 小延迟以将 UART 输出刷新
    time_.DelayMs(100);
    esp_restart();
    // Not reached
}

bool BootManager::GetCurrentLabel(std::string& label)
{
    char buf[17] = {};
    if (!partition_.GetBootLabel(buf, sizeof(buf)))
        return false;
    label = buf;
    return true;
}

}  // namespace launcher::core
