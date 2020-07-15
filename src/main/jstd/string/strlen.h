
#ifndef JSTD_STRLEN_H
#define JSTD_STRLEN_H

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

#include <string.h>
#include <wchar.h>
#include <assert.h>

#include <cstdint>
#include <cstddef>      // For std::size_t

namespace jstd {
namespace libc {

//////////////////////////////////////////
// libc::strlen<T>()
//////////////////////////////////////////

template <typename CharTy>
inline std::size_t StrLen(const CharTy * str) {
    const CharTy * start = str;
    while (CharTy(*str++) != CharTy('/0'));
    assert(str >= start);
    return std::size_t(str - start);
}

template <>
inline std::size_t StrLen(const char * str) {
    return (std::size_t)::strlen(str);
}

template <>
inline std::size_t StrLen(const unsigned char * str) {
    return (std::size_t)::strlen((const char *)str);
}

#if defined(_WIN32) || defined(WIN32) || defined(OS_WINDOWS) || defined(__WINDOWS__)

template <>
inline std::size_t StrLen(const short * str) {
    return (std::size_t)::wcslen((const wchar_t *)str);
}

template <>
inline std::size_t StrLen(const unsigned short * str) {
    return (std::size_t)::wcslen((const wchar_t *)str);
}

template <>
inline std::size_t StrLen(const wchar_t * str) {
    return (std::size_t)::wcslen((const wchar_t *)str);
}

#endif // _WIN32

} // namespace libc
} // namespace jstd

#endif // JSTD_STRLEN_H
