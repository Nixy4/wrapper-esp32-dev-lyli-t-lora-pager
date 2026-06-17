#pragma once

#include <cstdio>
#include <cstdint>
#include <string>
#include <vector>

namespace launcher::hal
{

/**
 * @brief Platform-agnostic storage abstraction covering NVS and SD card.
 *
 * NVS (Non-Volatile Storage) is used for:
 *   - App registry (partition label → display name)
 *   - Launcher settings (last-booted app, brightness, etc.)
 *
 * SD card is used for:
 *   - Browsing firmware .bin files for installation
 *
 * ESP32 implementation: esp32/storage_esp32.hpp
 */
class IStorage
{
   public:
    virtual ~IStorage() = default;

    // ── NVS ──────────────────────────────────────────────────────────────────

    /// Read a string value.  @return false if the key does not exist.
    virtual bool nvsGet(const char* ns, const char* key, std::string& out) = 0;

    /// Write or overwrite a string value.
    virtual bool nvsSet(const char* ns, const char* key, const std::string& val) = 0;

    /// Delete a key.  Returns true even if the key did not exist.
    virtual bool nvsDel(const char* ns, const char* key) = 0;

    /// List all keys in a namespace.
    virtual bool nvsIterateKeys(const char* ns, std::vector<std::string>& keys) = 0;

    // ── SD card ──────────────────────────────────────────────────────────────

    /// Mount the SD card.  Safe to call multiple times.
    virtual bool sdMount() = 0;

    /// Unmount the SD card.
    virtual bool sdUnmount() = 0;

    /// @return true if the SD card is currently mounted.
    virtual bool sdAvailable() = 0;

    /**
     * @brief List files matching @p ext in @p dir.
     *
     * @param dir  Directory path on the SD card (e.g. "/sdcard").
     * @param ext  File extension filter including dot (e.g. ".bin").
     *             Pass nullptr or "" to list all files.
     * @return Full file paths (e.g. "/sdcard/myapp.bin").
     */
    virtual std::vector<std::string> sdListFiles(const char* dir, const char* ext) = 0;

    /// Get file size in bytes.  @return false if the file does not exist.
    virtual bool sdFileSize(const char* path, size_t& out_size) = 0;

    /// Open a file on the SD card for reading.
    /// Caller must call sdClose() when done.
    virtual FILE* sdOpenRead(const char* path) = 0;

    /// Close a file opened with sdOpenRead().
    virtual void sdClose(FILE* f) = 0;
};

}  // namespace launcher::hal
