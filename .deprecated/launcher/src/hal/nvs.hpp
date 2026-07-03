#pragma once
#include <cstdint>
#include <string>
namespace launcher::hal
{
enum class NvsMode
{
    ReadOnly = 0,
    ReadWrite = 1,
};

class NvsBase
{
   private:
    bool availability_;

   public:
    bool IsAvailable() { return availability_; }

    NvsBase() = default;
    virtual ~NvsBase() = default;

    virtual bool CheckAvailability() = 0;

    virtual bool Erase() = 0;
    virtual bool Commit() = 0;

    virtual bool OpenNamespace(std::string_view namespace_name, NvsMode open_mode) = 0;
    virtual bool EraseKey(std::string_view key) = 0;

    virtual bool SetValue8(std::string_view key, uint8_t value) = 0;
    virtual bool GetValue8(std::string_view key, uint8_t& out_value) = 0;
    virtual bool SetValue16(std::string_view key, uint16_t value) = 0;
    virtual bool GetValue16(std::string_view key, uint16_t& out_value) = 0;
    virtual bool SetValue32(std::string_view key, uint32_t value) = 0;
    virtual bool GetValue32(std::string_view key, uint32_t& out_value) = 0;
    virtual bool SetValue64(std::string_view key, uint64_t value) = 0;
    virtual bool GetValue64(std::string_view key, uint64_t& out_value) = 0;
    virtual bool SetString(std::string_view key, std::string value) = 0;
    virtual bool GetString(std::string_view key, std::string& out_value) = 0;
};
}  // namespace launcher::hal