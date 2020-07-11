
#ifndef JSTD_TYPE_TRAITS_H
#define JSTD_TYPE_TRAITS_H

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

#include "jstd/basic/stdint.h"

namespace jstd {

// Template struct param_tester

// test if parameters are valid
template <class ...>
struct param_tester
{
    typedef void type;
};

// Alias template void_t
template <class... Types>
using void_t = typename param_tester<Types...>::type;

} // namespace jstd

#endif // JSTD_TYPE_TRAITS_H
