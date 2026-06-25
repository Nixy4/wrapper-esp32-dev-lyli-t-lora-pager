#pragma once

#include <cstdio>
#include <cstdint>
#include <string>
#include <vector>
#include <type_traits>
#include <utility>

namespace launcher::hal
{

/**
 * @brief CRTP + SFINAE 静态基类：平台无关的存储接口约束。
 *
 * 编译期验证（析构函数中 static_assert + 检测惯用法），Derived 须实现：
 *   bool NvsGet(const char* ns, const char* key, std::string& out)
 *   bool NvsSet(const char* ns, const char* key, const std::string& val)
 *   bool NvsDel(const char* ns, const char* key)
 *   bool NvsIterateKeys(const char* ns, std::vector<std::string>& keys)
 *   bool SdMount()
 *   bool SdUnmount()
 *   bool SdAvailable()
 *   std::vector<std::string> SdListFiles(const char* dir, const char* ext)
 *   bool SdFileSize(const char* path, size_t& out_size)
 *   FILE* SdOpenRead(const char* path)
 *   void SdClose(FILE* f)
 *
 * 签名错误时产生带方法名的 static_assert 错误，而非晦涩的模板展开错误。
 */
template<typename Derived>
class StorageBase
{
    // ── SFINAE 检测 helper（私有，内嵌于基类模板）─────────────────────────────

    template<typename T, typename = void>
    struct HasNvsGet : std::false_type {};
    template<typename T>
    struct HasNvsGet<T, std::void_t<std::enable_if_t<std::is_convertible_v<
        decltype(std::declval<T&>().NvsGet(
            std::declval<const char*>(), std::declval<const char*>(),
            std::declval<std::string&>())),
        bool>>>> : std::true_type {};

    template<typename T, typename = void>
    struct HasNvsSet : std::false_type {};
    template<typename T>
    struct HasNvsSet<T, std::void_t<std::enable_if_t<std::is_convertible_v<
        decltype(std::declval<T&>().NvsSet(
            std::declval<const char*>(), std::declval<const char*>(),
            std::declval<const std::string&>())),
        bool>>>> : std::true_type {};

    template<typename T, typename = void>
    struct HasNvsDel : std::false_type {};
    template<typename T>
    struct HasNvsDel<T, std::void_t<std::enable_if_t<std::is_convertible_v<
        decltype(std::declval<T&>().NvsDel(
            std::declval<const char*>(), std::declval<const char*>())),
        bool>>>> : std::true_type {};

    template<typename T, typename = void>
    struct HasNvsIterateKeys : std::false_type {};
    template<typename T>
    struct HasNvsIterateKeys<T, std::void_t<std::enable_if_t<std::is_convertible_v<
        decltype(std::declval<T&>().NvsIterateKeys(
            std::declval<const char*>(),
            std::declval<std::vector<std::string>&>())),
        bool>>>> : std::true_type {};

    template<typename T, typename = void>
    struct HasSdMount : std::false_type {};
    template<typename T>
    struct HasSdMount<T, std::void_t<std::enable_if_t<std::is_convertible_v<
        decltype(std::declval<T&>().SdMount()), bool>>>> : std::true_type {};

    template<typename T, typename = void>
    struct HasSdUnmount : std::false_type {};
    template<typename T>
    struct HasSdUnmount<T, std::void_t<std::enable_if_t<std::is_convertible_v<
        decltype(std::declval<T&>().SdUnmount()), bool>>>> : std::true_type {};

    template<typename T, typename = void>
    struct HasSdAvailable : std::false_type {};
    template<typename T>
    struct HasSdAvailable<T, std::void_t<std::enable_if_t<std::is_convertible_v<
        decltype(std::declval<T&>().SdAvailable()), bool>>>> : std::true_type {};

    template<typename T, typename = void>
    struct HasSdListFiles : std::false_type {};
    template<typename T>
    struct HasSdListFiles<T, std::void_t<std::enable_if_t<std::is_same_v<
        decltype(std::declval<T&>().SdListFiles(
            std::declval<const char*>(), std::declval<const char*>())),
        std::vector<std::string>>>>> : std::true_type {};

    template<typename T, typename = void>
    struct HasSdFileSize : std::false_type {};
    template<typename T>
    struct HasSdFileSize<T, std::void_t<std::enable_if_t<std::is_convertible_v<
        decltype(std::declval<T&>().SdFileSize(
            std::declval<const char*>(), std::declval<size_t&>())),
        bool>>>> : std::true_type {};

    template<typename T, typename = void>
    struct HasSdOpenRead : std::false_type {};
    template<typename T>
    struct HasSdOpenRead<T, std::void_t<std::enable_if_t<std::is_pointer_v<
        decltype(std::declval<T&>().SdOpenRead(
            std::declval<const char*>()))>>>> : std::true_type {};

    template<typename T, typename = void>
    struct HasSdClose : std::false_type {};
    template<typename T>
    struct HasSdClose<T, std::void_t<std::enable_if_t<std::is_void_v<
        decltype(std::declval<T&>().SdClose(
            std::declval<FILE*>()))>>>> : std::true_type {};

   protected:
    ~StorageBase() noexcept
    {
        static_assert(HasNvsGet<Derived>::value,
            "StorageBase<D>: D 须实现 bool NvsGet(const char* ns, const char* key, std::string& out)");
        static_assert(HasNvsSet<Derived>::value,
            "StorageBase<D>: D 须实现 bool NvsSet(const char* ns, const char* key, const std::string& val)");
        static_assert(HasNvsDel<Derived>::value,
            "StorageBase<D>: D 须实现 bool NvsDel(const char* ns, const char* key)");
        static_assert(HasNvsIterateKeys<Derived>::value,
            "StorageBase<D>: D 须实现 bool NvsIterateKeys(const char* ns, std::vector<std::string>& keys)");
        static_assert(HasSdMount<Derived>::value,
            "StorageBase<D>: D 须实现 bool SdMount()");
        static_assert(HasSdUnmount<Derived>::value,
            "StorageBase<D>: D 须实现 bool SdUnmount()");
        static_assert(HasSdAvailable<Derived>::value,
            "StorageBase<D>: D 须实现 bool SdAvailable()");
        static_assert(HasSdListFiles<Derived>::value,
            "StorageBase<D>: D 须实现 std::vector<std::string> SdListFiles(const char* dir, const char* ext)");
        static_assert(HasSdFileSize<Derived>::value,
            "StorageBase<D>: D 须实现 bool SdFileSize(const char* path, size_t& out_size)");
        static_assert(HasSdOpenRead<Derived>::value,
            "StorageBase<D>: D 须实现 FILE* SdOpenRead(const char* path)");
        static_assert(HasSdClose<Derived>::value,
            "StorageBase<D>: D 须实现 void SdClose(FILE* f)");
    }
};

}  // namespace launcher::hal
