#pragma once

#include "hal/i_storage.hpp"
#include "wrapper/logger.hpp"
#include "wrapper/spi.hpp"
#include "wrapper/sd_spi.hpp"

#include "driver/gpio.h"

namespace launcher::hal
{

/**
 * @brief IStorage implementation for ESP32.
 *
 * NVS: uses nvs_flash ESP-IDF API directly.
 * SD:  uses wrapper::SdSpi (SPI-mode SD card via the shared SPI bus).
 *
 * T-LoRa Pager SD card: CS = GPIO_NUM_21, shared SPI bus (SPI2_HOST).
 */
class StorageEsp32 : public IStorage
{
    wrapper::Logger& logger_;
    wrapper::SpiBus& spi_bus_;
    gpio_num_t sd_cs_pin_;
    const char* sd_mount_point_;
    wrapper::SdSpi sd_;
    bool sd_mounted_ = false;

   public:
    /**
     * @param logger          Logger reference.
     * @param spi_bus         Already-initialised SPI bus reference.
     * @param sd_cs_pin       GPIO pin for SD card chip-select.
     * @param sd_mount_point  VFS mount path (default: CONFIG_LAUNCHER_SD_MOUNT_POINT).
     */
    StorageEsp32(wrapper::Logger& logger,
                 wrapper::SpiBus& spi_bus,
                 gpio_num_t sd_cs_pin,
                 const char* sd_mount_point = CONFIG_LAUNCHER_SD_MOUNT_POINT);

    ~StorageEsp32() override;

    // ── NVS ──────────────────────────────────────────────────────────────────
    bool nvsGet(const char* ns, const char* key, std::string& out) override;
    bool nvsSet(const char* ns, const char* key, const std::string& val) override;
    bool nvsDel(const char* ns, const char* key) override;
    bool nvsIterateKeys(const char* ns, std::vector<std::string>& keys) override;

    // ── SD card ──────────────────────────────────────────────────────────────
    bool sdMount() override;
    bool sdUnmount() override;
    bool sdAvailable() override { return sd_mounted_; }
    std::vector<std::string> sdListFiles(const char* dir, const char* ext) override;
    bool sdFileSize(const char* path, size_t& out_size) override;
    FILE* sdOpenRead(const char* path) override;
    void sdClose(FILE* f) override;
};

}  // namespace launcher::hal
