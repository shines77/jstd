
#ifndef JSTD_HASH_DICTIONARY_TRAITS_H
#define JSTD_HASH_DICTIONARY_TRAITS_H

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

#include "jstd/basic/stddef.h"
#include "jstd/basic/stdint.h"
#include "jstd/basic/stdsize.h"

#include <string.h>
#include <string>

#include "jstd/hash/hash_helper.h"
#include "jstd/string/string_utils.h"

namespace jstd {

//
// Default jstd::dictionary<K, V> hash traits
//
template <typename Key, std::size_t HashFunc = HashFunc_Default>
struct DefaultDictionaryHasher {
    typedef Key             argument_type;
    typedef std::uint32_t   result_type;

    // The invalid hash value.
    static const result_type kInvalidHash = static_cast<result_type>(-1);
    // The replacement value for invalid hash value.
    static const result_type kReplacedHash = static_cast<result_type>(-2);

    DefaultDictionaryHasher() {}
    ~DefaultDictionaryHasher() {}

    result_type operator () (const argument_type & key) const {
        // All of the following two methods can get the hash value.
#if 0
        result_type hash_code = jstd::HashHelper<argument_type, result_type, HashFunc>::getHashCode(key);
#else
        jstd::hash<argument_type, result_type, HashFunc> hasher;
        result_type hash_code = hasher(key);
#endif
        // The hash code can't equal to kInvalidHash, replacement to kReplacedHash.
        if (likely(hash != kInvalidHash))
            return hash;
        else
            return kReplacedHash;
    }
};

//
// Default Dictionary<K, V> compare traits
//
template <typename Key, typename Value>
struct DefaultDictionaryComparer {
    typedef Key     key_type;
    typedef Value   value_type;

    DefaultDictionaryComparer() {}
    ~DefaultDictionaryComparer() {}

#if (STRING_COMPARE_MODE == STRING_COMPARE_LIBC)

    bool key_is_equals(const key_type & key1, const key_type & key2) const {
        return (::strcmp(key1.c_str(), key2. c_str()) == 0);
    }

    bool value_is_equals(const value_type & value1, const value_type & value2) const {
        return (::strcmp(value1.c_str(), value2. c_str()) == 0);
    }

    int key_compare(const key_type & key1, const key_type & key2) const {
        return ::strcmp(key1.c_str(), key2. c_str());
    }

    int value_compare(const value_type & value1, const value_type & value2) const {
        return ::strcmp(value1.c_str(), value2. c_str());
    }

#else // (STRING_COMPARE_MODE != STRING_COMPARE_LIBC)

    bool key_is_equals(const key_type & key1, const key_type & key2) const {
        return StrUtils::is_equals(key1, key2);
    }

    bool value_is_equals(const value_type & value1, const value_type & value2) const {
        return StrUtils::is_equals(value1, value2);
    }

    int key_compare(const key_type & key1, const key_type & key2) const {
        return StrUtils::compare(key1, key2);
    }

    int value_compare(const value_type & value1, const value_type & value2) const {
        return StrUtils::compare(value1, value2);
    }

#endif // STRING_COMPARE_MODE
};

//
// Default Dictionary<K, V> traits
//
template <typename Key, typename Value, std::size_t HashFunc = HashFunc_Default,
          typename Hasher = DefaultDictionaryHasher<Key, HashFunc>,
          typename Comparer = DefaultDictionaryComparer<Key, Value>>
struct DefaultDictionaryTraits {
    typedef Key                             key_type;
    typedef Value                           value_type;
    typedef typename Hasher::size_type      size_type;
    typedef typename Hasher::hash_type      hash_type;
    typedef typename Hasher::index_type     index_type;

    Hasher      hasher_;
    Comparer    comparer_;

    DefaultDictionaryTraits() {}
    ~DefaultDictionaryTraits() {}

    //
    // Hasher
    //
    hash_type hash_code(const key_type & key) const {
        return this->hasher_(key);
    }

    //
    // Comparer
    //
    bool key_is_equals(const key_type & key1, const key_type & key2) const {
        return this->comparer_.key_is_equals(key1, key2);
    }

    bool value_is_equals(const value_type & value1, const value_type & value2) const {
        return this->comparer_.value_is_equals(value1, value2);
    }

    int key_compare(const key_type & key1, const key_type & key2) const {
        return this->comparer_.key_compare(key1, key2);
    }

    int value_compare(const value_type & value1, const value_type & value2) const {
        return this->comparer_.value_compare(value1, value2);
    }
};

template <typename Key>
struct equal_to {
    typedef Key key_type;

    equal_to() {}
    ~equal_to() {}

    bool operator () (const key_type & key1, const key_type & key2) const {
        return (key1 == key2);
    }
};

template <>
struct equal_to<std::string> {
    typedef std::string key_type;

    equal_to() {}
    ~equal_to() {}

#if (STRING_COMPARE_MODE == STRING_COMPARE_LIBC)
    bool operator () (const key_type & key1, const key_type & key2) const {
        return libc::StrEqual(key1, key2);
    }
#else // (STRING_COMPARE_MODE != STRING_COMPARE_LIBC)
    bool operator () (const key_type & key1, const key_type & key2) const {
        return StrUtils::is_equals(key1, key2);
    }
#endif
};

template <>
struct equal_to<std::wstring> {
    typedef std::wstring key_type;

    equal_to() {}
    ~equal_to() {}

#if (STRING_COMPARE_MODE == STRING_COMPARE_LIBC)
    bool operator () (const key_type & key1, const key_type & key2) const {
        return libc::StrEqual(key1, key2);
    }
#else // (STRING_COMPARE_MODE != STRING_COMPARE_LIBC)
    bool operator () (const key_type & key1, const key_type & key2) const {
        return StrUtils::is_equals(key1, key2);
    }
#endif
};

} // namespace jstd

#endif // JSTD_HASH_DICTIONARY_TRAITS_H
