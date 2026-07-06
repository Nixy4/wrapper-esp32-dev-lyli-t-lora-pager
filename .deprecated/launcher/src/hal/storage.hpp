#pragma once
#include <cstdint>
#include <string>
#include <vector>

namespace launcher::hal
{

struct StoregeType
{
    uint8_t file_system : 4;
    // 0b00: fs
    // 0b01: binary
    // 0b10: custom

    uint8_t medium : 4;
    // 0b00: flash
    // 0b01: sd_mmc
    // 0b10: sd_spi
};

union StorageInfo
{
    struct
    {
        StoregeType type;
        uint8_t data[];
    } raw;

    struct
    {
        StoregeType type;
        std::string name;         ///< 卡产品名称（来自 CID 寄存器，最长 7 字节）
        uint64_t capacity_bytes;  ///< 总容量（字节）
        uint32_t sector_count;    ///< 扇区总数
        uint32_t sector_size;     ///< 每扇区字节数（通常 512）
        uint32_t max_freq_khz;    ///< 卡支持的最大时钟频率（kHz）
        int real_freq_khz;        ///< 当前实际时钟频率（kHz）
        bool is_mmc;              ///< 是否为 eMMC / MMC 卡
        bool is_sdio;             ///< 是否为 SDIO 卡
    } sd_card;

    struct
    {
        StoregeType type;
        size_t total_bytes;  ///< 分区总字节数
        size_t used_bytes;   ///< 已使用字节数
    } spiffs;
};

enum class StorageEraseMode : int
{
    Erase = 0,    ///< 物理擦除（对应 SDMMC_ERASE_ARG / MMC Trim）
    Discard = 1,  ///< 逻辑丢弃（对应 SDMMC_DISCARD_ARG）
};

class StorageBase
{
   private:
    bool availability_;

   public:
    bool IsAvailable() { return availability_; }

    StorageBase() = default;
    virtual ~StorageBase() = 0;

    virtual bool CheckAvailability() = 0;

    virtual bool IsReady() = 0;
    virtual bool DeleteObj(const std::string src) = 0;
    virtual bool MoveObj(const std::string src, const std::string dst) = 0;
    virtual std::vector<std::string> GetObjList(const std::string src) = 0;
};
}  // namespace launcher::hal