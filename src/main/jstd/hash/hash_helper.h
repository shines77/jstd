
#ifndef JSTD_HASH_HELPER_H
#define JSTD_HASH_HELPER_H

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

#include "jstd/basic/stddef.h"
#include "jstd/basic/stdint.h"
#include "jstd/basic/stdsize.h"

#include <cstdint>
#include <cstddef>
#include <string>
#include <type_traits>

#include "jstd/string/strlen.h"
#include "jstd/hash/hash.h"
#include "jstd/hash/crc32c.h"

#define HASH_HELPER_CHAR(KeyType, HashType, HashFuncId, HashFunc)               \
    template <>                                                                 \
    struct HashHelper<KeyType, HashType, HashFuncId> {                          \
        typedef typename std::remove_pointer<KeyType>::type char_type;          \
        typedef typename std::remove_pointer<                                   \
                    typename std::remove_cv<char_type>::type                    \
                >::type decay_type;                                             \
                                                                                \
        static HashType getHashCode(char_type * data, std::size_t length) {     \
            return (HashType)HashFunc((const char *)data,                       \
                                      length * sizeof(char_type));              \
        }                                                                       \
                                                                                \
        static HashType getHashCode(char_type * data) {                         \
            return (HashType)HashFunc((const char *)data,                       \
                             jstd::detail::StrLen((const decay_type *)data));   \
        }                                                                       \
    }

#define HASH_HELPER_CHAR_ALL(HashHelperClass, HashType, HashFuncId, HashFunc)   \
    HashHelperClass(char *,                 HashType, HashFuncId, HashFunc);    \
    HashHelperClass(const char *,           HashType, HashFuncId, HashFunc);    \
    HashHelperClass(unsigned char *,        HashType, HashFuncId, HashFunc);    \
    HashHelperClass(const unsigned char *,  HashType, HashFuncId, HashFunc);    \
    HashHelperClass(short *,                HashType, HashFuncId, HashFunc);    \
    HashHelperClass(const short *,          HashType, HashFuncId, HashFunc);    \
    HashHelperClass(unsigned short *,       HashType, HashFuncId, HashFunc);    \
    HashHelperClass(const unsigned short *, HashType, HashFuncId, HashFunc);    \
    HashHelperClass(wchar_t *,              HashType, HashFuncId, HashFunc);    \
    HashHelperClass(const wchar_t *,        HashType, HashFuncId, HashFunc);

#define HASH_HELPER_POD_ALL(HashHelperClass, HashType, HashFuncId, HashFunc)    \
    HashHelperClass(bool,                   HashType, HashFuncId, HashFunc);    \
    HashHelperClass(char,                   HashType, HashFuncId, HashFunc);    \
    HashHelperClass(unsigned char,          HashType, HashFuncId, HashFunc);    \
    HashHelperClass(short,                  HashType, HashFuncId, HashFunc);    \
    HashHelperClass(unsigned short,         HashType, HashFuncId, HashFunc);    \
    HashHelperClass(int,                    HashType, HashFuncId, HashFunc);    \
    HashHelperClass(unsigned int,           HashType, HashFuncId, HashFunc);    \
    HashHelperClass(long,                   HashType, HashFuncId, HashFunc);    \
    HashHelperClass(unsigned long,          HashType, HashFuncId, HashFunc);    \
    HashHelperClass(uint64_t,               HashType, HashFuncId, HashFunc);    \
    HashHelperClass(unsigned uint64_t,      HashType, HashFuncId, HashFunc);    \
    HashHelperClass(float,                  HashType, HashFuncId, HashFunc);    \
    HashHelperClass(double,                 HashType, HashFuncId, HashFunc);

namespace jstd {

enum hash_mode_t {
    HashFunc_Default,
    HashFunc_CRC32C,
    HashFunc_Time31,
    HashFunc_Time31Std,
    HashFunc_SHA1_MSG2,
    HashFunc_SHA1,
    HashFunc_Last
};

template <typename T, typename HashType = std::uint32_t,
          std::size_t HashFunc = HashFunc_Default>
struct HashHelper {
    typedef typename std::remove_pointer<
                typename std::remove_const<
                    typename std::remove_reference<T>::type
                >::type
            >::type     key_type;

    static
    typename std::enable_if<(std::is_pod<T>::value && !std::is_pointer<T>::value), HashType>::type
    getHashCode(const key_type key) {
        return hashes::Times31_std((const char *)&key, sizeof(key));
    }

    static
    typename std::enable_if<(!std::is_pod<T>::value && !std::is_pointer<T>::value), HashType>::type
    getHashCode(const key_type & key) {
        return hashes::Times31_std((const char *)&key, sizeof(key));
    }

    static
    typename std::enable_if<std::is_pointer<T>::value, HashType>::type
    getHashCode(const key_type * key) {
        return hashes::Times31_std((const char *)key, sizeof(key_type *));
    }
};

#if SUPPORT_SSE42_CRC32C

/***************************************************************************
template <>
struct HashHelper<const char *, std::uint32_t, HashFunc_CRC32C> {
    static std::uint32_t getHashCode(const char * data, size_t length) {
        return crc32::crc32c_x64(data, length);
    }
};
****************************************************************************/

HASH_HELPER_CHAR_ALL(HASH_HELPER_CHAR, std::uint32_t, HashFunc_CRC32C, jstd::crc32::crc32c_x64);

template <>
struct HashHelper<std::string, std::uint32_t, HashFunc_CRC32C> {
    static std::uint32_t getHashCode(const std::string & key) {
        return crc32::crc32c_x64(key.c_str(), key.size());
    }
};

#endif // SUPPORT_SSE42_CRC32C

/***************************************************************************
template <>
struct HashHelper<const char *, std::uint32_t, HashFunc_Time31> {
    static std::uint32_t getHashCode(const char * data, size_t length) {
        return hashes::Times31(data, length);
    }
};
****************************************************************************/

HASH_HELPER_CHAR_ALL(HASH_HELPER_CHAR, std::uint32_t, HashFunc_Time31, hashes::Times31);

template <>
struct HashHelper<std::string, std::uint32_t, HashFunc_Time31> {
    static std::uint32_t getHashCode(const std::string & key) {
        return hashes::Times31(key.c_str(), key.size());
    }
};

/***************************************************************************
template <>
struct HashHelper<const char *, std::uint32_t, HashFunc_Time31Std> {
    static std::uint32_t getHashCode(const char * data, size_t length) {
        return hashes::Times31_std(data, length);
    }
};
****************************************************************************/

HASH_HELPER_CHAR_ALL(HASH_HELPER_CHAR, std::uint32_t, HashFunc_Time31Std, hashes::Times31_std);

template <>
struct HashHelper<std::string, std::uint32_t, HashFunc_Time31Std> {
    static std::uint32_t getHashCode(const std::string & key) {
        return hashes::Times31_std(key.c_str(), key.size());
    }
};

#if SUPPORT_SMID_SHA

/***************************************************************************
template <>
struct HashHelper<const char *, std::uint32_t, HashFunc_SHA1_MSG2> {
    static std::uint32_t getHashCode(const char * data, size_t length) {
        return sha1::sha1_msg2(data, length);
    }
};
****************************************************************************/

HASH_HELPER_CHAR_ALL(HASH_HELPER_CHAR, std::uint32_t, HashFunc_SHA1_MSG2, jstd::sha1::sha1_msg2);

template <>
struct HashHelper<std::string, std::uint32_t, HashFunc_SHA1_MSG2> {
    static std::uint32_t getHashCode(const std::string & key) {
        return sha1::sha1_msg2(key.c_str(), key.size());
    }
};

/***************************************************************************
template <>
struct HashHelper<const char *, std::uint32_t, HashFunc_SHA1> {
    static std::uint32_t getHashCode(const char * data, size_t length) {
        //alignas(16) uint32_t sha1_state[5];
        //memcpy((void *)&sha1_state[0], (const void *)&jimi::s_sha1_state[0], sizeof(uint32_t) * 5);
        return sha1::sha1_x86(jimi::s_sha1_state, data, length);
    }
};
****************************************************************************/

HASH_HELPER_CHAR_ALL(HASH_HELPER_CHAR, std::uint32_t, HashFunc_SHA1, jstd::sha1::sha1_x86);

template <>
struct HashHelper<std::string, std::uint32_t, HashFunc_SHA1> {
    static std::uint32_t getHashCode(const std::string & key) {
        //alignas(16) uint32_t sha1_state[5];
        //memcpy((void *)&sha1_state[0], (const void *)&jimi::s_sha1_state[0], sizeof(uint32_t) * 5);
        return sha1::sha1_x86(jimi::s_sha1_state, key.c_str(), key.size());
    }
};

#endif // SUPPORT_SMID_SHA

template <typename T, typename HashType = std::uint32_t,
          std::size_t HashFunc = HashFunc_Default>
struct hash {
    typedef typename std::remove_pointer<
                typename std::remove_cv<
                    typename std::remove_reference<T>::type
                >::type
            >::type     key_type;

    hash() {}
    ~hash() {}

    HashType operator() (const key_type & key) const {
        return HashHelper<key_type, HashType, HashFunc>::getHashCode(key);
    }

    HashType operator() (const volatile key_type & key) const {
        return HashHelper<key_type, HashType, HashFunc>::getHashCode(key);
    }

    HashType operator() (const key_type * key) const {
        return HashHelper<key_type *, HashType, HashFunc>::getHashCode(key);
    }

    HashType operator() (const volatile key_type * key) const {
        return HashHelper<key_type *, HashType, HashFunc>::getHashCode(key);
    }
};

} // namespace jstd

#endif // JSTD_HASH_HELPER_H
