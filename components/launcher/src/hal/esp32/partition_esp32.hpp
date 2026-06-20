#pragma once

#include "hal/i_partition.hpp"
#include "wrapper/logger.hpp"
#include "wrapper/partition.hpp"

#include "esp_ota_ops.h"
#include "esp_partition.h"

namespace launcher::hal
{

/**
 * @brief ESP32 的 IPartition 实现。
 *
 * 使用 wrapper::PartitionManager 进行分区表操作，
 * 使用 esp_ota_*() API 进行原始固件刷写。
 *
 * planInstall() 扫描 ESP-IDF 缓存的分区表，搜索现有
 * OTA 槽位（ota_0、ota_1 等）。目标 OTA 槽位必须已在
 * partitions.csv 中定义。动态创建槽位留待未来版本支持。
 */
class PartitionEsp32 : public IPartition
{
    wrapper::Logger& logger_;
    wrapper::PartitionManager part_mgr_;

    // Active OTA session
    esp_ota_handle_t ota_handle_ = 0;
    const esp_partition_t* ota_partition_ = nullptr;
    bool session_active_ = false;

   public:
    explicit PartitionEsp32(wrapper::Logger& logger);

    bool PlanInstall(const char* label, size_t image_size, InstallSlot& slot) override;

    bool FlashBegin(const InstallSlot& slot, size_t image_size) override;
    bool FlashWrite(const uint8_t* data, size_t len) override;
    bool FlashEnd() override;
    bool FlashAbort() override;

    bool SetBootByLabel(const char* label) override;
    bool GetBootLabel(char* label_out, size_t max_len) override;
};

}  // namespace launcher::hal
