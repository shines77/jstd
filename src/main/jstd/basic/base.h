
#ifndef JSTD_BASIC_BASE_H
#define JSTD_BASIC_BASE_H

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

#include "jstd/basic/stddef.h"
#include "jstd/basic/stdint.h"
#include "jstd/basic/stdsize.h"

#include <cstdint>      // For std::size_t
#include <cstddef>      // For std::max_align_t
#include <type_traits>  // For std::alignment_of<T>

namespace jstd {

template <typename T>
struct align_of {
#if 0
    static constexpr std::size_t value = (alignof(T) >= alignof(std::max_align_t))
                                        ? alignof(T) :  alignof(std::max_align_t);
#else
    static constexpr std::size_t value = std::alignment_of<T>::value;
#endif
};

} // namespace jstd

#endif // JSTD_BASIC_BASE_H
