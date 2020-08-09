
#ifndef JSTD_INTERNAL_NONASSIGNABLE_H
#define JSTD_INTERNAL_NONASSIGNABLE_H

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

#include "jstd/basic/stddef.h"
#include "jstd/config/config.h"

namespace jstd {
namespace internal {

class NonAssignable
{
public:
#ifdef __GNUC__
    //! Explicitly define default construction, because otherwise gcc issues gratuitous warning.
    NonAssignable() {}
#endif  /* __GNUC__ */

#if JSTD_HAS_DELETED_FUNCTIONS
    //! Deny assignment operator
    NonCopyable & operator = (const NonCopyable &) = delete;
#else
private:
    //! Deny assignment operator
    NonAssignable & operator = (const NonAssignable &);
#endif
};

} // namespace internal
} // namespace jstd

#endif // JSTD_INTERNAL_NONASSIGNABLE_H
