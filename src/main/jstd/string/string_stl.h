
#ifndef JSTD_STRING_STL_H
#define JSTD_STRING_STL_H

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

#include "jstd/basic/stddef.h"
#include "jstd/basic/stdint.h"
#include "jstd/basic/stdsize.h"

#include <string.h>
#include <memory.h>
#include <assert.h>

#include <cstdint>
#include <cstddef>      // For std::size_t
#include <cstring>
#include <string>

#include "jstd/string/string_libc.h"

namespace jstd {
namespace stl {

///////////////////////////////////////////////////////////////////////////////
// stl::StrEqual<CharTy>()
///////////////////////////////////////////////////////////////////////////////

template <typename CharTy>
inline
bool StrEqual(const CharTy * str1, const CharTy * str2) {
    return libc::StrEqual(str1, str2);
}

template <typename CharTy>
inline
bool StrEqual(const CharTy * str1, const CharTy * str2, std::size_t count) {
    return (std::memcmp((const void *)str1, (const void *)str2, count * sizeof(CharTy)) == 0);
}

template <typename CharTy>
inline
bool StrEqual(const CharTy * str1, std::size_t len1, const CharTy * str2, std::size_t len2) {
    if (len1 == len2)
        return StrEqual(str1, str2, len1);
    else
        return false;
}

template <typename StringTy>
inline
bool StrEqual(const StringTy & str1, const StringTy & str2) {
    return StrEqual(str1.c_str(), str1.size(), str2.c_str(), str2.size());
}

///////////////////////////////////////////////////////////////////////////////
// stl::StrCmp<CharTy>()
///////////////////////////////////////////////////////////////////////////////

template <typename CharTy>
inline
int StrCmp(const CharTy * str1, const CharTy * str2) {
    return libc::StrCmp(str1, str2);
}

template <typename CharTy>
inline
int StrCmp(const CharTy * str1, const CharTy * str2, std::size_t count) {
    return std::memcmp((const void *)str1, (const void *)str2, count * sizeof(CharTy));
}

template <typename CharTy>
inline
int StrCmp(const CharTy * str1, std::size_t len1, const CharTy * str2, std::size_t len2) {
    std::size_t count = (len1 <= len2) ? len1 : len2;
    int compare = StrCmp(str1, str2, count);
    if (likely(compare != 0)) {
        return compare;
    }
    else {
        if (len1 > len2)
            return 1;
        else if (len1 < len2)
            return -1;
        else
            return 0;
    }
}

template <typename StringTy>
inline
int StrCmp(const StringTy & str1, const StringTy & str2) {
    return StrCmp(str1.c_str(), str1.size(), str2.c_str(), str2.size());
}

} // namespace stl
} // namespace jstd

#endif // JSTD_STRING_STL_H
