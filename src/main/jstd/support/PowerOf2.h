
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
#include <cstddef>
#include <cassert>
#include <type_traits>
#include <limits>       // For std::numeric_limits<T>::max()

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

static inline
std::size_t round_down_to_pow2(std::size_t n)
{
    unsigned long index;
#if defined(WIN64) || defined(_WIN64) || defined(_M_X64) || defined(_M_AMD64) \
 || defined(_M_IA64) || defined(__amd64__) || defined(__x86_64__) || defined(_M_ARM64)
    unsigned char nonZero = __BitScanReverse64(index, n + 1);
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

    if (n < std::numeric_limits<std::size_t>::max() / 2) {
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
        return std::numeric_limits<std::size_t>::max();
    }
}

static inline
std::size_t next_pow2(std::size_t n)
{
    if (n < std::numeric_limits<std::size_t>::max() / 2) {
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
        return std::numeric_limits<std::size_t>::max();
    }
}

//
// N is power of 2, and [x] is rounding function.
//
//   (2 ^ [Log2(n) - 1]) <= N < (2 ^ [Log2(n)])
//
template <typename SizeType>
inline SizeType round_down_to_power2(SizeType n) {
    static_assert(std::is_integral<SizeType>::value,
                  "Error: round_down_to_power2(SizeType n) -- SizeType must be a integral type.");
    if (n < std::numeric_limits<SizeType>::max())
        return round_to_power2<SizeType>(n - 1);
    else
        return n;
}

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
inline SizeType round_to_power2(SizeType n) {
    static_assert(std::is_integral<SizeType>::value,
                  "Error: round_to_power2(SizeType n) -- SizeType must be a integral type.");
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
        if (jstd::integral_utils<unsigned_type>::bits >= 64) {
            u = u | (u >> 32);
        }
        if (jstd::integral_utils<unsigned_type>::bits >= 128) {
            u = u | (u >> 64);
        }
        return (u < jstd::integral_utils<unsigned_type>::max_num) ?
                static_cast<SizeType>((u + 1) / 2) :
                static_cast<SizeType>(jstd::integral_utils<unsigned_type>::max_power2);
    }
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
inline SizeType round_up_to_power2(SizeType n) {
    static_assert(std::is_integral<SizeType>::value,
                  "Error: round_up_to_power2(SizeType n) -- SizeType must be a integral type.");
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
        if (jstd::integral_utils<unsigned_type>::bits >= 64) {
            u = u | (u >> 32);
        }
        if (jstd::integral_utils<unsigned_type>::bits >= 128) {
            u = u | (u >> 64);
        }
        return (u < jstd::integral_utils<unsigned_type>::max_num) ?
                static_cast<SizeType>(u + 1) : static_cast<SizeType>(u);
    }
}

//
// N is power of 2, and [x] is rounding function.
//
//   2 ^ [Log2(n)] < N <= 2 ^ ([Log2(n)] + 1)
//
template <typename SizeType>
inline SizeType next_power2(SizeType n) {
    static_assert(std::is_integral<SizeType>::value,
                  "Error: next_power2(SizeType n) -- SizeType must be a integral type.");
    if (n < std::numeric_limits<SizeType>::max())
        return round_up_to_power2<SizeType>(n + 1);
    else
        return n;
}

} // namespace run_time
} // namespace jstd

//////////////////////////////////////////////////////////////////////////////////

namespace jstd {
namespace compile_time {

//
// is_pow_of_2 = (N && ((N & (N - 1)) == 0);  // here, N must is unsigned number
//
template <std::size_t N>
struct is_power2 {
    static const bool value = ((N & (N - 1)) == 0);
};

template <>
struct is_power2<0> {
    static const bool value = true;
};

//////////////////////////////////////////////////////////////////////////////////
//
// Msvc: Maxinum recursive depth is about 497 layer.
//

// struct round_to_pow2<N>

template <std::size_t N, std::size_t Power2>
struct round_to_pow2_impl {
    static const size_t max_power2 = jstd::integral_utils<std::size_t>::max_power2;
    static const size_t max_num = jstd::integral_utils<std::size_t>::max_num;
    static const size_t nextPower2 = (Power2 < max_power2) ? (Power2 << 1) : 0;

    static const bool too_large = (N > max_power2);
    static const bool reach_limit = (Power2 == max_power2);

    static const size_t value = ((N > max_power2) ? max_num :
           (((Power2 == max_power2) || (Power2 >= N)) ? Power2 :
            round_to_pow2_impl<N, nextPower2>::value));
};

template <std::size_t N>
struct round_to_pow2_impl<N, 0> {
    static const std::size_t value = jstd::integral_utils<size_t>::max_num;
};

template <std::size_t N>
struct round_to_pow2 {
    static const std::size_t value = round_to_pow2_impl<N, 1>::value;
};

template <>
struct round_to_pow2<0> {
    static const std::size_t value = 0;
};

// struct round_up_to_pow2<N>

template <std::size_t N, std::size_t Power2>
struct round_up_to_pow2_impl {
    static const size_t max_power2 = jstd::integral_utils<std::size_t>::max_power2;
    static const size_t max_num = jstd::integral_utils<std::size_t>::max_num;
    static const size_t nextPower2 = (Power2 < max_power2) ? (Power2 << 1) : 0;

    static const bool too_large = (N >= max_power2);
    static const bool reach_limit = (Power2 == max_power2);

    static const size_t value = ((N >= max_power2) ? max_num :
           (((Power2 == max_power2) || (Power2 > N)) ? Power2 :
            round_up_to_pow2_impl<N, nextPower2>::value));
};

template <std::size_t N>
struct round_up_to_pow2_impl<N, 0> {
    static const std::size_t value = jstd::integral_utils<std::size_t>::max_num;
};

template <std::size_t N>
struct round_up_to_pow2 {
    static const std::size_t value = round_up_to_pow2_impl<N, 1>::value;
};

template <>
struct round_up_to_pow2<0> {
    static const std::size_t value = 1;
};

//////////////////////////////////////////////////////////////////////////////////

#if defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable: 4307)
#endif

// struct round_to_power2_impl<N>

template <std::size_t N>
struct round_to_power2_impl {
    static const std::size_t max_num = jstd::integral_utils<std::size_t>::max_num;
    static const std::size_t N1 = N;
    static const std::size_t N2 = N1 | (N1 >> 1);
    static const std::size_t N3 = N2 | (N2 >> 2);
    static const std::size_t N4 = N3 | (N3 >> 4);
    static const std::size_t N5 = N4 | (N4 >> 8);
    static const std::size_t N6 = N5 | (N5 >> 16);
    static const std::size_t N7 = N6 | (N6 >> 32);
    static const std::size_t value = N7;
};

template <std::size_t N>
struct round_to_power2 {
    static const std::size_t max_power2 = jstd::integral_utils<std::size_t>::max_power2;
    static const std::size_t max_num = jstd::integral_utils<std::size_t>::max_num;
    static const std::size_t value = (N < max_num) ? round_up_to_power2_impl<N + 1>::value : max_num;
};

template <>
struct round_to_power2<0> {
    static const size_t value = 0;
};

// struct round_up_to_power2_impl<N>

template <size_t N>
struct round_up_to_power2_impl {
    static const size_t max_num = jstd::integral_utils<size_t>::max_num;
    static const size_t N1 = N - 1;
    static const size_t N2 = N1 | (N1 >> 1);
    static const size_t N3 = N2 | (N2 >> 2);
    static const size_t N4 = N3 | (N3 >> 4);
    static const size_t N5 = N4 | (N4 >> 8);
    static const size_t N6 = N5 | (N5 >> 16);
    static const size_t N7 = N6 | (N6 >> 32);
    static const size_t value = (N7 < max_num) ? (N7 + 1) : max_num;
};

template <size_t N>
struct round_up_to_power2 {
    static const size_t max_power2 = jstd::integral_utils<size_t>::max_power2;
    static const size_t max_num = jstd::integral_utils<size_t>::max_num;
    static const size_t value = (N < max_num) ? round_up_to_power2_impl<N + 1>::value : max_num;
};

template <>
struct round_up_to_power2<0> {
    static const size_t value = 1;
};

#if defined(_MSC_VER)
#pragma warning(pop)
#endif

//////////////////////////////////////////////////////////////////////////////////

template <size_t N, size_t Power2>
struct round_down_to_pow2_impl {
    static const size_t max_power2 = jstd::integral_utils<size_t>::max_power2;
    static const size_t max_num = jstd::integral_utils<size_t>::max_num;
    static const size_t nextPower2 = (Power2 < max_power2) ? (Power2 << 1) : 0;

    static const bool too_large = (N >= max_power2);
    static const bool reach_limit = (nextPower2 == max_power2);

    static const size_t value = (too_large ? max_power2 :
           ((N == Power2) ? N :
            ((reach_limit || nextPower2 > N) ? Power2 :
             round_down_to_pow2_impl<N, nextPower2>::value)));
};

template <size_t N>
struct round_down_to_pow2 {
    static const size_t value = (is_power2<N>::value ? N : round_down_to_pow2_impl<N, 1>::value);
};

template <>
struct round_down_to_pow2<0> {
    static const size_t value = 0;
};

//////////////////////////////////////////////////////////////////////////////////

} // namespace compile_time
} // namespace jstd

#endif // JSTD_SUPPORT_POWEROF2_H
