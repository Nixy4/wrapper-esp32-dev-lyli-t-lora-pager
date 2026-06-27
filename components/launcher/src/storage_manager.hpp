#pragma once

#include "interface/storage_base.hpp"

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace launcher
{

/// 文件/目录信息描述符。
struct FileInfo
{
    std::string name;  ///< 文件或目录名（非完整路径）
    std::string path;  ///< 完整绝对路径
    bool is_dir = false;
    uint32_t size = 0;  ///< 文件大小（字节）；目录为 0
};

/// Launcher 层的高层存储管理器。
/// 封装一个 StorageBase 设备，提供丰富的文件操作。
/// 对应 Reference 中 sd_functions.cpp 的职责。
class StorageManager
{
   public:
    /// @param storage  主存储设备（如通过 StorageImpl 封装的 SD 卡）。
    explicit StorageManager(StorageBase& storage);
    ~StorageManager();

    // ---- 挂载 / 状态 ----

    bool IsMounted() const;
    bool IsHardwareReady() const;

    // ---- 目录列表 ----

    /// 列出 path 下的文件和目录，排序（目录优先，再按名称）。
    std::vector<FileInfo> ListDirectory(std::string_view path) const;

    /// 仅返回 path 下的 .bin 固件文件（供安装选择器使用）。
    std::vector<FileInfo> ListBinFiles(std::string_view path) const;

    // ---- 文件操作 ----

    bool DeleteFile(std::string_view path);
    bool RenameFile(std::string_view path, std::string_view new_name);
    bool CreateDirectory(std::string_view path);

    /// 将 src_path 处的文件复制到 dst_path。
    bool CopyFile(std::string_view src_path, std::string_view dst_path);

    // ---- 读 / 写 ----

    /// 将文件全部内容读入缓冲区；出错返回 false。
    bool ReadFile(std::string_view path, std::vector<uint8_t>& out) const;

    /// 将 data 写入 path，覆盖已有内容。
    bool WriteFile(std::string_view path, const uint8_t* data, size_t size);

    // ---- 查询 ----

    bool FileExists(std::string_view path) const;
    uint32_t FileSize(std::string_view path) const;  ///< 文件不存在时返回 0
    std::string MountPoint() const;

   private:
    StorageBase& storage_;
};

}  // namespace launcher
