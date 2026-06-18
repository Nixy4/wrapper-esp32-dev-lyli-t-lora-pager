#pragma once

#include <functional>
#include <string>

#include "hal/i_partition.hpp"
#include "hal/i_storage.hpp"
#include "core/app_registry.hpp"

namespace launcher::core
{

/**
 * @brief 进度通知回调：(阶段描述, 已写入字节数, 总字节数)。
 *        total_bytes == 0 表示总量尚未知。
 */
using InstallProgressCb = std::function<void(const char* stage, size_t written, size_t total)>;

/**
 * @brief 将 SD 卡上的固件二进制安装到 OTA 槽位。
 *
 * 典型调用时序：
 *   1. sdInstaller.install(sd_path, display_name, progress_cb)
 *      → 内部调用 IPartition::planInstall / flashBegin / flashWrite / flashEnd
 *      → 然后将（分区标签 → display_name）保存到 AppRegistry
 *
 * 安装器不重启设备，调用方自行决定引导时机。
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
     * @brief 从 SD 卡安装 .bin 固件。
     *
     * @param sd_path      VFS 完整路径（如 "/sdcard/myapp.bin"）。
     * @param display_name 用户可见的应用名称（存入 NVS）。
     * @param progress     可选的进度回调。
     * @return 成功返回 true；任何错误自动中止 OTA 并返回 false。
     */
    bool install(const char* sd_path,
                 const char* display_name,
                 InstallProgressCb progress = nullptr);
};

}  // namespace launcher::core
