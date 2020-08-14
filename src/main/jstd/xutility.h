
#ifndef JSTD_XUTILITY_H
#define JSTD_XUTILITY_H

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

#include "jstd/basic/stdint.h"

#include <cstdint>
#include <cstddef>
#include <cstdbool>
#include <memory>

#include <type_traits>

#include "jstd/type_traits.h"

namespace jstd {
namespace detail {

#if !defined(_MSC_VER) || (_MSC_VER >= 1800)

// function template const_cast

// remove const-ness from a fancy pointer
template <typename Ptr>
inline
auto const_cast_(Ptr ptr)
#if !defined(_MSC_VER) || (_MSC_VER < 1900)
                           -> typename std::pointer_traits<
                                typename std::pointer_traits<Ptr>::template rebind<
                                  typename std::remove_const<
                                    typename std::pointer_traits<Ptr>::element_type
                                  >::type
                                >
                              >::pointer
#endif // !_MSC_VER
{
    using Element = typename std::pointer_traits<Ptr>::element_type;
    using Modifiable = typename std::remove_const<Element>::type;
    using Dest = typename std::pointer_traits<Ptr>::template rebind<Modifiable>;

    return (std::pointer_traits<Dest>::pointer_to(const_cast<Modifiable &>(*ptr)));
}

// remove const-ness from a plain pointer
template <typename T>
inline
auto const_cast_(T * ptr) -> const typename std::remove_const<T>::type * {
    return (const_cast<typename std::remove_const<T>::type *>(ptr));
}

#endif // (_MSC_VER >= 1800)

} // namespace detail
} // namespace jstd

#endif // JSTD_XUTILITY_H
