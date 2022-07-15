
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

#include "jstd/hasher/hash_helper.h"
#include "jstd/string/string_stl.h"
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
        if (likely(hash_code != kInvalidHash))
            return hash_code;
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

#if (STRING_UTILS_MODE != STRING_UTILS_SSE42)

    bool key_is_equal(const key_type & key1, const key_type & key2) const {
        return (::strcmp(key1.c_str(), key2. c_str()) == 0);
    }

    bool value_is_equal(const value_type & value1, const value_type & value2) const {
        return (::strcmp(value1.c_str(), value2. c_str()) == 0);
    }

    int key_compare(const key_type & key1, const key_type & key2) const {
        return ::strcmp(key1.c_str(), key2. c_str());
    }

    int value_compare(const value_type & value1, const value_type & value2) const {
        return ::strcmp(value1.c_str(), value2. c_str());
    }

#else // (STRING_UTILS_MODE == STRING_UTILS_SSE42)

    bool key_is_equal(const key_type & key1, const key_type & key2) const {
        return str_utils::is_equal(key1, key2);
    }

    bool value_is_equal(const value_type & value1, const value_type & value2) const {
        return str_utils::is_equal(value1, value2);
    }

    int key_compare(const key_type & key1, const key_type & key2) const {
        return str_utils::compare_safe(key1, key2);
    }

    int value_compare(const value_type & value1, const value_type & value2) const {
        return str_utils::compare_safe(value1, value2);
    }

#endif // STRING_UTILS_MODE
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
    bool key_is_equal(const key_type & key1, const key_type & key2) const {
        return this->comparer_.key_is_equal(key1, key2);
    }

    bool value_is_equal(const value_type & value1, const value_type & value2) const {
        return this->comparer_.value_is_equal(value1, value2);
    }

    int key_compare(const key_type & key1, const key_type & key2) const {
        return this->comparer_.key_compare(key1, key2);
    }

    int value_compare(const value_type & value1, const value_type & value2) const {
        return this->comparer_.value_compare(value1, value2);
    }
};

} // namespace jstd

#endif // JSTD_HASH_DICTIONARY_TRAITS_H
