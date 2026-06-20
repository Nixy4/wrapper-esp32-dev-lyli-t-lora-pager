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
 * @brief 平台无关的分区 / OTA 刷写抽象接口。
 *
 * 典型安装流程：
 *   1. planInstall(nullptr, image_size, slot)   — 选择目标槽位
 *   2. flashBegin(slot, image_size)             — 擦除并初始化 OTA 会话
 *   3. flashWrite(data, len)  × N              — 流式传输固件块
 *   4. flashEnd()                               — 校验并终结
 *   5. setBootByLabel(slot.label)               — 将 otadata 指向新应用
 *
 * 如需中止进行中的写入，调用 flashAbort()。
 *
 * ESP32 实现： esp32/partition_esp32.hpp
 */
class IPartition
{
   public:
    virtual ~IPartition() = default;

    /**
     * @brief 选择安装用的分区槽位。
     *
     * @param label       要（重）安装的分区标签，传 nullptr 则自动
     *                    选择第一个合适的 OTA 槽位。
     * @param image_size  固件二进制大小（字节）。
     * @param slot        输出：所选槽位详情。
     * @return 无合适槽位时（如固件过大）返回 false。
     */
    virtual bool PlanInstall(const char* label, size_t image_size, InstallSlot& slot) = 0;

    /**
     * @brief 开始向 @p slot 写入 OTA 会话。
     *
     * @param slot        由 planInstall() 返回的槽位。
     * @param image_size  固件镜像总大小（0 表示未知）。
     */
    virtual bool FlashBegin(const InstallSlot& slot, size_t image_size) = 0;

    /// 写入 @p len 字节的固件数据。
    virtual bool FlashWrite(const uint8_t* data, size_t len) = 0;

    /// 校验并终结 OTA 会话。
    virtual bool FlashEnd() = 0;

    /// 中止当前活跃的 OTA 会话（无活跃会话时为空操作）。
    virtual bool FlashAbort() = 0;

    /**
     * @brief 按标签设置引导分区。
     *
     * 将 OTA 选择写入 otadata 分区，调用方负责执行
     * esp_restart()（本函数不调用重启）。
     *
     * @return 分区未找到或 otadata 缺失时返回 false。
     */
    virtual bool SetBootByLabel(const char* label) = 0;

    /**
     * @brief 获取当前已选定的引导分区标签。
     *
     * @param label_out  输出缓冲区（至少 17 字节）。
     * @param max_len    @p label_out 的大小。
     */
    virtual bool GetBootLabel(char* label_out, size_t max_len) = 0;
};

}  // namespace launcher::hal
