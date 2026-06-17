#pragma once

#include <functional>
#include <string>

#include "hal/i_partition.hpp"
#include "hal/i_storage.hpp"
#include "core/app_registry.hpp"

namespace launcher::core
{

/**
 * @brief Progress notification: (stage_description, bytes_written, total_bytes).
 *        total_bytes == 0 means the total is not known yet.
 */
using InstallProgressCb = std::function<void(const char* stage, size_t written, size_t total)>;

/**
 * @brief Installs a firmware binary from the SD card into an OTA slot.
 *
 * Typical call sequence:
 *   1. sdInstaller.install(sd_path, display_name, progress_cb)
 *      → internally calls IPartition::planInstall / flashBegin / flashWrite / flashEnd
 *      → then saves (partition_label → display_name) to AppRegistry
 *
 * The installer does NOT reboot the device; the caller decides when to boot.
 */
class SdInstaller
{
    hal::IStorage& storage_;
    hal::IPartition& partition_;
    AppRegistry& registry_;

    static constexpr size_t kChunkSize = 4096;

   public:
    SdInstaller(hal::IStorage& storage, hal::IPartition& partition, AppRegistry& registry);

    /**
     * @brief Install a .bin firmware from the SD card.
     *
     * @param sd_path      Full VFS path (e.g. "/sdcard/myapp.bin").
     * @param display_name User-visible app name (stored in NVS).
     * @param progress     Optional progress callback.
     * @return true on success; false on any error (OTA aborted automatically).
     */
    bool install(const char* sd_path,
                 const char* display_name,
                 InstallProgressCb progress = nullptr);
};

}  // namespace launcher::core
