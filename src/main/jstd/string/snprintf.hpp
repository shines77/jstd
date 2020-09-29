/*
    cpp-format - Code covered by the MIT License
    Author: mutouyun (http://orzz.org)
*/

#pragma once

#include <typeinfo>     // typeid
#include <type_traits>  // std::is_convertible, std::enable_if, ...
#include <string>       // std::string
#include <stdexcept>    // std::invalid_argument
#include <iostream>     // std::cout
#include <utility>      // std::forward, std::move
#include <cstdint>      // intmax_t, uintmax_t
#include <cstddef>      // size_t, ptrdiff_t
#include <cstdio>       // snprintf
#include <cstdarg>      // va_list, va_start, va_end

#if defined(__GNUC__)
#   include <cxxabi.h>  // abi::__cxa_demangle
#endif/*__GNUC__*/

#include "jstd/string/printf.hpp"

namespace format {

////////////////////////////////////////////////////////////////
/// Perpare for type-safe printf
////////////////////////////////////////////////////////////////

namespace detail_snprintf_ {

#if defined(__GNUC__)
#   pragma GCC diagnostic push
#   pragma GCC diagnostic ignored "-Wformat-security"
#endif/*__GNUC__*/
template <typename StringType, typename... A>
int impl_(StringType & str, const char * fmt, A &&... args)
{
    size_t old_size = str.size();
    int n = ::snprintf(nullptr, 0, fmt, std::forward<A>(args)...);
    if (n > 0)
    {
        str.resize(old_size + n);
        n = ::snprintf(const_cast<char *>(str.data()), n + 1, fmt, std::forward<A>(args)...);
        if (n != static_cast<int>(str.size() - old_size)) return n;
    }
    return n;
}
#if defined(__GNUC__)
#   pragma GCC diagnostic pop
#endif/*__GNUC__*/

} // namespace detail_snprintf_

////////////////////////////////////////////////////////////////
/// Print formatted data to output stream
////////////////////////////////////////////////////////////////

template <typename StringType, typename... A/*,
          typename std::enable_if<StringPred<StringType, typename StringType::char_type>::value, bool>::type = true*/>
int snprintf(StringType & str, const char * fmt, A &&... args)
{
    if (fmt == nullptr) return 0;
    detail_format_::check(fmt, std::forward<A>(args)...);
    //return 0;
    return detail_snprintf_::impl_(str, fmt, std::forward<A>(args)...);
}

} // namespace format
