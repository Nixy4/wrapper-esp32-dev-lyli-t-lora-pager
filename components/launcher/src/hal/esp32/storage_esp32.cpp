#include "storage_esp32.hpp"

#include <cstring>
#include <dirent.h>
#include <sys/stat.h>

#include <esp_log.h>
#include <nvs.h>
#include <nvs_flash.h>

static const char* TAG = "Launcher|Storage";

namespace launcher::hal
{

// ─── 构造 / 析构 ────────────────────────────────────────────────────────────────────────────────────────────────────────

StorageEsp32::StorageEsp32(wrapper::Logger& logger,
                           wrapper::SpiBus& spi_bus,
                           gpio_num_t sd_cs_pin,
                           const char* sd_mount_point)
    : logger_(logger),
      spi_bus_(spi_bus),
      sd_cs_pin_(sd_cs_pin),
      sd_mount_point_(sd_mount_point),
      sd_(logger)
{
}

StorageEsp32::~StorageEsp32()
{
    if (sd_mounted_)
        sdUnmount();
}

// ─── NVS helpers ─────────────────────────────────────────────────────────────

bool StorageEsp32::nvsGet(const char* ns, const char* key, std::string& out)
{
    nvs_handle_t h;
    if (nvs_open(ns, NVS_READONLY, &h) != ESP_OK)
        return false;

    size_t len = 0;
    esp_err_t err = nvs_get_str(h, key, nullptr, &len);
    if (err != ESP_OK)
    {
        nvs_close(h);
        return false;
    }

    out.resize(len);
    err = nvs_get_str(h, key, out.data(), &len);
    nvs_close(h);

    if (err != ESP_OK)
        return false;

    // nvs_get_str 的 len 包含空终止符，删掉它
    if (!out.empty() && out.back() == '\0')
        out.pop_back();

    return true;
}

bool StorageEsp32::nvsSet(const char* ns, const char* key, const std::string& val)
{
    nvs_handle_t h;
    if (nvs_open(ns, NVS_READWRITE, &h) != ESP_OK)
        return false;

    esp_err_t err = nvs_set_str(h, key, val.c_str());
    if (err == ESP_OK)
        err = nvs_commit(h);

    nvs_close(h);
    return err == ESP_OK;
}

bool StorageEsp32::nvsDel(const char* ns, const char* key)
{
    nvs_handle_t h;
    if (nvs_open(ns, NVS_READWRITE, &h) != ESP_OK)
        return false;

    esp_err_t err = nvs_erase_key(h, key);
    if (err == ESP_OK)
        nvs_commit(h);

    nvs_close(h);
    // 将“键不存在”视为成功（幂等删除）
    return err == ESP_OK || err == ESP_ERR_NVS_NOT_FOUND;
}

bool StorageEsp32::nvsIterateKeys(const char* ns, std::vector<std::string>& keys)
{
    keys.clear();

    nvs_iterator_t it = nullptr;
    esp_err_t err = nvs_entry_find(NVS_DEFAULT_PART_NAME, ns, NVS_TYPE_STR, &it);

    while (err == ESP_OK && it != nullptr)
    {
        nvs_entry_info_t info;
        nvs_entry_info(it, &info);
        keys.emplace_back(info.key);
        err = nvs_entry_next(&it);
    }

    nvs_release_iterator(it);
    return true;
}

// ─── SD 卡 ───────────────────────────────────────────────────────────────────

bool StorageEsp32::sdMount()
{
    if (sd_mounted_)
        return true;

    wrapper::SdSpiDevConfig dev_cfg(sd_cs_pin_);
    wrapper::SdSpiMountConfig mnt_cfg(false, 5, 0, false);

    if (!sd_.Init(spi_bus_, dev_cfg, mnt_cfg, sd_mount_point_))
    {
        ESP_LOGE(TAG, "SD card mount failed");
        return false;
    }

    sd_mounted_ = true;
    ESP_LOGI(TAG, "SD mounted at %s", sd_mount_point_);
    return true;
}

bool StorageEsp32::sdUnmount()
{
    if (!sd_mounted_)
        return true;

    sd_.Deinit();
    sd_mounted_ = false;
    ESP_LOGI(TAG, "SD unmounted");
    return true;
}

std::vector<std::string> StorageEsp32::sdListFiles(const char* dir, const char* ext)
{
    std::vector<std::string> result;
    if (!sd_mounted_)
        return result;

    const char* search_dir = (dir && dir[0] != '\0') ? dir : sd_mount_point_;

    DIR* dp = opendir(search_dir);
    if (!dp)
    {
        ESP_LOGW(TAG, "Cannot open dir: %s", search_dir);
        return result;
    }

    struct dirent* de;
    while ((de = readdir(dp)) != nullptr)
    {
        if (de->d_type != DT_REG)
            continue;

        const char* name = de->d_name;

        // 扩展名过滤器
        if (ext && ext[0] != '\0')
        {
            const char* dot = strrchr(name, '.');
            if (!dot || strcasecmp(dot, ext) != 0)
                continue;
        }

        std::string full_path = std::string(search_dir) + "/" + name;
        result.push_back(std::move(full_path));
    }

    closedir(dp);
    return result;
}

bool StorageEsp32::sdFileSize(const char* path, size_t& out_size)
{
    struct stat st;
    if (stat(path, &st) != 0)
        return false;
    out_size = static_cast<size_t>(st.st_size);
    return true;
}

FILE* StorageEsp32::sdOpenRead(const char* path) { return fopen(path, "rb"); }

void StorageEsp32::sdClose(FILE* f)
{
    if (f)
        fclose(f);
}

}  // namespace launcher::hal
