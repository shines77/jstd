
#ifndef JSTD_ALLOCATOR_H
#define JSTD_ALLOCATOR_H

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

#include "jstd/basic/stddef.h"
#include "jstd/basic/stdint.h"
#include "jstd/basic/stdsize.h"

#include <stddef.h>
#include <malloc.h>
#include <memory.h>
#include <assert.h>

#include <cstdint>
#include <cstddef>
#include <memory>   // For std::pointer_traits<T>
#include <limits>   // For std::numeric_limits<T>

#define JSTD_MINIMUM_ALIGNMENT   4
#define JSTD_DEFAULT_ALIGNMENT   alignof(std::max_align_t)

namespace jstd {

// TODO: _Allocate()

static inline
void * _Allocate(std::size_t size) {
    assert(size != 0);
    void * ptr = std::malloc(size);
    return ptr;
}

// TODO: _Reallocate()

static inline
void * _Reallocate(void * ptr, std::size_t size) {
    assert(size != 0);
    void * new_ptr = std::realloc(ptr, size);
    return new_ptr;
}

// TODO: _Deallocate()

static inline
void _Deallocate(void * p, std::size_t size = 0) {
    std::free(p);
}

// TODO: _AlignedAllocate()

static inline
void * _AlignedAllocate(std::size_t size, std::size_t alignment = JSTD_DEFAULT_ALIGNMENT) {
    assert(size != 0);
#ifdef _WIN32
    void * ptr = ::_aligned_malloc(size, alignment);
#else
    void * ptr = ::memalign(alignment, size);
#endif
    return ptr;
}

// TODO: _AlignedReallocate()

static inline
void * _AlignedReallocate(void * ptr, std::size_t size, std::size_t alignment = JSTD_DEFAULT_ALIGNMENT) {
    assert(size != 0);
#ifdef _WIN32
    void * new_ptr = ::_aligned_realloc(ptr, size, alignment);
#else
    // On Unix/Linux, it's have no posix_memalign_realloc() function.
    // See: https://stackoverflow.com/questions/9078259/does-realloc-keep-the-memory-alignment-of-posix-memalign
    // See: https://www.jb51.cc/c/115220.html
    void * new_ptr = ::memalign(alignment, size);
#endif
    return new_ptr;
}

// TODO: _AlignedDeallocate()

static inline
void _AlignedDeallocate(void * p, std::size_t size = 0) {
#ifdef _WIN32
    ::_aligned_free(p);
#else
    ::free(p);
#endif
}

template <typename T>
struct align_of {
    static const std::size_t value = alignof(T);
};

template <class Derive, class T, std::size_t Alignment = align_of<T>::value>
struct allocator_base {
    typedef Derive          derive_type;
    typedef T               value_type;
    typedef T *             pointer;
    typedef const T *       const_pointer;
    typedef T &             reference;
    typedef const T &       const_reference;

    typedef std::ptrdiff_t  difference_type;
    typedef std::size_t     size_type;

    typedef true_type       propagate_on_container_move_assignment;
    typedef true_type       is_always_equal;

    static const size_type kAlignOf = Alignment;
    static const size_type kAlignment = compile_time::round_up_to_pow2<Alignment>::value;

    size_type align_of() const { return kAlignOf; }
    size_type alignment() const { return kAlignment; }

    size_type max_size() const {
        // Estimate maximum array size
        return (std::numeric_limits<std::size_t>::(max)() / sizeof(T));
    }

    pointer address(reference value) const noexcept {
        return std::addressof(value);
    }

    const_pointer address(const_reference value) const noexcept {
        return std::addressof(value);
    }

    pointer create_new() {
        return this->create_new_array(1);
    }

    template <typename ...Args>
    pointer create_new(Args && ... args) {
        return this->create_new_array(1, std::forward<Args>(args)...);
    }

    pointer create_new_array(size_type count) {
        derive_type * pThis = static<derive_type *>(this);
        pointer ptr = pThis->allocate(count);
        pointer cur = ptr;
        for (size_type i = count; i != 0; i--) {
            pThis->constructor(cur);
            cur++;
        }
        return ptr;
    }

    template <typename ...Args>
    pointer create_new_array(size_type count, Args && ... args) {
        derive_type * pThis = static<derive_type *>(this);
        pointer ptr = pThis->allocate(count);
        pointer cur = ptr;
        for (size_type i = count; i != 0; i--) {
            pThis->constructor(cur, std::forward<Args>(args)...);
            cur++;
        }
        return ptr;
    }

    template <typename U>
    pointer create_new_at(U * ptr, size_type count) {
        return this->create_new_array_at(U, 1);
    }

    template <typename U, typename ...Args>
    pointer create_new_at(U * ptr, size_type count) {
        return this->create_new_array_at(U, 1, std::forward<Args>(args)...);
    }

    template <typename U>
    pointer create_new_array_at(U * ptr, size_type count) {
        derive_type * pThis = static<derive_type *>(this);
        pointer new_ptr = pThis->reallocate(ptr, count);
        pointer cur = new_ptr;
        for (size_type i = count; i != 0; i--) {
            pThis->constructor(cur);
            cur++;
        }
        return new_ptr;
    }

    template <typename U, typename ...Args>
    pointer create_new_array_at(U * ptr, size_type count, Args && ... args) {
        derive_type * pThis = static<derive_type *>(this);
        pointer new_ptr = pThis->reallocate(ptr, count);
        pointer cur = new_ptr;
        for (size_type i = count; i != 0; i--) {
            pThis->constructor(cur, std::forward<Args>(args)...);
            cur++;
        }
        return new_ptr;
    }

    template <typename U>
    void destroy(U * ptr) {
        this->destroy(ptr, 1);
    }

    template <typename U>
    void destroy_array(U * ptr, size_type count) {
        derive_type * pThis = static<derive_type *>(this);
        assert(ptr != nullptr);
        U * cur = ptr;
        for (size_type i = count; i != 0; i--) {
            pThis->destructor(cur);
            cur++;
        }
        pThis->deallocate(ptr, count);
    }

    // placement new
    pointer constructor(pointer ptr) {
        assert(ptr != nullptr);
        void * v_ptr = static_cast<void *>(ptr);
        return ::new (v_ptr) value_type();
    }

    // placement new
    pointer constructor(void * ptr) {
        return this->constructor(static_cast<pointer>(ptr));
    }

    // placement new (Args...)
    template <typename ...Args>
    pointer constructor(pointer ptr, Args && ... args) {
        assert(ptr != nullptr);
        void * v_ptr = static_cast<void *>(ptr);
        return ::new (v_ptr) value_type(std::forward<Args>(args)...);
    }

    // placement new (Args...)
    template <typename ...Args>
    pointer constructor(void * ptr, Args && ... args) {
        return this->constructor(static_cast<pointer>(ptr), std::forward<Args>(args)...);
    }

    template <typename U>
    void destructor(U * ptr) {
        assert(ptr != nullptr);
        ptr->~U();
    }

    void destructor(void * ptr) {
        assert(ptr != nullptr);
        static_cast<pointer>(ptr)->~T();
    }

    pointer reallocate(void * ptr, size_type count) {
        derive_type * pThis = static<derive_type *>(this);
        return pThis->reallocate(static_cast<pointer>(ptr), count);
    }

    void deallocate(void * ptr, size_type count) {
        derive_type * pThis = static<derive_type *>(this);
        pThis->deallocate(static_cast<pointer>(ptr), count);
    }
};

template <class T, std::size_t Alignment = align_of<T>::value>
struct allocator : public allocator_base<
            allocator<T, Alignment>, T, Alignment> {
    typedef allocator<T, Alignment>                 this_type;
    typedef allocator_base<this_type, T, Alignment> base_type;

    typedef typename base_type::value_type          value_type;
    typedef typename base_type::pointer             pointer;
    typedef typename base_type::const_pointer       const_pointer;
    typedef typename base_type::reference           reference;
    typedef typename base_type::reference           const_reference;

    typedef typename base_type::difference_type     difference_type;
    typedef typename base_type::size_type           size_type;

    static const size_type kAlignOf = base_type::kAlignOf;
    static const size_type kAlignment = base_type::kAlignment;

    allocator() noexcept {}
    allocator(const this_type & other) noexcept {}
    template <typename U>
    allocator(const allocator<U, Alignment> & other) noexcept {}

    this_type & operator = (const this_type & other) noexcept {
        return *this;
    }
    template <typename U>
    this_type & operator = (const allocator<U, Alignment> & other) noexcept {
        return *this;
    }

    ~allocator() {}

    pointer allocate(size_type count) {
        pointer ptr;
        if (kAlignment != 0)
            ptr = static_cast<pointer>(_AlignedAllocate(count * sizeof(value_type), kAlignment));
        else
            ptr = static_cast<pointer>(_Allocate(count * sizeof(value_type)));
        return ptr;
    }

    template <typename U>
    pointer reallocate(U * ptr, size_type count) {
        pointer new_ptr;
        if (kAlignment != 0)
            new_ptr = static_cast<pointer>(_AlignedReallocate((void *)ptr, count * sizeof(value_type), kAlignment));
        else
            new_ptr = static_cast<pointer>(_Reallocate((void *)ptr, count * sizeof(value_type)));
        return new_ptr;
    }

    template <typename U>
    void deallocate(U * ptr, size_type count) {
        assert(ptr != nullptr);
        if (kAlignment != 0)
            _AlignedDeallocate((void *)ptr, count * sizeof(value_type));
        else
            _Deallocate((void *)ptr, count * sizeof(value_type));
    }

    bool isAutoRelease() { return true; }
};

template <class T, std::size_t Alignment = align_of<T>::value>
struct std_new_allocator : public allocator_base<
            std_new_allocator<T, Alignment>, T, Alignment> {
    typedef std_new_allocator<T, Alignment>         this_type;
    typedef allocator_base<this_type, T, Alignment> base_type;

    typedef typename base_type::value_type          value_type;
    typedef typename base_type::pointer             pointer;
    typedef typename base_type::const_pointer       const_pointer;
    typedef typename base_type::reference           reference;
    typedef typename base_type::reference           const_reference;

    typedef typename base_type::difference_type     difference_type;
    typedef typename base_type::size_type           size_type;

    std_new_allocator() noexcept {}
    std_new_allocator(const this_type & other) noexcept {}
    template <typename U>
    std_new_allocator(const std_new_allocator<U, Alignment> & other) noexcept {}

    this_type & operator = (const this_type & other) noexcept {
        return *this;
    }
    template <typename U>
    this_type & operator = (const std_new_allocator<U, Alignment> & other) noexcept {
        return *this;
    }

    ~std_new_allocator() {}

    pointer allocate(size_type count) {
        pointer ptr = static_cast<pointer>(::operator new[](count * sizeof(value_type)));
        return ptr;
    }

    template <typename U>
    pointer reallocate(U * ptr, size_type count) {
        pointer new_ptr = static_cast<pointer>(::operator new[](count * sizeof(value_type)));
        return new_ptr;
    }

    template <typename U>
    void deallocate(U * ptr, size_type count) {
        assert(ptr != nullptr);
        ::operator delete[]((void *)ptr, count * sizeof(value_type));
    }

    bool isAutoRelease() { return true; }
};

template <typename T, std::size_t Alignment = align_of<T>::value>
struct malloc_allocator : public allocator_base<
            malloc_allocator<T, Alignment>, T, Alignment> {
    typedef malloc_allocator<T, Alignment>          this_type;
    typedef allocator_base<this_type, T, Alignment> base_type;

    typedef typename base_type::value_type          value_type;
    typedef typename base_type::pointer             pointer;
    typedef typename base_type::const_pointer       const_pointer;
    typedef typename base_type::reference           reference;
    typedef typename base_type::reference           const_reference;

    typedef typename base_type::difference_type     difference_type;
    typedef typename base_type::size_type           size_type;

    malloc_allocator() noexcept {}
    malloc_allocator(const this_type & other) noexcept {}
    template <typename U>
    malloc_allocator(const malloc_allocator<U, Alignment> & other) noexcept {}

    this_type & operator = (const this_type & other) noexcept {
        return *this;
    }
    template <typename U>
    this_type & operator = (const malloc_allocator<U, Alignment> & other) noexcept {
        return *this;
    }

    ~malloc_allocator() {}

    pointer allocate(size_type count) {
        pointer ptr = static_cast<pointer>(std::malloc(count * sizeof(value_type)));
        return ptr;
    }

    template <typename U>
    pointer reallocate(U * ptr, size_type count) {
        pointer new_ptr = static_cast<pointer>(std::realloc((void *)ptr, count * sizeof(value_type)));
        return new_ptr;
    }

    template <typename U>
    void deallocate(U * ptr, size_type count) {
        assert(ptr != nullptr);
        std::free((void *)ptr);
    }

    bool isAutoRelease() { return true; }
};

template <class T, std::size_t Alignment = align_of<T>::value>
struct dummy_allocator : public allocator_base<
            dummy_allocator<T, Alignment>, T, Alignment> {
    typedef dummy_allocator<T, Alignment>           this_type;
    typedef allocator_base<this_type, T, Alignment> base_type;

    typedef typename base_type::value_type          value_type;
    typedef typename base_type::pointer             pointer;
    typedef typename base_type::const_pointer       const_pointer;
    typedef typename base_type::reference           reference;
    typedef typename base_type::reference           const_reference;

    typedef typename base_type::difference_type     difference_type;
    typedef typename base_type::size_type           size_type;

    dummy_allocator() noexcept {}
    dummy_allocator(const this_type & other) noexcept {}
    template <typename U>
    dummy_allocator(const dummy_allocator<U, Alignment> & other) noexcept {}

    this_type & operator = (const this_type & other) noexcept {
        return *this;
    }
    template <typename U>
    this_type & operator = (const dummy_allocator<U, Alignment> & other) noexcept {
        return *this;
    }

    ~dummy_allocator() {}

    pointer allocate(size_type count) {
        pointer nullptr;
    }

    template <typename U>
    pointer reallocate(U * ptr, size_type count) {
        pointer nullptr;
    }

    template <typename U>
    void deallocate(U * ptr, size_type count) {
    }

    bool isAutoRelease() { return false; }
};

} // namespace jstd

#endif // JSTD_ALLOCATOR_H
