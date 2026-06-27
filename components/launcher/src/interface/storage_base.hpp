#pragma once

#include <string>
#include <string_view>
#include <vector>

namespace launcher
{
class StorageBase
{
   private:
    std::string mount_point_;

   public:
    StorageBase() = default;
    virtual ~StorageBase() = default;

    // 所有底层硬件应该由底层封装全部初始化并自检, APP层只验证所需要功能是否可用
    virtual bool IsHardwareInitialized() = 0;
    virtual bool IsFileSystemInitialized() = 0;
    virtual bool IsMounted() = 0;

    virtual void SetMountPoint(std::string_view path) { mount_point_ = std::string(path); }
    virtual std::string GetMountPoint() { return mount_point_; }

    virtual std::vector<std::string> GetFileList(std::string_view path) = 0;
};
}  // namespace launcher