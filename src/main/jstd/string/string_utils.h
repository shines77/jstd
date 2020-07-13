
#ifndef JSTD_STRING_UTILS_H
#define JSTD_STRING_UTILS_H

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

#include "jstd/basic/stddef.h"
#include "jstd/basic/stdint.h"

#include <string.h>
#include <nmmintrin.h>  // For SSE 4.2

#include "jstd/string/char_traits.h"
#include "jstd/support/SSEHelper.h"

namespace jstd {
namespace StrUtils {

enum CompareResult {
    IsSmaller = -1,
    IsEqual = 0,
    IsBigger = 1
};

#if 0

template <typename CharTy>
static inline
bool is_equals_unsafe(const CharTy * str1, const CharTy * str2, size_t length)
{
    assert(str1 != nullptr && str2 != nullptr);

    static const int kMaxSize = jstd::SSEHelper<CharTy>::kMaxSize;
    static const int _SIDD_CHAR_OPS = jstd::SSEHelper<CharTy>::_SIDD_CHAR_OPS;
    static const int kEqualEach = _SIDD_CHAR_OPS | _SIDD_CMP_EQUAL_EACH
                                | _SIDD_NEGATIVE_POLARITY | _SIDD_LEAST_SIGNIFICANT;

    if (likely(length > 0)) {
        ssize_t nlength = (ssize_t)length;
        do {
            __m128i __str1 = _mm_loadu_si128((const __m128i *)str1);
            __m128i __str2 = _mm_loadu_si128((const __m128i *)str2);
            int len = ((int)nlength >= kMaxSize) ? kMaxSize : (int)nlength;
            assert(len > 0);

            int full_matched = _mm_cmpestrc(__str1, len, __str2, len, kEqualEach);
            if (likely(full_matched == 0)) {
                // Full matched, continue match next kMaxSize bytes.
                str1 += kMaxSize;
                str2 += kMaxSize;
                nlength -= kMaxSize;
            }
            else {
                // It's dismatched.
                return false;
            }
        } while (nlength > 0);
    }

    // It's matched, or the length is equal 0.
    return true;
}

#elif (STRING_COMPARE_MODE == STRING_COMPARE_SSE42)

template <typename CharTy>
static inline
bool is_equals_unsafe(const CharTy * str1, const CharTy * str2, size_t length)
{
    assert(str1 != nullptr && str2 != nullptr);

    static const int kMaxSize = jstd::SSEHelper<CharTy>::kMaxSize;
    static const int _SIDD_CHAR_OPS = jstd::SSEHelper<CharTy>::_SIDD_CHAR_OPS;
    static const int kEqualEach = _SIDD_CHAR_OPS | _SIDD_CMP_EQUAL_EACH
                                | _SIDD_NEGATIVE_POLARITY | _SIDD_LEAST_SIGNIFICANT;
    
    ssize_t nlength = (ssize_t)length;
    while (likely(nlength > 0)) {
        __m128i __str1 = _mm_loadu_si128((const __m128i *)str1);
        __m128i __str2 = _mm_loadu_si128((const __m128i *)str2);
        int len = ((int)nlength >= kMaxSize) ? kMaxSize : (int)nlength;
        assert(len > 0);

        int full_matched = _mm_cmpestrc(__str1, len, __str2, len, kEqualEach);
        str1 += kMaxSize;
        str2 += kMaxSize;
        nlength -= kMaxSize;
        if (likely(full_matched == 0)) {
            // Full matched, continue match next kMaxSize bytes.
            continue;
        }
        else {
            // It's dismatched.
            return false;
        }
    }

    // It's matched, or the length is equal 0.
    return true;
}

#elif (STRING_COMPARE_MODE == STRING_COMPARE_U64)

template <typename CharTy>
static inline
bool is_equals_unsafe(const CharTy * str1, const CharTy * str2, size_t length)
{
    assert(str1 != nullptr && str2 != nullptr);

    static const size_t kMaxSize = sizeof(uint64_t);
    static const uint64_t kMaskOne64 = 0xFFFFFFFFFFFFFFFFULL;

    uint64_t * __str1 = (uint64_t * )str1;
    uint64_t * __str2 = (uint64_t * )str2;

    while (likely(length > 0)) {
        assert(__str1 != nullptr && __str2 != nullptr);
        if (likely(length >= kMaxSize)) {
            // Compare 8 bytes each time.
            uint64_t val64_1 = *__str1;
            uint64_t val64_2 = *__str2;
            if (likely(val64_1 == val64_2)) {
                // Full matched, continue match next kMaxSize bytes.
                __str1++;
                __str2++;
                length -= kMaxSize;
                continue;
            }
            else {
                // It's dismatched.
                return false;
            }
        }
        else {
            // Get the mask64
            assert(length > 0 && length < kMaxSize);
            uint32_t rest = (uint32_t)(kMaxSize - length);
            assert(rest > 0 && rest < (uint32_t)kMaxSize);
            uint64_t mask64 = kMaskOne64 >> (rest * 8U);

            // Compare the remain bytes.
            uint64_t val64_1 = *__str1;
            uint64_t val64_2 = *__str2;
            val64_1 &= mask64;
            val64_2 &= mask64;
            if (likely(val64_1 == val64_2)) {
                // Full matched, return true.
                return true;
            }
            else {
                // It's dismatched.
                return false;
            }
        }
    }

    // It's matched, or the length is equal 0.
    return true;
}

#endif // STRING_COMPARE_MODE

template <typename CharTy>
static inline
bool is_equals_fast(const CharTy * str1, const CharTy * str2, size_t length)
{
#if (STRING_COMPARE_MODE == STRING_COMPARE_STDC)
    return (::strcmp((const char *)str1, (const char *)str2) == 0);
#else
    if (likely(((uintptr_t)str1 & (uintptr_t)str2) != 0)) {
        assert(str1 != nullptr && str2 != nullptr);
        return StrUtils::is_equals_unsafe(str1, str2, length);
    }
    else if (likely(((uintptr_t)str1 | (uintptr_t)str2) != 0)) {
        assert((str1 == nullptr && str2 != nullptr) ||
               (str1 != nullptr && str2 == nullptr));
        return (length == 0);
    }
    else {
        assert(str1 == nullptr && str2 == nullptr);
        return true;
    }
#endif // STRING_COMPARE_MODE
}

template <typename CharTy>
static inline
bool is_equals(const CharTy * str1, size_t length1, const CharTy * str2, size_t length2)
{
    if (likely(length1 == length2)) {
        return StrUtils::is_equals_fast(str1, str2, length1);
    }

    // The length of between str1 and str2 is different.
    return false;
}

template <typename StringType>
static inline
bool is_equals_fast(const StringType & str1, const StringType & str2)
{
    assert(str1.size() == str2.size());
#if (STRING_COMPARE_MODE == STRING_COMPARE_STDC)
    return (::strcmp((const char *)str1.c_str(), (const char *)str2.c_str()) == 0);
#else
    return StrUtils::is_equals_fast(str1.c_str(), str2.c_str(), str1.size());   
#endif
}

template <typename StringType>
static inline
bool is_equals(const StringType & str1, const StringType & str2)
{
#if (STRING_COMPARE_MODE == STRING_COMPARE_STDC)
    return (::strcmp((const char *)str1.c_str(), (const char *)str2.c_str()) == 0);
#else
    return StrUtils::is_equals(str1.c_str(), str1.size(), str2.c_str(), str2.size());
#endif
}

template <typename CharTy>
static inline
int compare_unsafe(const CharTy * str1, size_t length1, const CharTy * str2, size_t length2)
{
    assert(str1 != nullptr && str2 != nullptr);

    static const int kMaxSize = jstd::SSEHelper<CharTy>::kMaxSize;
    static const int _SIDD_CHAR_OPS = jstd::SSEHelper<CharTy>::_SIDD_CHAR_OPS;
    static const int kEqualEach = _SIDD_CHAR_OPS | _SIDD_CMP_EQUAL_EACH
                                | _SIDD_NEGATIVE_POLARITY | _SIDD_LEAST_SIGNIFICANT;

    typedef typename jstd::uchar_traits<CharTy>::type UCharTy;

    size_t length = (length1 <= length2) ? length1 : length2;
    ssize_t nlength = (ssize_t)length;

    while (likely(nlength > 0)) {
        __m128i __str1 = _mm_loadu_si128((const __m128i *)str1);
        __m128i __str2 = _mm_loadu_si128((const __m128i *)str2);
        int len = ((int)nlength >= kMaxSize) ? kMaxSize : (int)nlength;
        assert(len > 0);

        int full_matched = _mm_cmpestrc(__str1, len, __str2, len, kEqualEach);
        int matched_index = _mm_cmpestri(__str1, len, __str2, len, kEqualEach);
        str1 += kMaxSize;
        str2 += kMaxSize;
        nlength -= kMaxSize;
        if (likely(full_matched == 0)) {
            // Full matched, continue match next kMaxSize bytes.
            assert(matched_index >= kMaxSize);
            continue;
        }
        else {
            // It's dismatched.
            assert(matched_index >= 0 && matched_index < kMaxSize);
            int offset = (kMaxSize - matched_index);
            UCharTy ch1 = *((const UCharTy *)str1 - offset);
            UCharTy ch2 = *((const UCharTy *)str2 - offset);
            if (likely(ch1 > ch2))
                return StrUtils::IsBigger;
            else if (likely(ch1 < ch2))
                return StrUtils::IsSmaller;
            else
                return StrUtils::IsEqual;
        }
    }

    // It's matched, or the length is equal 0.
    UCharTy ch1 = *((const UCharTy *)str1);
    UCharTy ch2 = *((const UCharTy *)str2);
    if (likely(ch1 > ch2))
        return StrUtils::IsBigger;
    else if (likely(ch1 < ch2))
        return StrUtils::IsSmaller;
    else
        return StrUtils::IsEqual;
}

template <typename CharTy>
static inline
int compare(const CharTy * str1, size_t length1, const CharTy * str2, size_t length2)
{
#if (STRING_COMPARE_MODE == STRING_COMPARE_STDC)
    return ::strcmp((const char *)str1, (const char *)str2);
#else
    if (likely(((uintptr_t)str1 & (uintptr_t)str2) != 0)) {
        assert(str1 != nullptr && str2 != nullptr);
        return StrUtils::compare_unsafe(str1, length1, str2, length2);
    }
    else if (likely(((uintptr_t)str1 | (uintptr_t)str2) != 0)) {
        assert((str1 == nullptr && str2 != nullptr) ||
               (str1 != nullptr && str2 == nullptr));
        if (likely(str1 != nullptr))
            return ((length1 != 0) ? StrUtils::IsBigger : StrUtils::IsEqual);
        else
            return ((length2 != 0) ? StrUtils::IsSmaller : StrUtils::IsEqual);
    }
    else {
        assert(str1 == nullptr && str2 == nullptr);
        return StrUtils::IsEqual;
    }
#endif // STRING_COMPARE_MODE
}

template <typename StringType>
static inline
int compare(const StringType & str1, const StringType & str2)
{
#if (STRING_COMPARE_MODE == STRING_COMPARE_STDC)
    return ::strcmp((const char *)str1.c_str(), (const char *)str2.c_str());
#else
    return StrUtils::compare(str1.c_str(), str1.size(), str2.c_str(), str2.size());
#endif
}

} // namespace StrUtils
} // namespace jstd

#endif // JSTD_STRING_UTILS_H
