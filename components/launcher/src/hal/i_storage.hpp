#pragma once

#include <cstdio>
#include <cstdint>
#include <string>
#include <vector>

namespace launcher::hal
{

/**
 * @brief 平台无关的存储抽象接口，覆盖 NVS 和 SD 卡。
 *
 * NVS（非易失性存储）用于：
 *   - 应用注册表（分区标签 → 显示名称）
 *   - Launcher 设置（上次启动的应用、亮度等）
 *
 * SD 卡用于：
 *   - 浏览待安装的固件 .bin 文件
 *
 * ESP32 实现： esp32/storage_esp32.hpp
 */
class IStorage
{
   public:
    virtual ~IStorage() = default;

    // ── NVS ──────────────────────────────────────────────────────────────────

    /// Read a string value.  @return false if the key does not exist.
    virtual bool NvsGet(const char* ns, const char* key, std::string& out) = 0;

    /// Write or overwrite a string value.
    virtual bool NvsSet(const char* ns, const char* key, const std::string& val) = 0;

    /// 删除键。即使键不存在也返回 true。
    virtual bool NvsDel(const char* ns, const char* key) = 0;

    /// 列出命名空间中的所有键。
    virtual bool NvsIterateKeys(const char* ns, std::vector<std::string>& keys) = 0;

    // ── SD 卡 ────────────────────────────────────────────────────────────────

    /// 挂载 SD 卡。可重复调用。
    virtual bool SdMount() = 0;

    /// 卸载 SD 卡。
    virtual bool SdUnmount() = 0;

    /// @return SD 卡当前是否已挂载。
    virtual bool SdAvailable() = 0;

    /**
     * @brief 列出 @p dir 目录中所有符合 @p ext 过滤器的文件。
     *
     * @param dir  SD 卡上的目录路径（如 "/sdcard"）。
     * @param ext  包含点的文件扩展名过滤器（如 ".bin"）。
     *             传入 nullptr 或 "" 列出所有文件。
     * @return 完整文件路径列表（如 "/sdcard/myapp.bin"）。
     */
    virtual std::vector<std::string> SdListFiles(const char* dir, const char* ext) = 0;

    /// 获取文件大小（字节）。@return 文件不存在时返回 false。
    virtual bool SdFileSize(const char* path, size_t& out_size) = 0;

    /// 以只读模式打开 SD 卡上的文件。
    /// 调用方完成后必须调用 sdClose()。
    virtual FILE* SdOpenRead(const char* path) = 0;

    /// 关闭由 sdOpenRead() 打开的文件。
    virtual void SdClose(FILE* f) = 0;
};

}  // namespace launcher::hal
