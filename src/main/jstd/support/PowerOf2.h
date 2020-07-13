
#ifndef JSTD_SUPPORT_POWEROF2_H
#define JSTD_SUPPORT_POWEROF2_H

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

#include "jstd/basic/stddef.h"
#include "jstd/basic/stdint.h"
#include "jstd/basic/stdsize.h"

#include <stddef.h>
#include <assert.h>

#include <cassert>
#include <type_traits>

namespace jstd {

namespace run_time {

template <typename SizeType>
inline bool is_pow2(SizeType n) {
    static_assert(std::is_integral<SizeType>::value,
        "Error: is_pow2(SizeType n) -- SizeType must be a integral type.");
    typedef typename std::make_unsigned<SizeType>::type UnsignedSizeType;
    UnsignedSizeType x = n;
    return ((x & (x - 1)) == 0);
}

template <typename SizeType>
inline SizeType verify_pow2(SizeType n) {
    static_assert(std::is_integral<SizeType>::value,
        "Error: verify_pow2(SizeType n) -- SizeType must be a integral type.");
    typedef typename std::make_unsigned<SizeType>::type UnsignedSizeType;
    UnsignedSizeType x = n;
    return (x & (x - 1));
}

template <typename SizeType>
inline SizeType round_up_to_pow2(SizeType n) {
    static_assert(std::is_integral<SizeType>::value,
        "Error: round_up_to_pow2(SizeType n) -- SizeType must be a integral type.");
    typedef typename std::make_unsigned<SizeType>::type UnsignedSizeType;
    UnsignedSizeType u = static_cast<UnsignedSizeType>(n);
    if (is_pow2(u)) {
        if (u == jstd::integral_utils<UnsignedSizeType>::max_power2)
            return static_cast<SizeType>(jstd::integral_utils<UnsignedSizeType>::max_num);
        else if (u == 0)
            return static_cast<SizeType>(1);
        else
            return static_cast<SizeType>(u * 2);
    }
    else {
        u = static_cast<UnsignedSizeType>(u - 1);
        u = u | (u >> 1);
        u = u | (u >> 2);
        u = u | (u >> 4);
        u = u | (u >> 8);
        u = u | (u >> 16);
        if (jstd::integral_utils<UnsignedSizeType>::bits == 64) {
            u = u | (u >> 32);
        }
        return (u != jstd::integral_utils<UnsignedSizeType>::max_num) ?
            static_cast<SizeType>(u + 1) : static_cast<SizeType>(u);
    }
}

} // namespace run_time

//////////////////////////////////////////////////////////////////////////////////

namespace compile_time {

//
// is_pow_of_2 = (N && ((N & (N - 1)) == 0);  // here, N must is unsigned number
//
template <size_t N>
struct is_power2 {
    static const bool value = ((N & (N - 1)) == 0);
};

template <>
struct is_power2<0> {
    static const bool value = true;
};

//////////////////////////////////////////////////////////////////////////////////

// Maxinum recursive depth is about 497 layer.

template <size_t N, size_t Power2>
struct round_up_to_pow2_impl {
    static const size_t max_power2 = jstd::integral_utils<size_t>::max_power2;
    static const size_t max_num = jstd::integral_utils<size_t>::max_num;
    static const size_t nextPower2 = (Power2 < max_power2) ? (Power2 << 1) : 0;

    static const bool too_large = (N >= max_power2);
    static const bool reach_limit = (Power2 == max_power2);

    static const size_t value = ((N >= max_power2) ? max_num :
           (((Power2 == max_power2) || (Power2 > N)) ? Power2 :
            round_up_to_pow2_impl<N, nextPower2>::value));

    //static const size_t value = ((N >= max_power2) ? max_num :
    //    ((Power2 > N) ? Power2 : round_up_to_pow2_impl<N, nextPower2>::value));
};

template <size_t N>
struct round_up_to_pow2_impl<N, 0> {
    static const size_t value = jstd::integral_utils<size_t>::max_num;
};

template <size_t N>
struct round_up_to_pow2 {
    static const size_t value = round_up_to_pow2_impl<N, 1>::value;
};

template <>
struct round_up_to_pow2<0> {
    static const size_t value = 1;
};

//////////////////////////////////////////////////////////////////////////////////

#if defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable: 4307)
#endif

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
