#pragma once
#include <cstdint>
#include <string>
#include <vector>
namespace launcher::hal
{

struct PartitionEntry
{
    uint8_t type = 0xFF;
    uint8_t subtype = 0xFF;
    uint32_t offset = 0;
    uint32_t size = 0;
    uint32_t flags = 0;
    char label[17] = {0};

    bool IsApp() const;
    bool IsData() const;
    bool IsOtaApp() const;
    bool IsFactoryApp() const;
};

struct PartitionTable
{
    std::vector<PartitionEntry> entries;
    uint32_t flash_size = 0;
    bool has_md5 = false;
};

struct PartitionRange
{
    uint32_t offset = 0;
    uint32_t size = 0;
};

class PartitionBase
{
   private:
    bool availability_;

   public:
    bool IsAvailable() { return availability_; }

    PartitionBase() = default;
    virtual ~PartitionBase() = default;

    virtual bool CheckAvailability() = 0;

    // Read / Parse / Build / Write
    virtual bool ReadCurrent(PartitionTable& table) = 0;
    virtual bool Parse(const uint8_t* data, size_t size, PartitionTable& table) = 0;
    virtual bool Build(const PartitionTable& table, uint8_t* out, size_t out_size) = 0;
    virtual bool Validate(const PartitionTable& table) = 0;
    virtual bool Write(const PartitionTable& table) = 0;

    // Query
    virtual PartitionEntry* FindByLabel(PartitionTable& table, std::string_view label) = 0;
    virtual const PartitionEntry* FindByLabel(const PartitionTable& table,
                                              std::string_view label) = 0;
    virtual PartitionEntry* FindAppBySubtype(PartitionTable& table, uint8_t subtype) = 0;
    virtual const PartitionEntry* FindOtaData(const PartitionTable& table) = 0;
    virtual uint8_t CountOtaApps(const PartitionTable& table) = 0;
    virtual std::vector<PartitionRange> GetFreeRanges(const PartitionTable& table) = 0;
    virtual bool FindFreeRange(const PartitionTable& table,
                               uint32_t required_size,
                               uint32_t alignment,
                               PartitionRange& range) = 0;

    // Modify
    virtual bool AddEntry(PartitionTable& table, const PartitionEntry& entry) = 0;
    virtual bool CreateOtaApp(PartitionTable& table,
                              uint32_t image_size,
                              std::string_view label,
                              PartitionEntry* created = nullptr) = 0;
    virtual bool CreateDataPartition(PartitionTable& table,
                                     uint8_t subtype,
                                     std::string_view label,
                                     uint32_t size,
                                     PartitionEntry* created = nullptr) = 0;
    virtual bool Compact(PartitionTable& table) = 0;

    // OTA boot control
    virtual bool SetOtaBoot(const PartitionTable& table, uint8_t app_subtype) = 0;
    virtual bool ClearOtaBoot(const PartitionTable& table) = 0;
};
}  // namespace launcher::hal