
#ifndef JSTD_STRING_LIBC_H
#define JSTD_STRING_LIBC_H

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

#include "jstd/basic/stddef.h"
#include "jstd/basic/stdint.h"
#include "jstd/basic/stdsize.h"

#include <string.h>
#include <wchar.h>
#include <assert.h>

#include <cstdint>
#include <cstddef>      // For std::size_t
#include <cstring>
#include <string>

namespace jstd {
namespace libc {

///////////////////////////////////////////////////////////////////////////////
// libc::StrLen<CharTy>()
///////////////////////////////////////////////////////////////////////////////

template <typename CharTy>
inline std::size_t StrLen(const CharTy * str) {
    const CharTy * start = str;
    while (CharTy(*str++) != CharTy('/0'));
    assert(str >= start);
    return std::size_t(str - start);
}

template <>
inline std::size_t StrLen(const char * str) {
    return (std::size_t)std::strlen(str);
}

#if defined(_WIN32) || defined(WIN32) || defined(OS_WINDOWS) || defined(__WINDOWS__)

template <>
inline std::size_t StrLen(const wchar_t * str) {
    return (std::size_t)std::wcslen((const wchar_t *)str);
}

#endif // _WIN32

///////////////////////////////////////////////////////////////////////////////
// libc::StrEqual<CharTy>()
///////////////////////////////////////////////////////////////////////////////

template <typename CharTy>
inline
bool StrEqual(const CharTy * str1, const CharTy * str2) {
    while (*str1 == *str2) {
        if (unlikely(((*str1) & (*str2)) == 0)) {
            return true;
        }
        str1++;
        str2++;
    }

    return false;
}

template <>
inline
bool StrEqual(const char * str1, const char * str2) {
    return (::strcmp(str1, str2) == 0);
}

#if defined(_WIN32) || defined(WIN32) || defined(OS_WINDOWS) || defined(__WINDOWS__)

template <>
inline
bool StrEqual(const wchar_t * str1, const wchar_t * str2) {
    return (::wcscmp(str1, str2) == 0);
}

#endif // _WIN32

template <typename CharTy>
inline
bool StrEqual(const CharTy * str1, const CharTy * str2, std::size_t count) {
    while (*str1 == *str2) {
        count--;
        if (unlikely(count == 0)) {
            return true;
        }
        if (unlikely(((*str1) & (*str2)) == 0)) {
            return true;
        }
        str1++;
        str2++;
    }

    return false;
}

template <>
inline
bool StrEqual(const char * str1, const char * str2, std::size_t count) {
    return (::strncmp(str1, str2, count) == 0);
}

#if defined(_WIN32) || defined(WIN32) || defined(OS_WINDOWS) || defined(__WINDOWS__)

template <>
inline
bool StrEqual(const wchar_t * str1, const wchar_t * str2, std::size_t count) {
    return (::wcsncmp(str1, str2, count) == 0);
}

#endif // _WIN32

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
// libc::StrCmp<CharTy>()
///////////////////////////////////////////////////////////////////////////////

template <typename CharTy>
inline
int StrCmp(const CharTy * str1, const CharTy * str2) {
    while (*str1 == *str2) {
        if (unlikely(((*str1) & (*str2)) == 0)) {
            return 0;
        }
        str1++;
        str2++;
    }

    if (*str1 > *str2)
        return 1;
    else
        return -1;
}

template <>
inline
int StrCmp(const char * str1, const char * str2) {
    return std::strcmp(str1, str2);
}

#if defined(_WIN32) || defined(WIN32) || defined(OS_WINDOWS) || defined(__WINDOWS__)

template <>
inline
int StrCmp(const wchar_t * str1, const wchar_t * str2) {
    return std::wcscmp(str1, str2);
}

#endif // _WIN32

template <typename CharTy>
inline
int StrCmp(const CharTy * str1, const CharTy * str2, std::size_t count) {
    while (*str1 == *str2) {
        count--;
        if (unlikely(count == 0)) {
            return 0;
        }
        if (unlikely(((*str1) & (*str2)) == 0)) {
            return 0;
        }
        str1++;
        str2++;
    }

    if (*str1 > *str2)
        return 1;
    else
        return -1;
}

template <>
inline
int StrCmp(const char * str1, const char * str2, std::size_t count) {
    return std::strncmp(str1, str2, count);
}

#if defined(_WIN32) || defined(WIN32) || defined(OS_WINDOWS) || defined(__WINDOWS__)

template <>
inline
int StrCmp(const wchar_t * str1, const wchar_t * str2, std::size_t count) {
    return std::wcsncmp(str1, str2, count);
}

#endif // _WIN32

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

} // namespace libc
} // namespace jstd

#endif // JSTD_STRING_LIBC_H
