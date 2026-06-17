#pragma once

#include <cstdint>
#include <cstring>
#include <functional>

namespace launcher::hal
{

/**
 * @brief Describes a flash partition slot selected for firmware installation.
 */
struct InstallSlot
{
    char label[17] = {};    ///< Physical partition label (e.g. "ota_0")
    uint8_t subtype = 0;    ///< ESP partition subtype (ESP_PARTITION_SUBTYPE_APP_OTA_x)
    uint32_t offset = 0;    ///< Flash byte address of the partition
    uint32_t capacity = 0;  ///< Partition size in bytes
    bool is_new = true;     ///< true → new install, false → re-flash existing slot
};

/**
 * @brief Progress callback: (bytes_written, total_bytes).
 */
using FlashProgressCb = std::function<void(size_t written, size_t total)>;

/**
 * @brief Platform-agnostic partition / OTA flash abstraction.
 *
 * The typical install flow is:
 *   1. planInstall(nullptr, image_size, slot)   — select target slot
 *   2. flashBegin(slot, image_size)             — erase & init OTA session
 *   3. flashWrite(data, len)  × N              — stream firmware chunks
 *   4. flashEnd()                               — validate & finalise
 *   5. setBootByLabel(slot.label)               — point otadata at new app
 *
 * To abort an in-progress write call flashAbort().
 *
 * ESP32 implementation: esp32/partition_esp32.hpp
 */
class IPartition
{
   public:
    virtual ~IPartition() = default;

    /**
     * @brief Select a partition slot for installation.
     *
     * @param label       Partition label to (re-)install to, or nullptr to
     *                    auto-select the first suitable OTA slot.
     * @param image_size  Firmware binary size in bytes.
     * @param slot        Output: selected slot details.
     * @return false if no suitable slot exists (e.g. image too large).
     */
    virtual bool planInstall(const char* label, size_t image_size, InstallSlot& slot) = 0;

    /**
     * @brief Begin an OTA write session to @p slot.
     *
     * @param slot        Slot returned by planInstall().
     * @param image_size  Total firmware image size (0 = unknown).
     */
    virtual bool flashBegin(const InstallSlot& slot, size_t image_size) = 0;

    /// Write @p len bytes of firmware data.
    virtual bool flashWrite(const uint8_t* data, size_t len) = 0;

    /// Validate and finalise the OTA session.
    virtual bool flashEnd() = 0;

    /// Abort an active OTA session (no-op if none is active).
    virtual bool flashAbort() = 0;

    /**
     * @brief Set the boot partition by label.
     *
     * Writes the OTA selection to the otadata partition and calls
     * esp_restart() in the caller (not here — that is the caller's job).
     *
     * @return false if the partition was not found or otadata is absent.
     */
    virtual bool setBootByLabel(const char* label) = 0;

    /**
     * @brief Get the label of the currently-selected boot partition.
     *
     * @param label_out  Output buffer (at least 17 bytes).
     * @param max_len    Size of @p label_out.
     */
    virtual bool getBootLabel(char* label_out, size_t max_len) = 0;
};

}  // namespace launcher::hal
