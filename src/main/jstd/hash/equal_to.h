
#ifndef JSTD_HASH_EQUAL_TO_H
#define JSTD_HASH_EQUAL_TO_H

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

#include "jstd/basic/stddef.h"
#include "jstd/basic/stdint.h"
#include "jstd/basic/stdsize.h"

#include <string.h>
#include <string>

#include "jstd/string/string_stl.h"
#include "jstd/string/string_utils.h"

namespace jstd {

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

#if (STRING_UTILS_MODE == STRING_UTILS_LIBC)
    bool operator () (const key_type & key1, const key_type & key2) const {
        return stl::StrEqual(key1, key2);
    }
#else // (STRING_UTILS_MODE != STRING_UTILS_LIBC)
    bool operator () (const key_type & key1, const key_type & key2) const {
        return str_utils::is_equals(key1, key2);
    }
#endif
};

template <>
struct equal_to<std::wstring> {
    typedef std::wstring key_type;

    equal_to() {}
    ~equal_to() {}

#if (STRING_UTILS_MODE == STRING_UTILS_LIBC)
    bool operator () (const key_type & key1, const key_type & key2) const {
        return stl::StrEqual(key1, key2);
    }
#else // (STRING_UTILS_MODE != STRING_UTILS_LIBC)
    bool operator () (const key_type & key1, const key_type & key2) const {
        return str_utils::is_equals(key1, key2);
    }
#endif
};

} // namespace jstd

#endif // JSTD_HASH_EQUAL_TO_H
