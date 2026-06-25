#pragma once

#include <cstdint>
#include <cstring>
#include <functional>
#include <type_traits>
#include <utility>

namespace launcher::hal
{

/**
 * @brief 描述为固件安装选定的 Flash 分区槽位。
 */
struct InstallSlot
{
    char label[17] = {};    ///< 物理分区标签（如 "ota_0"）
    uint8_t subtype = 0;    ///< ESP 分区子类型（ESP_PARTITION_SUBTYPE_APP_OTA_x）
    uint32_t offset = 0;    ///< 分区的 Flash 字节地址
    uint32_t capacity = 0;  ///< 分区大小（字节）
    bool is_new = true;     ///< true 表示新安装，false 表示重新刷入已有槽位
};

/**
 * @brief 进度回调：(bytes_written, total_bytes)。
 */
using FlashProgressCb = std::function<void(size_t written, size_t total)>;

/**
 * @brief CRTP + SFINAE 静态基类：平台无关的分区 / OTA 刷写接口约束。
 *
 * 编译期验证 Derived 须实现：
 *   bool PlanInstall(const char* label, size_t image_size, InstallSlot& slot)
 *   bool FlashBegin(const InstallSlot& slot, size_t image_size)
 *   bool FlashWrite(const uint8_t* data, size_t len)
 *   bool FlashEnd()
 *   bool FlashAbort()
 *   bool SetBootByLabel(const char* label)
 *   bool GetBootLabel(char* label_out, size_t max_len)
 */
template<typename Derived>
class PartitionBase
{
    template<typename T, typename = void>
    struct HasPlanInstall : std::false_type {};
    template<typename T>
    struct HasPlanInstall<T, std::void_t<std::enable_if_t<std::is_convertible_v<
        decltype(std::declval<T&>().PlanInstall(
            std::declval<const char*>(), std::declval<size_t>(),
            std::declval<InstallSlot&>())),
        bool>>>> : std::true_type {};

    template<typename T, typename = void>
    struct HasFlashBegin : std::false_type {};
    template<typename T>
    struct HasFlashBegin<T, std::void_t<std::enable_if_t<std::is_convertible_v<
        decltype(std::declval<T&>().FlashBegin(
            std::declval<const InstallSlot&>(), std::declval<size_t>())),
        bool>>>> : std::true_type {};

    template<typename T, typename = void>
    struct HasFlashWrite : std::false_type {};
    template<typename T>
    struct HasFlashWrite<T, std::void_t<std::enable_if_t<std::is_convertible_v<
        decltype(std::declval<T&>().FlashWrite(
            std::declval<const uint8_t*>(), std::declval<size_t>())),
        bool>>>> : std::true_type {};

    template<typename T, typename = void>
    struct HasFlashEnd : std::false_type {};
    template<typename T>
    struct HasFlashEnd<T, std::void_t<std::enable_if_t<std::is_convertible_v<
        decltype(std::declval<T&>().FlashEnd()), bool>>>> : std::true_type {};

    template<typename T, typename = void>
    struct HasFlashAbort : std::false_type {};
    template<typename T>
    struct HasFlashAbort<T, std::void_t<std::enable_if_t<std::is_convertible_v<
        decltype(std::declval<T&>().FlashAbort()), bool>>>> : std::true_type {};

    template<typename T, typename = void>
    struct HasSetBootByLabel : std::false_type {};
    template<typename T>
    struct HasSetBootByLabel<T, std::void_t<std::enable_if_t<std::is_convertible_v<
        decltype(std::declval<T&>().SetBootByLabel(
            std::declval<const char*>())),
        bool>>>> : std::true_type {};

    template<typename T, typename = void>
    struct HasGetBootLabel : std::false_type {};
    template<typename T>
    struct HasGetBootLabel<T, std::void_t<std::enable_if_t<std::is_convertible_v<
        decltype(std::declval<T&>().GetBootLabel(
            std::declval<char*>(), std::declval<size_t>())),
        bool>>>> : std::true_type {};

   protected:
    ~PartitionBase() noexcept
    {
        static_assert(HasPlanInstall<Derived>::value,
            "PartitionBase<D>: D 须实现 bool PlanInstall(const char* label, size_t image_size, InstallSlot& slot)");
        static_assert(HasFlashBegin<Derived>::value,
            "PartitionBase<D>: D 须实现 bool FlashBegin(const InstallSlot& slot, size_t image_size)");
        static_assert(HasFlashWrite<Derived>::value,
            "PartitionBase<D>: D 须实现 bool FlashWrite(const uint8_t* data, size_t len)");
        static_assert(HasFlashEnd<Derived>::value,
            "PartitionBase<D>: D 须实现 bool FlashEnd()");
        static_assert(HasFlashAbort<Derived>::value,
            "PartitionBase<D>: D 须实现 bool FlashAbort()");
        static_assert(HasSetBootByLabel<Derived>::value,
            "PartitionBase<D>: D 须实现 bool SetBootByLabel(const char* label)");
        static_assert(HasGetBootLabel<Derived>::value,
            "PartitionBase<D>: D 须实现 bool GetBootLabel(char* label_out, size_t max_len)");
    }
};

}  // namespace launcher::hal
