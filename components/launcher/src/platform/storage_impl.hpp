#pragma once
#include "interface/storage_base.hpp"

namespace launcher
{
class StorageImpl : public StorageBase
{
   public:
    StorageImpl() = default;
    virtual ~StorageImpl() = default;

    virtual bool IsHardwareInitialized() override;
    virtual bool IsFileSystemInitialized() override;
    virtual bool IsMounted() override;

    virtual std::vector<std::string> GetFileList(std::string_view path) override;

    virtual bool Mount() override;
    virtual bool Unmount() override;
};
}  // namespace launcher