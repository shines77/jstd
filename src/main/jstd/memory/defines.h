
#ifndef JSTD_MEMORY_DEFINE_H
#define JSTD_MEMORY_DEFINE_H

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

#include "jstd/basic/stddef.h"

#ifndef MUST_BE_A_DERIVED_CLASS_OF
#define MUST_BE_A_DERIVED_CLASS_OF(Base, Derived) \
        static_assert(!std::is_base_of<Base, Derived>::value, \
            "Error: [" JSTD_TO_STRING(Base) "] must be a derived class of [" JSTD_TO_STRING(Derived) "].")
#endif

namespace jstd {

///////////////////////////////////////////////////
// struct delete_helper<T, IsArray>
///////////////////////////////////////////////////

template <typename T, bool IsArray>
struct delete_helper {};

template <typename T>
struct delete_helper<T, false> {
    static void delete_it(T * p) {
        delete p;
    }
};

template <typename T>
struct delete_helper<T, true> {
    static void delete_it(T * p) {
        delete[] p;
    }
};

} // namespace jstd

#endif // JSTD_MEMORY_DEFINE_H
