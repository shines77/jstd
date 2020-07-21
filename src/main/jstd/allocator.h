
#ifndef JSTD_ALLOCATOR_H
#define JSTD_ALLOCATOR_H

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

#include "jstd/basic/stdint.h"
#include "jstd/basic/stdsize.h"

#include <assert.h>

#include <cstdint>
#include <cstddef>
#include <memory>           // For std::pointer_traits<T>

namespace jstd {

template <class T>
struct allocator_base {
    typedef T *         pointer;
    typedef const T *   const_pointer;
    typedef T &         reference;
    typedef const T &   const_reference;

    typedef std::size_t size_type;
};

template <class T>
struct dummy_allocator : public allocator_base<T> {
    typedef allocator_base<T>                   base_type;
    typedef dummy_allocator<T>                  this_type;

    typedef typename base_type::pointer         pointer;
    typedef typename base_type::const_pointer   const_pointer;
    typedef typename base_type::reference       reference;
    typedef typename base_type::reference       const_reference;
    typedef typename base_type::size_type       size_type;

    dummy_allocator() {}
    ~dummy_allocator() {}

    pointer allocate(size_type size) {
        return nullptr;
    }

    pointer reallocate(pointer ptr, size_type new_size) {
        return nullptr;
    }

    pointer reallocate(void * ptr, size_type new_size) {
        return nullptr;
    }

    void deallocate(pointer ptr) {
    }

    void deallocate(void * ptr) {
    }

    bool isAutoRelease() { return false; }
};

template <class T>
struct allocator : public allocator_base<T> {
    typedef allocator_base<T>                   base_type;
    typedef allocator<T>                        this_type;

    typedef typename base_type::pointer         pointer;
    typedef typename base_type::const_pointer   const_pointer;
    typedef typename base_type::reference       reference;
    typedef typename base_type::reference       const_reference;
    typedef typename base_type::size_type       size_type;

    allocator() {}
    ~allocator() {}

    pointer allocate(size_type size) {
        return new T[size];
    }

    pointer reallocate(pointer ptr, size_type new_size) {
        return new T[new_size];
    }

    pointer reallocate(void * ptr, size_type new_size) {
        return new T[new_size];
    }

    void deallocate(pointer ptr) {
        assert(ptr != nullptr);
        delete[] ptr;
    }

    void deallocate(void * ptr) {
        assert(ptr != nullptr);
        delete[] (pointer)ptr;
    }

    bool isAutoRelease() { return true; }
};

template <typename T>
struct malloc_allocator : public allocator_base<T> {
    typedef allocator_base<T>                   base_type;
    typedef malloc_allocator<T>                 this_type;

    typedef typename base_type::pointer         pointer;
    typedef typename base_type::const_pointer   const_pointer;
    typedef typename base_type::reference       reference;
    typedef typename base_type::reference       const_reference;
    typedef typename base_type::size_type       size_type;

    malloc_allocator() {}
    ~malloc_allocator() {}

    pointer allocate(size_type size) {
        return std::malloc(size);
    }

    pointer reallocate(pointer ptr, size_type new_size) {
        return static_cast<pointer>(std::realloc((void *)ptr, new_size));
    }

    pointer reallocate(void * ptr, size_type new_size) {
        return static_cast<pointer>(std::realloc(ptr, new_size));
    }

    void deallocate(pointer ptr) {
        assert(ptr != nullptr);
        std::free(ptr);
    }

    void deallocate(void * ptr) {
        assert(ptr != nullptr);
        std::free(ptr);
    }

    bool isAutoRelease() { return true; }
};

} // namespace jstd

#endif // JSTD_ALLOCATOR_H
