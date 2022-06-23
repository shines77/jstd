
#ifndef JSTD_HASH_EQUAL_TO_H
#define JSTD_HASH_EQUAL_TO_H

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

#include "jstd/basic/stddef.h"
#include "jstd/basic/stdint.h"
#include "jstd/basic/stdsize.h"

#include <string>

#include "jstd/string/string_utils.h"

namespace jstd {

template <typename Key>
struct equal_to {
    typedef Key key_type;

    bool operator () (const key_type & key1, const key_type & key2) const {
        return (key1 == key2);
    }
};

template <>
struct equal_to<std::string> {
    typedef std::string key_type;

    bool operator () (const key_type & key1, const key_type & key2) const {
        return str_utils::is_equal_unsafe(key1, key2);
    }
};

template <>
struct equal_to<std::wstring> {
    typedef std::wstring key_type;

    bool operator () (const key_type & key1, const key_type & key2) const {
        return str_utils::is_equal_unsafe(key1, key2);
    }
};

} // namespace jstd

#endif // JSTD_HASH_EQUAL_TO_H
