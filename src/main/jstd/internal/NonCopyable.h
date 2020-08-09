
#ifndef JSTD_INTERNAL_NONCOPYABLE_H
#define JSTD_INTERNAL_NONCOPYABLE_H

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

#include "jstd/basic/stddef.h"
#include "jstd/config/config.h"

#include "jstd/internal/NonAssignable.h"

//
// C++ Idioms - Non Copyable Objects
// http://dev-faqs.blogspot.com/2010/07/c-idioms-non-copyable.html
//

namespace jstd {
namespace internal {

class NonCopyable
{
protected:
#if JSTD_HAS_DEFAULTED_FUNCTIONS
    //! Allow default construction
    JSTD_CONSTEXPR NonCopyable() = default;
    ~NonCopyable() = default;
#else
    //! Allow default construction
    NonCopyable() {}
    ~NonCopyable() {}
#endif

#if JSTD_HAS_DELETED_FUNCTIONS
    //! Deny copy construction
    NonCopyable(const NonCopyable &) = delete;
    //! Deny assignment operator
    NonCopyable & operator = (const NonCopyable &) = delete;
#else
private:
    //! Deny copy construction
    NonCopyable(const NonCopyable &);
    //! Deny assignment operator
    NonCopyable & operator = (const NonCopyable &);
#endif
};

} // namespace internal
} // namespace jstd

#endif // JSTD_INTERNAL_NONCOPYABLE_H
