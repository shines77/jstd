
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
#include <cwchar>
#include <string>

#include "jstd/string/string_def.h"
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
    assert(str1 != nullptr);
    assert(str2 != nullptr);
    return (std::memcmp((const void *)str1, (const void *)str2, count * sizeof(CharTy)) == 0);
}


/*********************************************************************

// Maybe this is superfluous -- paint a snake with feet.

template <>
inline
bool StrEqual(const wchar_t * str1, const wchar_t * str2, std::size_t count) {
    assert(str1 != nullptr);
    assert(str2 != nullptr);
    return (std::wmemcmp(str1, str2, count * sizeof(wchar_t)) == 0);
}

**********************************************************************/

template <typename CharTy>
inline
bool StrEqual(const CharTy * str1, std::size_t len1, const CharTy * str2, std::size_t len2) {
    if (likely(len1 != len2)) {
        // The length of str1 and str2 is different, the string must be not equal.
        return false;
    }
    else {
        if (likely(str1 != str2)) {
            StrEqual(str1, str2, len1);
        }
        else {
            // The str1 and str2 is a same string.
            return true;
        }
    }
}

template <typename StringTy>
inline
bool StrEqualUnsafe(const StringTy & str1, const StringTy & str2) {
    return StrEqual(str1.c_str(), str1.size(), str2.c_str(), str2.size());
}

////////////////////////// stl::StrEqual<CharTy> - Safe //////////////////////////////

template <typename CharTy>
inline
bool StrEqualSafe(const CharTy * str1, const CharTy * str2, std::size_t count) {
    if (likely(((uintptr_t)str1 & (uintptr_t)str2) != 0)) {
        assert(str1 != nullptr && str2 != nullptr);
        return StrEqual(str1, str2, count);
    }
    else if (likely(((uintptr_t)str1 | (uintptr_t)str2) != 0)) {
        assert((str1 == nullptr && str2 != nullptr) ||
               (str1 != nullptr && str2 == nullptr));
        return (count == 0);
    }
    else {
        assert(str1 == nullptr && str2 == nullptr);
        return true;
    }
}

template <typename CharTy>
inline
bool StrEqualSafe(const CharTy * str1, std::size_t len1, const CharTy * str2, std::size_t len2) {
    if (likely(((uintptr_t)str1 & (uintptr_t)str2) != 0)) {
        assert(str1 != nullptr && str2 != nullptr);
        return StrEqual(str1, len1, str2, len2);
    }
    else if (likely(((uintptr_t)str1 | (uintptr_t)str2) != 0)) {
        assert((str1 == nullptr && str2 != nullptr) ||
               (str1 != nullptr && str2 == nullptr));
        // For efficiency, use this method faster than (len1 == 0 && len2 == 0).
        return (len1 == len2);
    }
    else {
        assert(str1 == nullptr && str2 == nullptr);
        return true;
    }
}

template <typename StringTy>
inline
bool StrEqualSafe(const StringTy & str1, const StringTy & str2) {
    return StrEqualSafe(str1.c_str(), str1.size(), str2.c_str(), str2.size());
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
    assert(str1 != nullptr);
    assert(str2 != nullptr);
    return std::memcmp((const void *)str1, (const void *)str2, count * sizeof(CharTy));
}

/*********************************************************************

// Maybe this is superfluous -- paint a snake with feet.

template <>
inline
bool StrCmp(const wchar_t * str1, const wchar_t * str2, std::size_t count) {
    assert(str1 != nullptr);
    assert(str2 != nullptr);
    return std::wmemcmp(str1, str2, count * sizeof(wchar_t));
}

**********************************************************************/

template <typename CharTy>
inline
int StrCmp(const CharTy * str1, std::size_t len1, const CharTy * str2, std::size_t len2) {
    assert(str1 != nullptr);
    assert(str2 != nullptr);

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
int StrCmpUnsafe(const StringTy & str1, const StringTy & str2) {
    return StrCmp(str1.c_str(), str1.size(), str2.c_str(), str2.size());
}

////////////////////////// stl::StrCmp<CharTy> - Safe //////////////////////////////

template <typename CharTy>
inline
int StrCmpSafe(const CharTy * str1, const CharTy * str2, std::size_t count) {
    if (likely(((uintptr_t)str1 & (uintptr_t)str2) != 0)) {
        assert(str1 != nullptr && str2 != nullptr);
        return StrCmp(str1, str2, count);
    }
    else if (likely(((uintptr_t)str1 | (uintptr_t)str2) != 0)) {
        assert((str1 == nullptr && str2 != nullptr) ||
               (str1 != nullptr && str2 == nullptr));
        if (likely(str1 != nullptr))
            return ((count != 0) ? CompareResult::IsBigger : CompareResult::IsEqual);
        else
            return ((count != 0) ? CompareResult::IsSmaller : CompareResult::IsEqual);
    }
    else {
        assert(str1 == nullptr && str2 == nullptr);
        return CompareResult::IsEqual;
    }
}

template <typename CharTy>
inline
int StrCmpSafe(const CharTy * str1, std::size_t len1, const CharTy * str2, std::size_t len2) {
    if (likely(((uintptr_t)str1 & (uintptr_t)str2) != 0)) {
        assert(str1 != nullptr && str2 != nullptr);
        return StrCmp(str1, len1, str2, len2);
    }
    else if (likely(((uintptr_t)str1 | (uintptr_t)str2) != 0)) {
        assert((str1 == nullptr && str2 != nullptr) ||
               (str1 != nullptr && str2 == nullptr));
        if (likely(str1 != nullptr))
            return ((len1 != 0) ? CompareResult::IsBigger : CompareResult::IsEqual);
        else
            return ((len2 != 0) ? CompareResult::IsSmaller : CompareResult::IsEqual);
    }
    else {
        assert(str1 == nullptr && str2 == nullptr);
        return CompareResult::IsEqual;
    }
}

template <typename StringTy>
inline
int StrCmpSafe(const StringTy & str1, const StringTy & str2) {
    return StrCmpSafe(str1.c_str(), str1.size(), str2.c_str(), str2.size());
}

///////////////////////////////////////////////////////////////////////////////

} // namespace stl
} // namespace jstd

#endif // JSTD_STRING_STL_H
