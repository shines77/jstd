
#ifndef JSTD_STRING_FORMATTER_H
#define JSTD_STRING_FORMATTER_H

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

#include "jstd/basic/stddef.h"
#include "jstd/basic/stdint.h"
#include "jstd/basic/stdsize.h"

#include <memory.h>
#include <string.h>
#include <tchar.h>
#include <assert.h>

#include <cstdint>
#include <cstddef>

#include <cstring>
#include <string>
#include <list>
#include <vector>
#include <exception>
#include <stdexcept>
#include <type_traits>

#include "jstd/string/char_traits.h"
#include "jstd/string/string_view.h"

namespace jstd {

static const uint8_t HexStrs [16] = {
    '0', '1', '2', '3', '4', '5', '6', '7',
    '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'
};

static const uint8_t LowerHexStrs [16] = {
    '0', '1', '2', '3', '4', '5', '6', '7',
    '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'
};

enum sprintf_except_id {
    Sprintf_Except_First = -16384,
    Sprintf_BadFormat_BrokenEscapeChar,
    Sprintf_BadFormat_UnknownSpecifier_FF,
    Sprintf_BadFormat_UnknownSpecifier,

    Sprintf_InvalidArgmument_MissingParameter,
    Sprintf_InvalidArgmument_ErrorFloatType,

    Sprintf_Success = 0,
    Sprintf_Except_Last
};

template <std::ssize_t delta = 0>
inline
std::ssize_t get_data_length(unsigned char u8) {
    if (u8 < 10) {
        return (1 - delta);
    }
    else {
        if (u8 < 100)
            return (2 - delta);
        else
            return (3 - delta);
    }
}

template <std::ssize_t delta = 0>
inline
std::ssize_t get_data_length(char i8) {
    unsigned char u8;
    if (i8 < 0)
        u8 = static_cast<unsigned char>(-i8);
    else
        u8 = static_cast<unsigned char>(i8);
    return get_data_length<delta>(u8);
}

template <std::ssize_t delta = 0>
inline
std::ssize_t get_data_length(unsigned int u32) {
    if (u32 < 100000000UL) {
        if (u32 < 10000UL) {
            if (u32 < 100UL) {
                if (u32 < 10UL)
                    return (1 - delta);
                else
                    return (2 - delta);
            }
            else {
                if (u32 < 1000UL)
                    return (3 - delta);
                else
                    return (4 - delta);
            }
        }
        else {
            if (u32 < 1000000UL) {
                if (u32 < 100000UL)
                    return (5 - delta);
                else
                    return (6 - delta);
            }
            else {
                if (u32 < 10000000UL)
                    return (7 - delta);
                else
                    return (8 - delta);
            }
        }
    }
    else {
        if (u32 < 1000000000UL)
            return (9 - delta);
        else
            return (10 - delta);
    }
}

template <std::ssize_t delta = 0>
inline
std::ssize_t get_data_length(int i32) {
    unsigned int u32;
    if (i32 < 0)
        u32 = static_cast<unsigned int>(-i32);
    else
        u32 = static_cast<unsigned int>(i32);
    return get_data_length<delta>(u32);
}

template <typename CharTy>
JSTD_FORCEINLINE
static CharTy to_hex_char(CharTy hex) {
    return ((hex >= 10) ? (hex - 10 + CharTy('A')) : (hex + CharTy('0')));
}

template <typename CharTy>
JSTD_FORCEINLINE
static CharTy to_lower_hex_char(CharTy hex) {
    return ((hex >= 10) ? (hex - 10 + CharTy('a')) : (hex + CharTy('0')));
}

template <typename CharTy>
static std::string to_hex(CharTy ch) {
    typedef typename make_unsigned_char<CharTy>::type UCharTy;
    UCharTy high = UCharTy(ch) / 16;
    std::string hex(to_hex_char(high), 1);
    UCharTy low = UCharTy(ch) % 16;
    hex += to_hex_char(low);
    return hex;
}

template <typename CharTy>
static std::string to_lower_hex(CharTy ch) {
    typedef typename make_unsigned_char<CharTy>::type UCharTy;
    UCharTy high = UCharTy(ch) / 16;
    std::string hex(to_lower_hex_char(high), 1);
    UCharTy low = UCharTy(ch) % 16;
    hex += to_lower_hex_char(low);
    return hex;
}

template <typename CharTy, typename Arg>
inline
std::size_t format_and_append_arg(jstd::basic_string_view<CharTy> & str,
                                Arg && arg, std::size_t size) {
    std::size_t data_len = size;
    str.append(size, CharTy('?'));
    return data_len;
}

template <typename CharTy>
inline
std::size_t format_and_append_arg(jstd::basic_string_view<CharTy> & str,
                                unsigned int u32, std::size_t digits) {
    std::size_t data_len = size;
    const CharTy * output_last = str.c_str() + digits;
    unsigned int num;
    while (u32 > 100UL) {
        num = u32 % 10UL;
        u32 /= 10UL;
        *output_last = static_cast<CharTy>(num);
        output_last--;

        num = u32 % 10UL;
        u32 /= 10UL;
        *output_last = static_cast<CharTy>(num);
        output_last--;
    }

    if (u32 > 10UL) {
        num = u32 % 10UL;
        u32 /= 10UL;
        *output_last = static_cast<CharTy>(num);
        output_last--;
    }

    assert(output_last == str.c_str());
    str.commit(digits);

    return data_len;
}

template <typename CharTy>
inline
std::size_t format_and_append_arg(std::basic_string<CharTy> & str,
                                int i32, std::size_t digits) {
    std::size_t data_len;
    unsigned int u32;
    if (likely(i32 >= 0)) {
        u32 = static_cast<unsigned int>(i32);
        data_len = format_and_append_arg(str, u32, digits);
    }
    else {
        str.push_back(char_type('-'));
        u32 = static_cast<unsigned int>(-i32);
        data_len = format_and_append_arg(str, u32, digits - 1) + 1;
    }
    return data_len;
}

template <typename CharTy>
struct sprintf_fmt_node {
    typedef CharTy          char_type;
    typedef std::size_t     size_type;
    typedef std::ssize_t    ssize_type;

    enum Type {
        isNotArg = 0,
        isArg
    };

    const char_type * fmt_first;
    const char_type * arg_first;
    size_type         arg_type;
    size_type         arg_size;

    sprintf_fmt_node() noexcept
        : fmt_first(nullptr),
          arg_first(nullptr),
          arg_type(isNotArg),
          arg_size(0) noexcept {
    }

    sprintf_fmt_node(const char_type * first, const char_type * arg) noexcept
        : fmt_first(first),
          arg_first(arg),
          arg_type(isNotArg),
          arg_size(0) {
    }

    sprintf_fmt_node(const char_type * first, const char_type * arg,
                     size_type type, size_type size) noexcept
        : fmt_first(first),
          arg_first(arg),
          arg_type(type),
          arg_size(size) {
    }

    bool is_arg() const {
        return (arg_type != isNotArg);
    }

    bool not_is_arg() const {
        return !(this->is_arg());
    }

    ssize_type fmt_length() const {
        return (arg_first - fmt_first);
    }

    const char_type * get_fmt_first() const {
        return fmt_first;
    }

    const char_type * get_arg_first() const {
        return arg_first;
    }

    size_type get_arg_type() const {
        return arg_type;
    }

    size_type get_arg_size() const {
        return arg_size;
    }

    void set_fmt_first(const char_type * first) {
        fmt_first = first;
    }

    void set_arg_first(const char_type * arg) {
        arg_first = arg;
    }

    void set_arg(size_type type, size_type size) {
        arg_type = type;
        arg_size = size;
    }
};

void throw_sprintf_except(std::ssize_t err_code, std::ssize_t pos, std::size_t arg1)
{
    switch (err_code) {
        case Sprintf_BadFormat_BrokenEscapeChar:
            throw std::runtime_error(
                "Bad format: broken escape char, *fmt pos: "
                + std::to_string(pos)
            );
            break;

        case Sprintf_BadFormat_UnknownSpecifier_FF:
            throw std::runtime_error(
                "Bad format: Unknown specifier: 0xFF, *fmt pos: "
                + std::to_string(pos)
            );
            break;

        case Sprintf_BadFormat_UnknownSpecifier:
            throw std::runtime_error(
                "Bad format: Unknown specifier: "
                + jstd::to_hex(static_cast<uint8_t>(arg1)) +
                ", *fmt pos: " + std::to_string(pos)
            );
            break;

        case Sprintf_InvalidArgmument_MissingParameter:
            throw std::invalid_argument(
                "Invalid argmument: missing parameter, *fmt pos: "
                + std::to_string(pos)
            );
            break;

        case Sprintf_InvalidArgmument_ErrorFloatType:
            throw std::invalid_argument(
                "Invalid argmument: error float type, *fmt pos: "
                + std::to_string(pos)
            );
            break;

        default:
            break;
    }
}

template <typename CharTy>
struct basic_formatter {
    typedef std::size_t     size_type;
    typedef std::ssize_t    ssize_type;

    typedef CharTy                                      char_type;
    typedef typename make_unsigned_char<CharTy>::type   uchar_type;

    typedef sprintf_fmt_node<CharTy>                    fmt_node_t;
    typedef basic_formatter<CharTy>                     this_type;

    size_type sprintf_calc_space_impl(const char_type * fmt) {
        assert(fmt != nullptr);
        ssize_type rest_size = 0;
        ssize_type err_code = Sprintf_Success;
        size_type ex_arg1 = 0;
        const char_type * fmt_first = fmt;
        while (*fmt != char_type('\0')) {
            if (likely(*fmt != char_type('%'))) {
                // is not "%": direct output
                fmt++;
            }
            else {
                fmt++;
                if (likely(*fmt != char_type('%'))) {
                    // "%?": specifier
                    uchar_type sp = static_cast<uchar_type>(*fmt);
#if 1
                    err_code = Sprintf_InvalidArgmument_MissingParameter;
                    goto Sprintf_Throw_Except;
#else
                    rest_size--;
                    fmt++;
#endif
                }
                else {
                    // "%%" = "%"
                    rest_size--;
                    fmt++;
                }
            }
        }

Sprintf_Throw_Except:
        if (err_code != Sprintf_Success) {
            ssize_type pos = (fmt - fmt_first);
            throw_sprintf_except(err_code, pos, ex_arg1);
        }

        ssize_type scan_len = (fmt - fmt_first);
        assert(scan_len >= 0);
        assert(scan_len >= rest_size);

        size_type total_size = scan_len + rest_size;
        return total_size;
    }

    template <typename Arg1, typename ...Args>
    size_type sprintf_calc_space_impl(const char_type * fmt,
                                      Arg1 && arg1,
                                      Args && ... args) {
        assert(fmt != nullptr);
        ssize_type rest_size = 0;
        ssize_type err_code = Sprintf_Success;
        size_type ex_arg1 = 0;
        const char_type * fmt_first = fmt;
        while (*fmt != char_type('\0')) {
            if (likely(*fmt != char_type('%'))) {
                // is not "%": direct output
                fmt++;
            }
            else {
                const char_type * arg_first = fmt;
                fmt++;
                if (likely(*fmt != char_type('%'))) {
                    // "%?": specifier
                    int i32;
                    unsigned int u32;
                    double d; float f;
                    long double ld;
                    unsigned char u8;
                    size_type data_len;

                    uchar_type sp = static_cast<uchar_type>(*fmt);
                    switch (sp) {
                    case uchar_type('\0'):
                        {
                            goto Sprintf_Exit;
                        }

                    case uchar_type('A'):
                        {
                            fmt++;
                            break;
                        }

                    case uchar_type('E'):
                        {
                            fmt++;
                            break;
                        }

                    case uchar_type('F'):
                        {
                            fmt++;
                            break;
                        }

                    case uchar_type('G'):
                        {
                            fmt++;
                            break;
                        }

                    case uchar_type('X'):
                        {
                            fmt++;
                            break;
                        }

                    case uchar_type('a'):
                        {
                            fmt++;
                            break;
                        }

                    case uchar_type('c'):
                        {
                            fmt++;
                            u8 = static_cast<unsigned char>(arg1);
                            data_len = get_data_length<0>(u8);
                            break;
                        }

                    case uchar_type('d'):
                        {
                            fmt++;
                            i32 = static_cast<int>(arg1);
                            data_len = get_data_length<0>(i32);
                            break;
                        }

                    case uchar_type('e'):
                        {
                            fmt++;
                            break;
                        }

                    case uchar_type('f'):
                        {
                            fmt++;
                            if (sizeof(Arg1) == sizeof(float)) {
                                f = static_cast<float>(arg1);
                            }
                            else if (sizeof(Arg1) == sizeof(double)) {
                                d = static_cast<double>(arg1);
                            }
                            else if (sizeof(Arg1) == sizeof(long double)) {
                                ld = static_cast<long double>(arg1);
                            }
                            else {
                                err_code = Sprintf_InvalidArgmument_ErrorFloatType;
                                goto Sprintf_Throw_Except;
                            }

                            data_len = 6;
                            break;
                        }

                    case uchar_type('g'):
                        {
                            fmt++;
                            break;
                        }

                    case uchar_type('i'):
                        {
                            fmt++;
                            break;
                        }

                    case uchar_type('n'):
                        {
                            fmt++;
                            break;
                        }

                    case uchar_type('o'):
                        {
                            fmt++;
                            break;
                        }

                    case uchar_type('p'):
                        {
                            fmt++;
                            break;
                        }

                    case uchar_type('s'):
                        {
                            fmt++;
                            break;
                        }

                    case uchar_type('u'):
                        {
                            fmt++;
                            u32 = static_cast<unsigned int>(arg1);
                            data_len = get_data_length<0>(u32);
                            break;
                        }

                    case uchar_type('x'):
                        {
                            fmt++;
                            break;
                        }

                    case uchar_type('\xff'):
                        {
                            err_code = Sprintf_BadFormat_UnknownSpecifier_FF;
                            goto Sprintf_Throw_Except;
                        }

                    default:
                        {
                            ex_arg1 = sp;
                            err_code = Sprintf_BadFormat_UnknownSpecifier;
                            goto Sprintf_Throw_Except;
                        }
                    }

                    rest_size += (ssize_type(data_len) - (fmt - arg_first));
                    size_type remain_size = sprintf_calc_space_impl(
                                                fmt, std::forward<Args>(args)...);
                    rest_size += remain_size;
                    goto Sprintf_Exit;
                }
                else {
                    // "%%" = "%"
                    rest_size--;
                    fmt++;
                }
            }
        }

Sprintf_Throw_Except:
        if (err_code != Sprintf_Success) {
            ssize_type pos = (fmt - fmt_first);
            throw_sprintf_except(err_code, pos, ex_arg1);
        }

Sprintf_Exit:
        assert(rest_size >= 0);
        ssize_type scan_len = (fmt - fmt_first);
        assert(scan_len >= 0);

        size_type total_size = scan_len + rest_size;
        return total_size;
    }

    template <typename Arg1, typename ...Args>
    size_type sprintf_output_impl(std::basic_string<char_type> & str,
                                  const char_type * fmt) {
        return 0;
    }

    template <typename Arg1, typename ...Args>
    size_type sprintf_output_impl(std::basic_string<char_type> & str,
                                  const char_type * fmt,
                                  Arg1 && arg1,
                                  Args && ... args) {
        return 0;
    }

    template <typename ...Args>
    size_type sprintf_output(std::basic_string<char_type> & str,
                             const char_type * fmt, Args && ... args) {
        return sprintf_output_impl(str, fmt, std::forward<Args>(args)...);
    }

    template <typename ...Args>
    size_type sprintf_calc_space(const char_type * fmt, Args && ... args) {
        return sprintf_calc_space_impl(fmt, std::forward<Args>(args)...);
    }

    size_type sprintf_prepare_space_impl(std::list<fmt_node_t> & fmt_list,
                                         const char_type * fmt) {
        assert(fmt != nullptr);
        ssize_type rest_size = 0;
        ssize_type err_code = Sprintf_Success;
        size_type ex_arg1 = 0;
        const char_type * fmt_first = fmt;
        const char_type * arg_first = nullptr;
        while (*fmt != char_type('\0')) {
            if (likely(*fmt != char_type('%'))) {
                // is not "%": direct output
                fmt++;
            }
            else {
                fmt++;
                if (likely(*fmt != char_type('%'))) {
                    // "%?": specifier
                    uchar_type sp = static_cast<uchar_type>(*fmt);
#if 1
                    err_code = Sprintf_InvalidArgmument_MissingParameter;
                    goto Sprintf_Throw_Except;
#else
                    rest_size--;
                    fmt++;
#endif
                }
                else {
                    // "%%" = "%"
                    rest_size--;
                    fmt++;
                }
            }
        }

Sprintf_Throw_Except:
        if (err_code != Sprintf_Success) {
            ssize_type pos = (fmt - fmt_first);
            throw_sprintf_except(err_code, pos, ex_arg1);
        }

        if (arg_first == nullptr) {
            fmt_node_t fmt_info(fmt_first, fmt);
            fmt_list.push_back(fmt_info);
        }

        ssize_type scan_len = (fmt - fmt_first);
        assert(scan_len >= 0);
        assert(scan_len >= rest_size);

        size_type total_size = scan_len + rest_size;
        return total_size;
    }

    template <typename Arg1, typename ...Args>
    size_type sprintf_prepare_space_impl(std::list<fmt_node_t> & fmt_list,
                                         const char_type * fmt,
                                         Arg1 && arg1,
                                         Args && ... args) {
        assert(fmt != nullptr);
        ssize_type rest_size = 0;
        ssize_type err_code = Sprintf_Success;
        size_type ex_arg1 = 0;
        const char_type * fmt_first = fmt;
        const char_type * arg_first = nullptr;
        while (*fmt != char_type('\0')) {
            if (likely(*fmt != char_type('%'))) {
                // is not "%": direct output
                fmt++;
            }
            else {
                arg_first = fmt;
                fmt++;
                if (likely(*fmt != char_type('%'))) {
                    // "%?": specifier
                    int i32;
                    unsigned int u32;
                    double d; float f;
                    long double ld;
                    unsigned char u8;
                    size_type data_len = 0;

                    uchar_type sp = static_cast<uchar_type>(*fmt);
                    switch (sp) {
                    case uchar_type('\0'):
                        {
                            goto Sprintf_Exit;
                        }

                    case uchar_type('A'):
                        {
                            fmt++;
                            break;
                        }

                    case uchar_type('E'):
                        {
                            fmt++;
                            break;
                        }

                    case uchar_type('F'):
                        {
                            fmt++;
                            break;
                        }

                    case uchar_type('G'):
                        {
                            fmt++;
                            break;
                        }

                    case uchar_type('X'):
                        {
                            fmt++;
                            break;
                        }

                    case uchar_type('a'):
                        {
                            fmt++;
                            break;
                        }

                    case uchar_type('c'):
                        {
                            fmt++;
                            u8 = static_cast<unsigned char>(arg1);
                            data_len = get_data_length<0>(u8);
                            break;
                        }

                    case uchar_type('d'):
                        {
                            fmt++;
                            i32 = static_cast<int>(arg1);
                            data_len = get_data_length<0>(i32);
                            break;
                        }

                    case uchar_type('e'):
                        {
                            fmt++;
                            break;
                        }

                    case uchar_type('f'):
                        {
                            fmt++;
                            if (sizeof(Arg1) == sizeof(float)) {
                                f = static_cast<float>(arg1);
                            }
                            else if (sizeof(Arg1) == sizeof(double)) {
                                d = static_cast<double>(arg1);
                            }
                            else if (sizeof(Arg1) == sizeof(long double)) {
                                ld = static_cast<long double>(arg1);
                            }
                            else {
                                err_code = Sprintf_InvalidArgmument_ErrorFloatType;
                                goto Sprintf_Throw_Except;
                            }

                            data_len = 6;
                            break;
                        }

                    case uchar_type('g'):
                        {
                            fmt++;
                            break;
                        }

                    case uchar_type('i'):
                        {
                            fmt++;
                            break;
                        }

                    case uchar_type('n'):
                        {
                            fmt++;
                            break;
                        }

                    case uchar_type('o'):
                        {
                            fmt++;
                            break;
                        }

                    case uchar_type('p'):
                        {
                            fmt++;
                            break;
                        }

                    case uchar_type('s'):
                        {
                            fmt++;
                            break;
                        }

                    case uchar_type('u'):
                        {
                            fmt++;
                            u32 = static_cast<unsigned int>(arg1);
                            data_len = get_data_length<0>(u32);
                            break;
                        }

                    case uchar_type('x'):
                        {
                            fmt++;
                            break;
                        }

                    case uchar_type('\xff'):
                        {
                            err_code = Sprintf_BadFormat_UnknownSpecifier_FF;
                            goto Sprintf_Throw_Except;
                        }

                    default:
                        {
                            ex_arg1 = sp;
                            err_code = Sprintf_BadFormat_UnknownSpecifier;
                            goto Sprintf_Throw_Except;
                        }
                    }

                    fmt_node_t arg_info(fmt_first, arg_first, fmt_node_t::isArg, data_len);
                    fmt_list.push_back(arg_info);

                    rest_size += (ssize_type(data_len) - (fmt - arg_first));
                    size_type remain_size = sprintf_prepare_space_impl(
                                                fmt_list, fmt,
                                                std::forward<Args>(args)...);
                    rest_size += remain_size;
                    goto Sprintf_Exit;
                }
                else {
                    // "%%" = "%"
                    rest_size--;
                    fmt++;
                }
            }
        }

Sprintf_Throw_Except:
        if (err_code != Sprintf_Success) {
            ssize_type pos = (fmt - fmt_first);
            throw_sprintf_except(err_code, pos, ex_arg1);
        }

        if (arg_first == nullptr) {
            fmt_node_t fmt_info(fmt_first, fmt);
            fmt_list.push_back(fmt_info);
        }

Sprintf_Exit:
        assert(rest_size >= 0);
        ssize_type scan_len = (fmt - fmt_first);
        assert(scan_len >= 0);

        size_type total_size = scan_len + rest_size;
        return total_size;
    }

    template <typename ...Args>
    size_type sprintf_prepare_space(std::list<fmt_node_t> & buf_list,
                                    const char_type * fmt, Args && ... args) {
        return sprintf_prepare_space_impl(buf_list, fmt, std::forward<Args>(args)...);
    }

    size_type sprintf_output_prepare_impl(jstd::basic_string_view<char_type> & str,
                                          std::list<fmt_node_t> & fmt_list) {
        size_type total_size = 0;
        if (!fmt_list.empty()) {
            fmt_node_t & fmt_info = fmt_list.front();
            if (fmt_info.fmt_length() > 0) {
                total_size += fmt_info.fmt_length();
                str.append(fmt_info.fmt_first, fmt_info.arg_first);
            }
            bool is_arg = fmt_info.is_arg();
            if (!is_arg) {
                fmt_list.pop_front();
            }
            else {
                throw std::runtime_error("Error: Wrong fmt_list!");
            }
        }

        return total_size;
    }

    template <typename Arg1, typename ...Args>
    size_type sprintf_output_prepare_impl(jstd::basic_string_view<char_type> & str,
                                          std::list<fmt_node_t> & fmt_list,
                                          Arg1 && arg1,
                                          Args && ... args) {
        size_type total_size = 0;
        if (!fmt_list.empty()) {
            fmt_node_t & fmt_info = fmt_list.front();
            if (fmt_info.fmt_length() > 0) {
                total_size += fmt_info.fmt_length();
                str.append(fmt_info.fmt_first, fmt_info.arg_first);
            }
            bool is_arg = fmt_info.is_arg();
            if (is_arg) {
                size_type arg_size = fmt_info.arg_size;
                size_type data_len = format_and_append_arg(str,
                                            std::forward<Arg1>(arg1),
                                            arg_size);
                fmt_list.pop_front();

                total_size += data_len;
                size_type remain_size = sprintf_output_prepare_impl(
                                            str, fmt_list,
                                            std::forward<Args>(args)...);
                total_size += remain_size;
            }
            else {
                size_type rest_args = sizeof...(args);
                if (rest_args > 0) {
                    throw std::runtime_error("Error: Missing argument!");
                }
            }
        }

        return total_size;
    }

    template <typename ...Args>
    size_type sprintf_output_prepare(std::basic_string<char_type> & str,
                                     std::list<fmt_node_t> & fmt_list,
                                     Args && ... args) {
        jstd::basic_string_view<char_type> str_view(str);
        return sprintf_output_prepare_impl(str_view, fmt_list, std::forward<Args>(args)...);
    }

    template <typename ...Args>
    size_type sprintf_no_prepare(std::basic_string<char_type> & str,
                                 const char_type * fmt,
                                 Args && ... args) {
        if (likely(fmt != nullptr)) {
            size_type prepare_size = sprintf_calc_space(fmt, std::forward<Args>(args)...);
            size_type old_size = str.size();
            size_type new_size = old_size + prepare_size;
            // Expand to newsize and reserve a null terminator '\0'.
            str.reserve(new_size + 1);
            size_type output_size = sprintf_output(str, fmt, std::forward<Args>(args)...);
            //assert(output_size == prepare_size);
            // Write the null terminator '\0'.
            str.push_back(char_type('\0'));
            return prepare_size;
        }

        return 0;
    }

    template <typename ...Args>
    size_type sprintf(std::basic_string<char_type> & str,
                      const char_type * fmt,
                      Args && ... args) {
        if (likely(fmt != nullptr)) {
            std::list<fmt_node_t> fmt_list;
            size_type prepare_size = sprintf_prepare_space(fmt_list, fmt, std::forward<Args>(args)...);
            size_type old_size = str.size();
            size_type new_size = old_size + prepare_size;
            // Expand to newsize and reserve a null terminator '\0'.
            str.reserve(new_size + 1);
            size_type output_size = sprintf_output_prepare(str, fmt_list, std::forward<Args>(args)...);
            assert(output_size == prepare_size);
            // Write the null terminator '\0'.
            str.push_back(char_type('\0'));
            return prepare_size;
        }

        return 0;
    }
};

typedef basic_formatter<char>       formatter;
typedef basic_formatter<wchar_t>    wformatter;

} // namespace jstd

#endif // JSTD_STRING_FORMATTER_H
