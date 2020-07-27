
#ifndef JSTD_SUPPORT_POWEROF2_H
#define JSTD_SUPPORT_POWEROF2_H

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

#include "jstd/basic/stddef.h"
#include "jstd/basic/stdint.h"
#include "jstd/basic/stdsize.h"

#include "jstd/support/bitscan_reverse.h"
#include "jstd/support/bitscan_forward.h"

#include "jstd/type_traits.h"   // For jstd::integral_utils<T>

#include <stddef.h>
#include <assert.h>

#include <cstdint>
#include <cstddef>      // For std::size_t
#include <cassert>
#include <type_traits>
#include <limits>       // For std::numeric_limits<T>::max()

//////////////////////////////////////////////////////////////////////////
//
// Bit Twiddling Hacks
//
// http://graphics.stanford.edu/~seander/bithacks.html#RoundUpPowerOf2
//
//////////////////////////////////////////////////////////////////////////

namespace jstd {
namespace run_time {

template <typename SizeType>
inline bool is_pow2(SizeType n) {
    static_assert(std::is_integral<SizeType>::value,
                  "Error: is_pow2(SizeType n) -- SizeType must be a integral type.");
    typedef typename std::make_unsigned<SizeType>::type unsigned_type;
    unsigned_type x = static_cast<unsigned_type>(n);
    return ((x & (x - 1)) == 0);
}

template <typename SizeType>
inline SizeType verify_pow2(SizeType n) {
    static_assert(std::is_integral<SizeType>::value,
                  "Error: verify_pow2(SizeType n) -- SizeType must be a integral type.");
    typedef typename std::make_unsigned<SizeType>::type unsigned_type;
    unsigned_type x = static_cast<unsigned_type>(n);
    return static_cast<SizeType>(x & (x - 1));
}

#if JSTD_SUPPORT_X86_BITSCAN_INSTRUCTION

static inline
std::size_t round_down_to_pow2(std::size_t n)
{
    if (n <= 1) {
        return 0;
    }

    unsigned long index;
#if defined(WIN64) || defined(_WIN64) || defined(_M_X64) || defined(_M_AMD64) \
 || defined(_M_IA64) || defined(__amd64__) || defined(__x86_64__) || defined(_M_ARM64)
    unsigned char nonZero = __BitScanReverse64(index, n - 1);
    return (std::size_t(1) << index);
#else
    unsigned char nonZero = __BitScanReverse(index, n);
    return (std::size_t(1) << index);
#endif // __amd64__
}

static inline
std::size_t round_to_pow2(std::size_t n)
{
    unsigned long index;
#if defined(WIN64) || defined(_WIN64) || defined(_M_X64) || defined(_M_AMD64) \
 || defined(_M_IA64) || defined(__amd64__) || defined(__x86_64__) || defined(_M_ARM64)
    unsigned char nonZero = __BitScanReverse64(index, n);
    return (nonZero ? (std::size_t(1) << index) : 0ULL);
#else
    unsigned char nonZero = __BitScanReverse(index, n);
    return (nonZero ? (std::size_t(1) << index) : 0UL);
#endif // __amd64__
}

static inline
std::size_t round_up_to_pow2(std::size_t n)
{
    if (is_pow2(n)) {
        return n;
    }

    if (n <= ((std::numeric_limits<std::size_t>::max)() / 2 + 1)) {
        unsigned long index;
#if defined(WIN64) || defined(_WIN64) || defined(_M_X64) || defined(_M_AMD64) \
 || defined(_M_IA64) || defined(__amd64__) || defined(__x86_64__) || defined(_M_ARM64)
        unsigned char nonZero = __BitScanReverse64(index, n - 1);
        return (nonZero ? (std::size_t(1) << (index + 1)) : 0ULL);
#else
        unsigned char nonZero = __BitScanReverse(index, n);
        return (nonZero ? (std::size_t(1) << (index + 1)) : 0UL);
#endif // __amd64__
    }
    else {
        return (std::numeric_limits<std::size_t>::max)();
    }
}

static inline
std::size_t next_pow2(std::size_t n)
{
    if (n < ((std::numeric_limits<std::size_t>::max)() / 2 + 1)) {
        unsigned long index;
#if defined(WIN64) || defined(_WIN64) || defined(_M_X64) || defined(_M_AMD64) \
 || defined(_M_IA64) || defined(__amd64__) || defined(__x86_64__) || defined(_M_ARM64)
        unsigned char nonZero = __BitScanReverse64(index, n);
        return (nonZero ? (std::size_t(1) << (index + 1)) : 1ULL);
#else
        unsigned char nonZero = __BitScanReverse(index, n);
        return (nonZero ? (std::size_t(1) << (index + 1)) : 1UL);
#endif // __amd64__
    }
    else {
        return (std::numeric_limits<std::size_t>::max)();
    }
}

#else // !JSTD_SUPPORT_X86_BITSCAN_INSTRUCTION

#if defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable: 4293)
#endif

//
// N is power of 2, and [x] is rounding function.
//
// If n is power of 2:
//      N = 2 ^ [Log2(n)],
//
// If n is not power of 2:
//      (2 ^ [Log2(n) - 1]) < N <= 2 ^ [Log2(n)]
//
template <typename SizeType>
inline SizeType round_to_pow2(SizeType n) {
    static_assert(std::is_integral<SizeType>::value,
                  "Error: round_to_pow2(SizeType n) -- SizeType must be a integral type.");
    typedef typename std::make_unsigned<SizeType>::type unsigned_type;
    unsigned_type u = static_cast<unsigned_type>(n);
    if (is_pow2(u)) {
        return static_cast<SizeType>(u);
    }
    else {
        u = static_cast<unsigned_type>(u - 1);
        u = u | (u >> 1);
        u = u | (u >> 2);
        u = u | (u >> 4);
        u = u | (u >> 8);
        u = u | (u >> 16);
        if (jstd::integral_traits<unsigned_type>::bits >= 64) {
            u = u | (u >> 32);
        }
        if (jstd::integral_traits<unsigned_type>::bits >= 128) {
            u = u | (u >> 64);
        }
        return (u != jstd::integral_traits<unsigned_type>::max_num) ?
                static_cast<SizeType>((u + 1) / 2) :
                static_cast<SizeType>(jstd::integral_traits<unsigned_type>::max_power2);
    }
}

//
// N is power of 2, and [x] is rounding function.
//
//   (2 ^ [Log2(n) - 1]) <= N < (2 ^ [Log2(n)])
//
template <typename SizeType>
inline SizeType round_down_to_pow2(SizeType n) {
    static_assert(std::is_integral<SizeType>::value,
                  "Error: round_down_to_pow2(SizeType n) -- SizeType must be a integral type.");
    if (n != 0)
        return round_to_pow2<SizeType>(n - 1);
    else
        return 0;
}

//
// N is power of 2, and [x] is rounding function.
//
// If n is power of 2:
//      N = 2 ^ [Log2(n)],
//
// If n is not power of 2:
//      2 ^ [Log2(n)] < N <= 2 ^ ([Log2(n)] + 1)
//
template <typename SizeType>
inline SizeType round_up_to_pow2(SizeType n) {
    static_assert(std::is_integral<SizeType>::value,
                  "Error: round_up_to_pow2(SizeType n) -- SizeType must be a integral type.");
    typedef typename std::make_unsigned<SizeType>::type unsigned_type;
    unsigned_type u = static_cast<unsigned_type>(n);
    if (is_pow2(u)) {
        return static_cast<SizeType>(u);
    }
    else {
        u = static_cast<unsigned_type>(u - 1);
        u = u | (u >> 1);
        u = u | (u >> 2);
        u = u | (u >> 4);
        u = u | (u >> 8);
        u = u | (u >> 16);
        if (jstd::integral_traits<unsigned_type>::bits >= 64) {
            u = u | (u >> 32);
        }
        if (jstd::integral_traits<unsigned_type>::bits >= 128) {
            u = u | (u >> 64);
        }
        return (u != jstd::integral_traits<unsigned_type>::max_num) ?
                static_cast<SizeType>(u + 1) : static_cast<SizeType>(u);
    }
}

//
// N is power of 2, and [x] is rounding function.
//
//   2 ^ [Log2(n)] < N <= 2 ^ ([Log2(n)] + 1)
//
template <typename SizeType>
inline SizeType next_pow2(SizeType n) {
    static_assert(std::is_integral<SizeType>::value,
                  "Error: next_pow2(SizeType n) -- SizeType must be a integral type.");
    if (n < (std::numeric_limits<SizeType>::max)())
        return round_up_to_pow2<SizeType>(n + 1);
    else
        return n;
}

#if defined(_MSC_VER)
#pragma warning(pop)
#endif

#endif // JSTD_SUPPORT_X86_BITSCAN_INSTRUCTION

} // namespace run_time
} // namespace jstd

//////////////////////////////////////////////////////////////////////////////////
//
// Msvc compiler: Maxinum recursive depth is about 497 layer.
//
//////////////////////////////////////////////////////////////////////////////////

namespace jstd {
namespace compile_time {

//////////////////////////////////////////////////////////////////////////////////

//
// is_power2 = (N && ((N & (N - 1)) == 0);
// Here, N must be a unsigned number.
//
template <std::size_t N>
struct is_power2 {
    static const bool value = ((N & (N - 1)) == 0);
};

//////////////////////////////////////////////////////////////////////////////////

// struct round_to_pow2<N>

template <std::size_t N, std::size_t Power2>
struct round_to_pow2_impl {
    static const std::size_t max_power2 = jstd::integral_traits<std::size_t>::max_power2;
    static const std::size_t max_num = jstd::integral_traits<std::size_t>::max_num;
    static const std::size_t next_power2 = (Power2 < max_power2) ? (Power2 << 1) : 0;

    static const bool too_large = (N > max_power2);
    static const bool reach_limit = (Power2 == max_power2);

    static const std::size_t value = ((N >= max_power2) ? max_power2 :
           (((Power2 == max_power2) || (Power2 >= N)) ? (Power2 / 2) :
            round_to_pow2_impl<N, next_power2>::value));
};

template <std::size_t N>
struct round_to_pow2_impl<N, 0> {
    static const std::size_t value = jstd::integral_traits<size_t>::max_power2;
};

template <std::size_t N>
struct round_to_pow2 {
    static const std::size_t value = is_power2<N>::value ? N : round_to_pow2_impl<N, 1>::value;
};

//////////////////////////////////////////////////////////////////////////////////

// struct round_down_to_pow2<N>

template <std::size_t N>
struct round_down_to_pow2 {
    static const std::size_t value = (N != 0) ? round_to_pow2<N - 1>::value : 0;
};

//////////////////////////////////////////////////////////////////////////////////

// struct round_up_to_pow2<N>

template <std::size_t N, std::size_t Power2>
struct round_up_to_pow2_impl {
    static const std::size_t max_power2 = jstd::integral_traits<std::size_t>::max_power2;
    static const std::size_t max_num = jstd::integral_traits<std::size_t>::max_num;
    static const std::size_t next_power2 = (Power2 < max_power2) ? (Power2 << 1) : 0;

    static const bool too_large = (N >= max_power2);
    static const bool reach_limit = (Power2 == max_power2);

    static const std::size_t value = ((N > max_power2) ? max_num :
           (((Power2 == max_power2) || (Power2 >= N)) ? Power2 :
            round_up_to_pow2_impl<N, next_power2>::value));
};

template <std::size_t N>
struct round_up_to_pow2_impl<N, 0> {
    static const std::size_t value = jstd::integral_traits<std::size_t>::max_num;
};

template <std::size_t N>
struct round_up_to_pow2 {
    static const std::size_t value = is_power2<N>::value ? N : round_up_to_pow2_impl<N, 1>::value;
};

//////////////////////////////////////////////////////////////////////////////////

// struct next_pow2<N>

template <std::size_t N, std::size_t Power2>
struct next_pow2_impl {
    static const std::size_t max_power2 = jstd::integral_traits<std::size_t>::max_power2;
    static const std::size_t max_num = jstd::integral_traits<std::size_t>::max_num;
    static const std::size_t next_power2 = (Power2 < max_power2) ? (Power2 << 1) : 0;

    static const bool too_large = (N >= max_power2);
    static const bool reach_limit = (Power2 == max_power2);

    static const std::size_t value = ((N >= max_power2) ? max_num :
           (((Power2 == max_power2) || (Power2 > N)) ? Power2 :
            next_pow2_impl<N, next_power2>::value));
};

template <std::size_t N>
struct next_pow2_impl<N, 0> {
    static const std::size_t value = jstd::integral_traits<size_t>::max_num;
};

template <std::size_t N>
struct next_pow2 {
    static const std::size_t value = next_pow2_impl<N, 1>::value;
};

template <>
struct next_pow2<0> {
    static const std::size_t value = 1;
};

//////////////////////////////////////////////////////////////////////////////////

#if defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable: 4307)
#endif

//////////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////////

// struct round_to_power2_impl<N>

template <std::size_t N>
struct round_to_power2_impl {
    static const std::size_t max_num = jstd::integral_traits<std::size_t>::max_num;
    static const std::size_t max_power2 = jstd::integral_traits<std::size_t>::max_power2;
    static const std::size_t N1 = N - 1;
    static const std::size_t N2 = N1 | (N1 >> 1);
    static const std::size_t N3 = N2 | (N2 >> 2);
    static const std::size_t N4 = N3 | (N3 >> 4);
    static const std::size_t N5 = N4 | (N4 >> 8);
    static const std::size_t N6 = N5 | (N5 >> 16);
#if defined(WIN64) || defined(_WIN64) || defined(_M_X64) || defined(_M_AMD64) \
 || defined(_M_IA64) || defined(__amd64__) || defined(__x86_64__) || defined(_M_ARM64)
    static const std::size_t N7 = N6 | (N6 >> 32);
    static const std::size_t value = (N7 != max_num) ? ((N7 + 1) / 2) : max_power2;
#else
    static const std::size_t value = (N6 != max_num) ? ((N6 + 1) / 2) : max_power2;
#endif
};

template <std::size_t N>
struct round_to_power2 {
    static const std::size_t value = is_power2<N>::value ? N : round_to_power2_impl<N>::value;
};

// struct round_down_to_power2<N>

template <std::size_t N>
struct round_down_to_power2 {
    static const std::size_t value = (N != 0) ? round_to_power2<N - 1>::value : 0;
};

//////////////////////////////////////////////////////////////////////////////////

// struct round_up_to_power2<N>

template <std::size_t N>
struct round_up_to_power2_impl {
    static const std::size_t max_num = jstd::integral_traits<std::size_t>::max_num;
    static const std::size_t N1 = N - 1;
    static const std::size_t N2 = N1 | (N1 >> 1);
    static const std::size_t N3 = N2 | (N2 >> 2);
    static const std::size_t N4 = N3 | (N3 >> 4);
    static const std::size_t N5 = N4 | (N4 >> 8);
    static const std::size_t N6 = N5 | (N5 >> 16);
#if defined(WIN64) || defined(_WIN64) || defined(_M_X64) || defined(_M_AMD64) \
 || defined(_M_IA64) || defined(__amd64__) || defined(__x86_64__) || defined(_M_ARM64)
    static const std::size_t N7 = N6 | (N6 >> 32);
    static const std::size_t value = (N7 != max_num) ? (N7 + 1) : max_num;
#else
    static const std::size_t value = (N6 != max_num) ? (N6 + 1) : max_num;
#endif
};

template <std::size_t N>
struct round_up_to_power2 {
    static const std::size_t value = is_power2<N>::value ? N : round_up_to_power2_impl<N>::value;
};

//////////////////////////////////////////////////////////////////////////////////

// struct next_power2<N>

template <std::size_t N>
struct next_power2 {
    static const std::size_t max_num = jstd::integral_traits<std::size_t>::max_num;
    static const std::size_t value = (N < max_num) ? round_up_to_power2<N + 1>::value : max_num;
};

#if defined(_MSC_VER)
#pragma warning(pop)
#endif

//////////////////////////////////////////////////////////////////////////////////

} // namespace compile_time
} // namespace jstd

#endif // JSTD_SUPPORT_POWEROF2_H
