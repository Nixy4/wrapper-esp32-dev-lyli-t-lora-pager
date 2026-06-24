#pragma once

#include <cstdio>
#include <cstdint>
#include <string>
#include <vector>

namespace launcher::hal
{

/**
 * @brief CRTP 静态基类：平台无关的存储接口约束。
 *
 * Derived 须提供以下方法（无 virtual）：
 *   NvsGet / NvsSet / NvsDel / NvsIterateKeys
 *   SdMount / SdUnmount / SdAvailable / SdListFiles
 *   SdFileSize / SdOpenRead / SdClose
 *
 * ESP32 实现： esp32/storage_esp32.hpp
 */
template <typename Derived>
class StorageBase
{
   protected:
    ~StorageBase() = default;
};

}  // namespace launcher::hal
