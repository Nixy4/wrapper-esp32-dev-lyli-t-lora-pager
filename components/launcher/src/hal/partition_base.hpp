#pragma once

#include <cstdint>
#include <cstring>
#include <functional>

namespace launcher::hal
{

/**
 * @brief 描述为固件安装选定的 Flash 分区槽位。
 */
struct InstallSlot
{
    char label[17] = {};    ///< 物理分区标签（如 "ota_0"）
    uint8_t subtype = 0;    ///< ESP 分区子类型（ESP_PARTITION_SUBTYPE_APP_OTA_x）
    uint32_t offset = 0;    ///< 分区的 Flash 字节地址
    uint32_t capacity = 0;  ///< 分区大小（字节）
    bool is_new = true;     ///< true 表示新安装，false 表示重新刷入已有槽位
};

/**
 * @brief 进度回调：(bytes_written, total_bytes)。
 */
using FlashProgressCb = std::function<void(size_t written, size_t total)>;

/**
 * @brief CRTP 静态基类：平台无关的分区 / OTA 刷写接口约束。
 *
 * Derived 须提供以下方法（无 virtual）：
 *   PlanInstall / FlashBegin / FlashWrite / FlashEnd / FlashAbort
 *   SetBootByLabel / GetBootLabel
 *
 * ESP32 实现： esp32/partition_esp32.hpp
 */
template <typename Derived>
class PartitionBase
{
   protected:
    ~PartitionBase() = default;
};

}  // namespace launcher::hal
