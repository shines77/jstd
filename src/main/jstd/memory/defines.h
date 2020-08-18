
#ifndef JSTD_MEMORY_DEFINE_H
#define JSTD_MEMORY_DEFINE_H

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

#include "jstd/basic/stddef.h"

#include <memory>

#ifndef MUST_BE_A_DERIVED_CLASS_OF
#if 0
#define MUST_BE_A_DERIVED_CLASS_OF(Base, Derived)
#else
#define MUST_BE_A_DERIVED_CLASS_OF(Base, Derived) \
        static_assert(!std::is_base_of<Base, Derived>::value, \
            "Error: [" JSTD_TO_STRING(Base) "] must be a derived class of [" JSTD_TO_STRING(Derived) "].")
#endif
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


template <typename T>
void swap(T & a, T & b, typename std::enable_if<!std::is_pointer<T>::value>::type * p = nullptr) {
    T tmp = std::move(a);
    a = std::move(b);
    b = std::move(tmp);
}

template <typename T>
void swap(T* & a, T* & b, typename std::enable_if<std::is_pointer<T>::value>::type * p = nullptr) {
    T* tmp = a;
    a = b;
    b = tmp;
}

} // namespace jstd

#endif // JSTD_MEMORY_DEFINE_H
