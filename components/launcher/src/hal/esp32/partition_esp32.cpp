#include "partition_esp32.hpp"

#include <cstring>
#include <esp_log.h>

static const char* TAG = "Launcher|Partition";

namespace launcher::hal
{

PartitionEsp32::PartitionEsp32(wrapper::Logger& logger) : logger_(logger), part_mgr_(logger) {}

// ─── planInstall ─────────────────────────────────────────────────────────────

bool PartitionEsp32::PlanInstall(const char* label, size_t image_size, InstallSlot& slot)
{
    // ── 情况 1：调用方指定了具体标签（重安装）────────────────────────────────────────
    if (label && label[0] != '\0')
    {
        const esp_partition_t* p =
            esp_partition_find_first(ESP_PARTITION_TYPE_APP, ESP_PARTITION_SUBTYPE_ANY, label);
        if (p)
        {
            if (image_size > p->size)
            {
                ESP_LOGE(TAG, "Image (%zu B) > partition '%s' (%lu B)", image_size, label, p->size);
                return false;
            }
            snprintf(slot.label, sizeof(slot.label), "%s", p->label);
            slot.subtype = static_cast<uint8_t>(p->subtype);
            slot.offset = p->address;
            slot.capacity = p->size;
            slot.is_new = false;
            ESP_LOGI(TAG, "Re-install: slot '%s' @ 0x%08lx, %lu B", slot.label, p->address,
                     p->size);
            return true;
        }
        ESP_LOGW(TAG, "Partition '%s' not found; auto-selecting", label);
    }

    // ── 情况 2：自动选择第一个合适的 OTA 槽位────────────────────────────────────
    constexpr int kMaxOtaSlots = 16;
    for (int i = 0; i < kMaxOtaSlots; ++i)
    {
        auto st = static_cast<esp_partition_subtype_t>(ESP_PARTITION_SUBTYPE_APP_OTA_0 + i);

        const esp_partition_t* p = esp_partition_find_first(ESP_PARTITION_TYPE_APP, st, nullptr);
        if (!p)
            break;  // 没有更多 OTA 槽位

        if (image_size > p->size)
        {
            ESP_LOGW(TAG, "Slot '%s' too small (%lu B < %zu B); skipping", p->label, p->size,
                     image_size);
            continue;
        }

        snprintf(slot.label, sizeof(slot.label), "%s", p->label);
        slot.subtype = static_cast<uint8_t>(p->subtype);
        slot.offset = p->address;
        slot.capacity = p->size;
        slot.is_new = true;
        ESP_LOGI(TAG, "Auto-selected slot '%s' @ 0x%08lx, %lu B", slot.label, p->address, p->size);
        return true;
    }

    ESP_LOGE(TAG, "No suitable OTA partition for image_size=%zu", image_size);
    return false;
}

// ─── flashBegin ──────────────────────────────────────────────────────────────

bool PartitionEsp32::FlashBegin(const InstallSlot& slot, size_t image_size)
{
    if (session_active_)
    {
        ESP_LOGW(TAG, "OTA 会话已处于活跃状态，中止前一次会话");
        FlashAbort();
    }

    const esp_partition_t* p = esp_partition_find_first(
        ESP_PARTITION_TYPE_APP, static_cast<esp_partition_subtype_t>(slot.subtype), slot.label);

    if (!p)
    {
        ESP_LOGE(TAG, "Cannot find partition '%s'", slot.label);
        return false;
    }

    const size_t erase_size = (image_size > 0) ? image_size : OTA_SIZE_UNKNOWN;

    esp_err_t err = esp_ota_begin(p, erase_size, &ota_handle_);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "esp_ota_begin failed: %s", esp_err_to_name(err));
        return false;
    }

    ota_partition_ = p;
    session_active_ = true;
    ESP_LOGI(TAG, "OTA session started → '%s'", slot.label);
    return true;
}

// ─── flashWrite ──────────────────────────────────────────────────────────────

bool PartitionEsp32::FlashWrite(const uint8_t* data, size_t len)
{
    if (!session_active_)
    {
        ESP_LOGE(TAG, "FlashWrite: no active OTA session");
        return false;
    }

    esp_err_t err = esp_ota_write(ota_handle_, data, len);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "esp_ota_write failed: %s", esp_err_to_name(err));
        return false;
    }
    return true;
}

// ─── flashEnd ────────────────────────────────────────────────────────────────

bool PartitionEsp32::FlashEnd()
{
    if (!session_active_)
    {
        ESP_LOGE(TAG, "FlashEnd: no active OTA session");
        return false;
    }

    esp_err_t err = esp_ota_end(ota_handle_);
    ota_handle_ = 0;
    session_active_ = false;

    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "esp_ota_end failed: %s", esp_err_to_name(err));
        ota_partition_ = nullptr;
        return false;
    }

    ESP_LOGI(TAG, "OTA write finalised on '%s'", ota_partition_->label);
    ota_partition_ = nullptr;
    return true;
}

// ─── flashAbort ──────────────────────────────────────────────────────────────

bool PartitionEsp32::FlashAbort()
{
    if (!session_active_)
        return true;

    esp_ota_abort(ota_handle_);
    ota_handle_ = 0;
    ota_partition_ = nullptr;
    session_active_ = false;
    ESP_LOGW(TAG, "OTA session aborted");
    return true;
}

// ─── setBootByLabel ──────────────────────────────────────────────────────────

bool PartitionEsp32::SetBootByLabel(const char* label)
{
    const esp_partition_t* p =
        esp_partition_find_first(ESP_PARTITION_TYPE_APP, ESP_PARTITION_SUBTYPE_ANY, label);

    if (!p)
    {
        ESP_LOGE(TAG, "SetBootByLabel: partition '%s' not found", label);
        return false;
    }

    esp_err_t err = esp_ota_set_boot_partition(p);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "esp_ota_set_boot_partition('%s') failed: %s", label, esp_err_to_name(err));
        return false;
    }

    ESP_LOGI(TAG, "Boot partition set to '%s'", label);
    return true;
}

// ─── getBootLabel ────────────────────────────────────────────────────────────

bool PartitionEsp32::GetBootLabel(char* label_out, size_t max_len)
{
    const esp_partition_t* p = esp_ota_get_boot_partition();
    if (!p)
        return false;

    snprintf(label_out, max_len, "%s", p->label);
    return true;
}

}  // namespace launcher::hal
