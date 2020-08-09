
#ifndef JSTD_INTERNAL_COPYABLE_H
#define JSTD_INTERNAL_COPYABLE_H

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

#include "jstd/basic/stddef.h"
#include "jstd/config/config.h"

namespace jstd {
namespace internal {

class Copyable
{
public:
#if JSTD_HAS_DEFAULTED_FUNCTIONS
    //! Allow default construction
    JSTD_CONSTEXPR Copyable() = default;
    ~Copyable() = default;
#else
    //! Allow default construction
    Copyable() {}
    ~Copyable() {}
#endif

    // Allow the copy constructor
    Copyable(const Copyable &);
    // Allow the assignment operator
    Copyable & operator = (const Copyable &);
};

} // namespace internal
} // namespace jstd

#endif // JSTD_INTERNAL_COPYABLE_H
