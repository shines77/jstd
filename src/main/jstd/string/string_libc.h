
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
#include <cwchar>
#include <string>

#include "jstd/string/string_def.h"

#if defined(_WIN32) || defined(WIN32) || defined(OS_WINDOWS) || defined(__WINDOWS__)
// TODO:
#endif // _WIN32

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
    return std::strlen(str);
}

template <>
inline std::size_t StrLen(const wchar_t * str) {
    return std::wcslen(str);
}

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
    return (std::strcmp(str1, str2) == 0);
}

template <>
inline
bool StrEqual(const wchar_t * str1, const wchar_t * str2) {
    assert(str1 != nullptr);
    assert(str2 != nullptr);
    return (std::wcscmp(str1, str2) == 0);
}

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
    return (std::strncmp(str1, str2, count) == 0);
}

template <>
inline
bool StrEqual(const wchar_t * str1, const wchar_t * str2, std::size_t count) {
    return (std::wcsncmp(str1, str2, count) == 0);
}

template <typename CharTy>
inline
bool StrEqual(const CharTy * str1, std::size_t len1, const CharTy * str2, std::size_t len2) {
    if (likely(len1 != len2)) {
        // The length of between str1 and str2 is not equal.
        return false;
    }
    else {
        if (likely(str1 != str2)) {
            return StrEqual(str1, str2, len1);
        }
        else {
            // The str1 and str2 is a same string.
            return true;
        }
    }
}

template <typename StringTy>
inline
bool StrEqual(const StringTy & str1, const StringTy & str2) {
    typedef typename StringTy::value_type   char_type;
    typedef typename StringTy::size_type    size_type;

    const char_type * s1 = str1.data();
    const char_type * s2 = str2.data();
    size_type len1 = str1.size();
    size_type len2 = str2.size();

    if (likely(((uintptr_t)s1 & (uintptr_t)s2) != 0)) {
        assert(s1 != nullptr && s2 != nullptr);
        return StrEqual(s1, len1, s2, len2);
    }
    else if (likely(((uintptr_t)s1 | (uintptr_t)s2) != 0)) {
        assert((s1 == nullptr && s2 != nullptr) ||
               (s1 != nullptr && s2 == nullptr));
        return (len1 == 0 && len2 == 0);
    }
    else {
        assert(s1 == nullptr && s2 == nullptr);
        return true;
    }
}

///////////////////////////////////////////////////////////////////////////////
// libc::StrCmp<CharTy>()
///////////////////////////////////////////////////////////////////////////////

template <typename CharTy>
inline
int StrCmp(const CharTy * str1, const CharTy * str2) {
    while (*str1 == *str2) {
        if (unlikely(((*str1) & (*str2)) == 0)) {
            return CompareResult::IsEqual;
        }
        str1++;
        str2++;
    }

    if (*str1 > *str2)
        return CompareResult::IsBigger;
    else
        return CompareResult::IsSmaller;
}

template <>
inline
int StrCmp(const char * str1, const char * str2) {
    return std::strcmp(str1, str2);
}

template <>
inline
int StrCmp(const wchar_t * str1, const wchar_t * str2) {
    return std::wcscmp(str1, str2);
}

template <typename CharTy>
inline
int StrCmp(const CharTy * str1, const CharTy * str2, std::size_t count) {
    while (*str1 == *str2) {
        count--;
        if (unlikely(count == 0)) {
            return CompareResult::IsEqual;
        }
        if (unlikely(((*str1) & (*str2)) == 0)) {
            return CompareResult::IsEqual;
        }
        str1++;
        str2++;
    }

    if (*str1 > *str2)
        return CompareResult::IsBigger;
    else
        return CompareResult::IsSmaller;
}

template <>
inline
int StrCmp(const char * str1, const char * str2, std::size_t count) {
    return std::strncmp(str1, str2, count);
}

template <>
inline
int StrCmp(const wchar_t * str1, const wchar_t * str2, std::size_t count) {
    return std::wcsncmp(str1, str2, count);
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
            return CompareResult::IsBigger;
        else if (len1 < len2)
            return CompareResult::IsSmaller;
        else
            return CompareResult::IsEqual;
    }
}

template <typename StringTy>
inline
int StrCmp(const StringTy & str1, const StringTy & str2) {
    return StrCmp(str1.data(), str1.size(), str2.data(), str2.size());
}

template <typename StringTy>
inline
int StrCmpSafe(const StringTy & str1, const StringTy & str2) {
    typedef typename StringTy::value_type   char_type;
    typedef typename StringTy::size_type    size_type;

    const char_type * s1 = str1.data();
    const char_type * s2 = str2.data();
    size_type len1 = str1.size();
    size_type len2 = str2.size();

    if (likely(((uintptr_t)s1 & (uintptr_t)s2) != 0)) {
        assert(s1 != nullptr && s2 != nullptr);
        return StrCmp(s1, len1, s2, len2);
    }
    else if (likely(((uintptr_t)s1 | (uintptr_t)s2) != 0)) {
        assert((s1 == nullptr && s2 != nullptr) ||
               (s1 != nullptr && s2 == nullptr));
        if (likely(s1 != nullptr))
            return ((len1 != 0) ? CompareResult::IsBigger : CompareResult::IsEqual);
        else
            return ((len2 != 0) ? CompareResult::IsSmaller : CompareResult::IsEqual);
    }
    else {
        assert(s1 == nullptr && s2 == nullptr);
        return CompareResult::IsEqual;
    }
}

} // namespace libc
} // namespace jstd

#endif // JSTD_STRING_LIBC_H
