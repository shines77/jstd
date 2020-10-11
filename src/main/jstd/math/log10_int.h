
#ifndef JSTD_MATH_LOG10_INT_H
#define JSTD_MATH_LOG10_INT_H

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

#include "jstd/basic/stddef.h"
#include "jstd/basic/stdint.h"
#include "jstd/basic/stdsize.h"

#include <memory.h>
#include <string.h>
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

#include "jstd/support/bitscan_reverse.h"

#if defined(_MSC_VER) || (defined(WIN32) && (defined(__INTEL_COMPILER) || defined(__ICL)))
#include <intrin.h>     // For _BitScanReverse(), _BitScanReverse64()

#pragma intrinsic(_BitScanReverse)

#if defined(WIN64) || defined(_WIN64) || defined(_M_X64) || defined(_M_AMD64) \
 || defined(_M_IA64) || defined(__amd64__) || defined(__x86_64__)
#pragma intrinsic(_BitScanReverse64)
#endif // __amd64__

#endif // _MSC_VER

namespace jstd {

static const uint32_t s_power10[16] = {
    9UL,            // 1
    99UL,           // 2
    999UL,          // 3
    9999UL,         // 4
    99999UL,        // 5
    999999UL,       // 6
    9999999UL,      // 7
    99999999UL,     // 8
    999999999UL,    // 9
    4294967295UL,   // 10

    // only fill for aligned to 64 bytes
    0, 0, 0, 0, 0, 0
};

static const uint64_t s_power10_64[24] = {
    9ULL,                    // 1
    99ULL,                   // 2
    999ULL,                  // 3
    9999ULL,                 // 4
    99999ULL,                // 5
    999999ULL,               // 6
    9999999ULL,              // 7
    99999999ULL,             // 8
    999999999ULL,            // 9
    9999999999ULL,           // 10
    99999999999ULL,          // 11
    999999999999ULL,         // 12
    9999999999999ULL,        // 13
    99999999999999ULL,       // 14
    999999999999999ULL,      // 15
    9999999999999999ULL,     // 16
    99999999999999999ULL,    // 17
    999999999999999999ULL,   // 18
    9999999999999999999ULL,  // 19
    18446744073709551615ULL, // 20

    // only fill for aligned to 64 bytes
    0, 0, 0, 0
};

uint32_t jm_log10_u32(uint32_t val)
{
    uint32_t exponent;
    uint32_t exp10;

    if (val >= 10) {
#if defined(_MSC_VER) || (defined(WIN32) && (defined(__INTEL_COMPILER) || defined(__ICL)))
        //
        // _BitScanReverse, _BitScanReverse64
        // Reference: http://msdn.microsoft.com/en-us/library/fbxyd7zd.aspx
        //
        _BitScanReverse((unsigned long *)&exponent, (unsigned long)val);
#else
#if defined(__has_builtin_clz) || defined(__linux__)
        // countLeadingZeros()
        int leading_zeros = __builtin_clz((unsigned int)val);
#else
        int leading_zeros = __internal_clz((unsigned int)val);
#endif
        exponent = (uint32_t)(leading_zeros ^ 31);
#endif

        // must 2,525,222 < 4,194,304 ( 2^32 / 1024)
        // exp10 = exponent * 2525222UL;
        exp10 = exponent * 2525222UL;
        // exp10 = exp10 / 131072 / 64;
        exp10 = exp10 >> 23;

        if (val > s_power10[exp10]) {
            exp10++;
        }

        return exp10;
    }
    else {
        return 0;
    }
}

#if defined(WIN64) || defined(_WIN64) || defined(_M_X64) || defined(_M_AMD64) \
 || defined(_M_IA64) || defined(__amd64__) || defined(__x86_64__)

uint32_t jm_log10_u64(uint64_t val)
{
    uint32_t exponent;
    uint32_t exp10;

    if (val >= 10) {
#if defined(_MSC_VER) || (defined(WIN32) && (defined(__INTEL_COMPILER) || defined(__ICL)))
        //
        // _BitScanReverse, _BitScanReverse64
        // Reference: http://msdn.microsoft.com/en-us/library/fbxyd7zd.aspx
        //
        _BitScanReverse64((unsigned long *)&exponent, (unsigned __int64)val);
#else
#if defined(__has_builtin_clzll) || defined(__linux__)
        // countLeadingZeros64()
        int leading_zeros = __builtin_clzll((unsigned long long)val);
#else
        int leading_zeros = __internal_clzll((uint64_t)val);
#endif
        exponent = (uint32_t)(leading_zeros ^ 63);
#endif

        // must 2,525,222 < 4,194,304 ( 2^32 / 1024)
        // exp10 = exponent * 2525222UL;
        exp10 = exponent * 2525222UL;
        // exp10 = exp10 / 131072 / 64;
        exp10 = exp10 >> 23;

        if (val > s_power10_64[exp10]) {
            exp10++;
        }

        return exp10;
    }
    else {
        return 0;
    }
}

#endif // __amd64__

} // namespace jstd

#endif // JSTD_MATH_LOG10_INT_H
