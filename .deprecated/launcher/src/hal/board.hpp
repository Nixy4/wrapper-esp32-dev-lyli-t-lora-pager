#pragma once
#include <string>
namespace launcher::hal
{
class Board
{
   public:
    std::string name_;
    std::string label_;
    struct
    {
        uint8_t major;
        uint8_t minor;
        uint8_t patch;
    } hw_version_;
};
};  // namespace launcher::hal