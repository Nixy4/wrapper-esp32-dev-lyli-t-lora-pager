#pragma once

#include "hal/i_partition.hpp"
#include "wrapper/logger.hpp"
#include "wrapper/partition.hpp"

#include "esp_ota_ops.h"
#include "esp_partition.h"

namespace launcher::hal
{

/**
 * @brief IPartition implementation for ESP32.
 *
 * Uses wrapper::PartitionManager for partition-table operations and
 * esp_ota_*() APIs for raw firmware flashing.
 *
 * planInstall() scans the partition table cached by ESP-IDF for existing
 * OTA slots (ota_0, ota_1, …).  The partition table must already contain
 * the target OTA slots (defined in partitions.csv).  Dynamic slot creation
 * is reserved for a future version.
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

    bool planInstall(const char* label, size_t image_size, InstallSlot& slot) override;

    bool flashBegin(const InstallSlot& slot, size_t image_size) override;
    bool flashWrite(const uint8_t* data, size_t len) override;
    bool flashEnd() override;
    bool flashAbort() override;

    bool setBootByLabel(const char* label) override;
    bool getBootLabel(char* label_out, size_t max_len) override;
};

}  // namespace launcher::hal
