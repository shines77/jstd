
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
#include <exception>
#include <stdexcept>
#include <type_traits>

namespace jstd {

static const uint8_t HexStrs [16] = {
    '0', '1', '2', '3', '4', '5', '6', '7',
    '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'
};

static const uint8_t LowerHexStrs [16] = {
    '0', '1', '2', '3', '4', '5', '6', '7',
    '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'
};

template <std::ssize_t delta = 0>
inline
std::ssize_t get_data_length(unsigned char u8) {
    if (u8 < 10) {
        return 1 - delta;
    }
    else {
        if (u8 < 100)
            return 2 - delta;
        else
            return 3 - delta;
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
                    return 1 - delta;
                else
                    return 2 - delta;
            }
            else {
                if (u32 < 1000UL)
                    return 3 - delta;
                else
                    return 4 - delta;
            }
        }
        else {
            if (u32 < 1000000UL) {
                if (u32 < 100000UL)
                    return 5 - delta;
                else
                    return 6 - delta;
            }
            else {
                if (u32 < 10000000UL)
                    return 7 - delta;
                else
                    return 8 - delta;
            }
        }
    }
    else {
        if (u32 < 1000000000UL)
            return 9 - delta;
        else
            return 10 - delta;
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

void throw_sprintf_except(std::ssize_t err_id, std::ssize_t pos, std::size_t arg1)
{
    switch (err_id) {
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
    typedef basic_formatter<CharTy>                     this_type;

    size_type sprintf_calc_space_impl(const char_type * fmt) {
        assert(fmt != nullptr);
        ssize_type delta = 0;
        ssize_type err_id = Sprintf_Success;
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
                    err_id = Sprintf_InvalidArgmument_MissingParameter;
                    goto Sprintf_Throw_Except;
#else
                    delta--;
                    fmt++;
#endif
                }
                else {
                    // "%%" = "%"
                    delta--;
                    fmt++;
                }
            }
        }

Sprintf_Throw_Except:
        if (err_id != Sprintf_Success) {
            ssize_type pos = (fmt - fmt_first);
            throw_sprintf_except(err_id, pos, ex_arg1);
        }

        ssize_type scan_len = (fmt - fmt_first);
        assert(scan_len >= 0);
        assert(scan_len >= delta);

        size_type out_size = scan_len + delta;
        return out_size;
    }

    template <typename Arg1, typename ...Args>
    size_type sprintf_calc_space_impl(const char_type * fmt,
                                         Arg1 && arg1,
                                         Args && ... args) {
        assert(fmt != nullptr);
        ssize_type delta = 0;
        ssize_type err_id = Sprintf_Success;
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
                            u8 = static_cast<unsigned char>(arg1);
                            data_len = get_data_length<2>(u8);
                            delta += data_len;
                            fmt++;
                            break;
                        }

                    case uchar_type('d'):
                        {
                            i32 = static_cast<int>(arg1);
                            data_len = get_data_length<2>(i32);
                            delta += data_len;
                            fmt++;
                            break;
                        }

                    case uchar_type('e'):
                        {
                            fmt++;
                            break;
                        }

                    case uchar_type('f'):
                        {
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
                                err_id = Sprintf_InvalidArgmument_ErrorFloatType;
                                goto Sprintf_Throw_Except;
                            }

                            delta += 6 - 2;
                            fmt++;
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
                            u32 = static_cast<unsigned int>(arg1);
                            data_len = get_data_length<2>(u32);
                            delta += data_len;
                            fmt++;
                            break;
                        }

                    case uchar_type('x'):
                        {
                            fmt++;
                            break;
                        }

                    case uchar_type('\xff'):
                        {
                            err_id = Sprintf_BadFormat_UnknownSpecifier_FF;
                            goto Sprintf_Throw_Except;
                        }

                    default:
                        {
                            ex_arg1 = sp;
                            err_id = Sprintf_BadFormat_UnknownSpecifier;
                            goto Sprintf_Throw_Except;
                        }
                    }

                    size_type rest_size = sprintf_calc_space_impl(fmt, std::forward<Args>(args)...);
                    delta += rest_size;
                    goto Sprintf_Exit;
                }
                else {
                    // "%%" = "%"
                    delta--;
                    fmt++;
                }
            }
        }

Sprintf_Throw_Except:
        if (err_id != Sprintf_Success) {
            ssize_type pos = (fmt - fmt_first);
            throw_sprintf_except(err_id, pos, ex_arg1);
        }

Sprintf_Exit:
        assert(delta >= 0);
        ssize_type scan_len = (fmt - fmt_first);
        assert(scan_len >= 0);
        assert(scan_len >= delta);

        size_type out_size = scan_len + delta;
        return out_size;
    }

    template <typename ...Args>
    size_type sprintf_calc_space(const char_type * fmt, Args && ... args) {
        return sprintf_calc_space_impl(fmt, std::forward<Args>(args)...);
    }

    size_type sprintf_prepare_space_impl(std::basic_string<char_type> & str,
                                         const char_type * fmt) {
        assert(fmt != nullptr);
        ssize_type delta = 0;
        ssize_type err_id = Sprintf_Success;
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
                    err_id = Sprintf_InvalidArgmument_MissingParameter;
                    goto Sprintf_Throw_Except;
#else
                    delta--;
                    fmt++;
#endif
                }
                else {
                    // "%%" = "%"
                    delta--;
                    fmt++;
                }
            }
        }

Sprintf_Throw_Except:
        if (err_id != Sprintf_Success) {
            ssize_type pos = (fmt - fmt_first);
            throw_sprintf_except(err_id, pos, ex_arg1);
        }

        ssize_type scan_len = (fmt - fmt_first);
        assert(scan_len >= 0);
        assert(scan_len >= delta);

        size_type out_size = scan_len + delta;
        return out_size;
    }

    template <typename Arg1, typename ...Args>
    size_type sprintf_prepare_space_impl(std::basic_string<char_type> & str,
                                         const char_type * fmt,
                                         Arg1 && arg1,
                                         Args && ... args) {
        assert(fmt != nullptr);
        ssize_type delta = 0;
        ssize_type err_id = Sprintf_Success;
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
                            u8 = static_cast<unsigned char>(arg1);
                            data_len = get_data_length<2>(u8);
                            delta += data_len;
                            fmt++;
                            break;
                        }

                    case uchar_type('d'):
                        {
                            i32 = static_cast<int>(arg1);
                            data_len = get_data_length<2>(i32);
                            delta += data_len;
                            fmt++;
                            break;
                        }

                    case uchar_type('e'):
                        {
                            fmt++;
                            break;
                        }

                    case uchar_type('f'):
                        {
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
                                err_id = Sprintf_InvalidArgmument_ErrorFloatType;
                                goto Sprintf_Throw_Except;
                            }

                            delta += 6 - 2;
                            fmt++;
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
                            u32 = static_cast<unsigned int>(arg1);
                            data_len = get_data_length<2>(u32);
                            delta += data_len;
                            fmt++;
                            break;
                        }

                    case uchar_type('x'):
                        {
                            fmt++;
                            break;
                        }

                    case uchar_type('\xff'):
                        {
                            err_id = Sprintf_BadFormat_UnknownSpecifier_FF;
                            goto Sprintf_Throw_Except;
                        }

                    default:
                        {
                            ex_arg1 = sp;
                            err_id = Sprintf_BadFormat_UnknownSpecifier;
                            goto Sprintf_Throw_Except;
                        }
                    }

                    size_type rest_size = sprintf_prepare_space_impl(str, fmt, std::forward<Args>(args)...);
                    delta += rest_size;
                    goto Sprintf_Exit;
                }
                else {
                    // "%%" = "%"
                    delta--;
                    fmt++;
                }
            }
        }

Sprintf_Throw_Except:
        if (err_id != Sprintf_Success) {
            ssize_type pos = (fmt - fmt_first);
            throw_sprintf_except(err_id, pos, ex_arg1);
        }

Sprintf_Exit:
        assert(delta >= 0);
        ssize_type scan_len = (fmt - fmt_first);
        assert(scan_len >= 0);
        assert(scan_len >= delta);

        size_type out_size = scan_len + delta;
        return out_size;
    }

    template <typename ...Args>
    size_type sprintf_prepare_space(std::basic_string<char_type> & str,
                                    const char_type * fmt, Args && ... args) {
        return sprintf_prepare_space_impl(str, fmt, std::forward<Args>(args)...);
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
    size_type sprintf(std::basic_string<char_type> & str,
                      const char_type * fmt,
                      Args && ... args) {
        if (likely(fmt != nullptr)) {
            size_type out_size = sprintf_calc_space(fmt, std::forward<Args>(args)...);
            size_type old_size = str.size();
            size_type new_size = old_size + out_size;
            // Expand a char for '\0'
            str.reserve(new_size + 1);
            size_type output_size = sprintf_output(str, fmt, std::forward<Args>(args)...);
            assert(output_size == out_size);
            // Write null terminator '\0'
            str[new_size] = char_type('\0');
            return out_size;
        }
        return 0;
    }
};

typedef basic_formatter<char>       formatter;
typedef basic_formatter<wchar_t>    wformatter;

} // namespace jstd

#endif // JSTD_STRING_FORMATTER_H
