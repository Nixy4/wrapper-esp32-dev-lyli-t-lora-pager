#pragma once

#include "hal/storage_base.hpp"
#include "wrapper/logger.hpp"
#include "wrapper/spi.hpp"
#include "wrapper/sd_spi.hpp"

#include "driver/gpio.h"

namespace launcher::hal
{

/**
 * @brief ESP32 的 IStorage 实现。
 *
 * NVS：直接使用 nvs_flash ESP-IDF API。
 * SD：使用 wrapper::SdSpi（通过共享 SPI 总线的 SPI 模式 SD 卡）。
 *
 * T-LoRa Pager SD 卡：CS = GPIO_NUM_21，共享 SPI 总线（SPI2_HOST）。
 */
class StorageEsp32 : public StorageBase<StorageEsp32>
{
    wrapper::Logger& logger_;
    wrapper::SpiBus& spi_bus_;
    gpio_num_t sd_cs_pin_;
    const char* sd_mount_point_;
    wrapper::SdSpi sd_;
    bool sd_mounted_ = false;

   public:
    /**
     * @param logger          日志引用。
     * @param spi_bus         已初始化的 SPI 总线引用。
     * @param sd_cs_pin       SD 卡片选 GPIO 引脚。
     * @param sd_mount_point  VFS 挂载路径（默认：CONFIG_LAUNCHER_SD_MOUNT_POINT）。
     */
    StorageEsp32(wrapper::Logger& logger,
                 wrapper::SpiBus& spi_bus,
                 gpio_num_t sd_cs_pin,
                 const char* sd_mount_point = CONFIG_LAUNCHER_SD_MOUNT_POINT);

    ~StorageEsp32();

    // ── NVS ──────────────────────────────────────────────────────────────────

    bool NvsGet(const char* ns, const char* key, std::string& out);
    bool NvsSet(const char* ns, const char* key, const std::string& val);
    bool NvsDel(const char* ns, const char* key);
    bool NvsIterateKeys(const char* ns, std::vector<std::string>& keys);

    // ── SD 卡 ────────────────────────────────────────────────────────────────
    bool SdMount();
    bool SdUnmount();
    bool SdAvailable() { return sd_mounted_; }
    std::vector<std::string> SdListFiles(const char* dir, const char* ext);
    bool SdFileSize(const char* path, size_t& out_size);
    FILE* SdOpenRead(const char* path);
    void SdClose(FILE* f);
};

}  // namespace launcher::hal
