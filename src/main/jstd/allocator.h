
#ifndef JSTD_ALLOCATOR_H
#define JSTD_ALLOCATOR_H

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

#include "jstd/basic/stddef.h"
#include "jstd/basic/stdint.h"
#include "jstd/basic/stdsize.h"

#include "jstd/support/PowerOf2.h"

#include <stddef.h>
#include <malloc.h>
#include <memory.h>
#include <assert.h>

#include <cstdint>
#include <cstddef>
#include <memory>   // For std::pointer_traits<T>
#include <limits>   // For std::numeric_limits<T>::max()

#include <new>      // ::operator new, ::operator new[], ::operator delete

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

static inline
std::size_t aligned_to(std::size_t size, std::size_t alignment)
{
    assert(size >= 1);
    if (likely((size & (size - 1)) == 0)) return size;

    assert((alignment & (alignment - 1)) == 0);
    size = (size + alignment - 1) & ~(alignment - 1);
    assert((size / alignment * alignment) == size);
    return size;
}

template <typename T>
struct align_of {
    static const std::size_t value =
        (alignof(T) > alignof(std::max_align_t)) ?
         alignof(T) : alignof(std::max_align_t);
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
    static const size_type kAlignment = compile_time::round_up_to_power2<Alignment>::value;

    size_type align_of() const { return kAlignOf; }
    size_type alignment() const { return kAlignment; }

    bool is_ok(bool expression) {
        derive_type * pThis = static_cast<derive_type *>(this);
        return (!pThis->is_nothrow() || expression);
    }

    bool is_ok(pointer ptr) {
        derive_type * pThis = static_cast<derive_type *>(this);
        return (!pThis->is_nothrow() || (ptr != nullptr));
    }

    size_type max_size() const {
        // Estimate maximum array size
        return ((std::numeric_limits<std::size_t>::max)() / sizeof(T));
    }

    pointer address(reference value) const noexcept {
        return std::addressof(value);
    }

    const_pointer address(const_reference value) const noexcept {
        return std::addressof(value);
    }

    pointer create() {
        return this->create_array(1);
    }

    template <typename ...Args>
    pointer create(Args && ... args) {
        return this->create_array(1, std::forward<Args>(args)...);
    }

    pointer create_array(size_type count) {
        derive_type * pThis = static_cast<derive_type *>(this);
        pointer ptr = pThis->allocate(count);
        pointer cur = ptr;
        for (size_type i = count; i != 0; i--) {
            pThis->constructor(cur);
            cur++;
        }
        return ptr;
    }

    template <typename ...Args>
    pointer create_array(size_type count, Args && ... args) {
        derive_type * pThis = static_cast<derive_type *>(this);
        pointer ptr = pThis->allocate(count);
        pointer cur = ptr;
        for (size_type i = count; i != 0; i--) {
            pThis->constructor(cur, std::forward<Args>(args)...);
            cur++;
        }
        return ptr;
    }

    template <typename U>
    pointer re_create(U * ptr) {
        return this->re_create_array(ptr, 1);
    }

    template <typename U, typename ...Args>
    pointer re_create(U * ptr, Args && ... args) {
        return this->re_create_array(ptr, 1, std::forward<Args>(args)...);
    }

    template <typename U>
    pointer re_create_array(U * ptr, size_type count) {
        derive_type * pThis = static_cast<derive_type *>(this);
        pointer new_ptr = pThis->reallocate(ptr, count);
        pointer cur = new_ptr;
        for (size_type i = count; i != 0; i--) {
            pThis->constructor(cur);
            cur++;
        }
        return new_ptr;
    }

    template <typename U, typename ...Args>
    pointer re_create_array(U * ptr, size_type count, Args && ... args) {
        derive_type * pThis = static_cast<derive_type *>(this);
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
        derive_type * pThis = static_cast<derive_type *>(this);
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
        // call ::operator new (size_t size, void * p);
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
        // call ::operator new (size_t size, void * p, Args &&... args);
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
        derive_type * pThis = static_cast<derive_type *>(this);
        return pThis->reallocate(static_cast<pointer>(ptr), count);
    }

    void deallocate(void * ptr, size_type count) {
        derive_type * pThis = static_cast<derive_type *>(this);
        pThis->deallocate(static_cast<pointer>(ptr), count);
    }
};

template <class DeriverT, class DeriverU, class T, class U, std::size_t AlignmentT, std::size_t AlignmentU>
inline bool operator == (const allocator_base<DeriverT, T, AlignmentT> & lhs,
                         const allocator_base<DeriverU, U, AlignmentU> & rhs) {
    return (std::is_same<DeriverT, DeriverU>::value && (AlignmentT == AlignmentU));
}

template <class DeriverT, class DeriverU, class T, class U, std::size_t AlignmentT, std::size_t AlignmentU>
inline bool operator != (const allocator_base<DeriverT, T, AlignmentT> & lhs,
                         const allocator_base<DeriverU, U, AlignmentU> & rhs) {
    return !(std::is_same<DeriverT, DeriverU>::value && (AlignmentT == AlignmentU));
}

template <class T, std::size_t Alignment = align_of<T>::value, bool ThrowEx = false>
struct allocator : public allocator_base<
            allocator<T, Alignment, ThrowEx>, T, Alignment> {
    typedef allocator<T, Alignment, ThrowEx>        this_type;
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
    allocator(const allocator<U, Alignment, ThrowEx> & other) noexcept {}

    this_type & operator = (const this_type & other) noexcept {
        return *this;
    }
    template <typename U>
    this_type & operator = (const allocator<U, Alignment, ThrowEx> & other) noexcept {
        return *this;
    }

    ~allocator() {}

    pointer allocate(size_type count = 1) {
        pointer ptr;
        if (kAlignment != 0)
            ptr = static_cast<pointer>(_AlignedAllocate(count * sizeof(value_type), kAlignment));
        else
            ptr = static_cast<pointer>(_Allocate(count * sizeof(value_type)));
        if (ThrowEx && (ptr == nullptr)) {
            throw std::bad_alloc();
        }
        return ptr;
    }

    template <typename U>
    pointer reallocate(U * ptr, size_type count = 1) {
        pointer new_ptr;
        if (kAlignment != 0)
            new_ptr = static_cast<pointer>(_AlignedReallocate((void *)ptr, count * sizeof(value_type), kAlignment));
        else
            new_ptr = static_cast<pointer>(_Reallocate((void *)ptr, count * sizeof(value_type)));
        if (ThrowEx && (new_ptr == nullptr)) {
            throw std::bad_alloc();
        }
        return new_ptr;
    }

    template <typename U>
    void deallocate(U * ptr, size_type count = 1) {
        assert(ptr != nullptr);
        if (kAlignment != 0)
            _AlignedDeallocate((void *)ptr, count * sizeof(value_type));
        else
            _Deallocate((void *)ptr, count * sizeof(value_type));
    }

    bool is_auto_release() { return true; }
    bool is_nothrow() { return !ThrowEx; }
};

template <class T, std::size_t Alignment = align_of<T>::value>
struct std_new_allocator : public allocator_base<
            std_new_allocator<T, Alignment>, T, Alignment> {
    typedef std_new_allocator<T, Alignment>             this_type;
    typedef allocator_base<this_type, T, Alignment>     base_type;

    typedef typename base_type::value_type              value_type;
    typedef typename base_type::pointer                 pointer;
    typedef typename base_type::const_pointer           const_pointer;
    typedef typename base_type::reference               reference;
    typedef typename base_type::reference               const_reference;

    typedef typename base_type::difference_type         difference_type;
    typedef typename base_type::size_type               size_type;

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

    pointer allocate(size_type count = 1) {
        // ::operator new[](size_type n) maybe throw a std::bad_alloc() exception.
        pointer ptr = static_cast<pointer>(::operator new[](count * sizeof(value_type)));
        return ptr;
    }

    template <typename U>
    pointer reallocate(U * ptr, size_type count = 1) {
        // ::operator new[](size_type n) maybe throw a std::bad_alloc() exception.
        pointer new_ptr = static_cast<pointer>(::operator new[](count * sizeof(value_type)));
        return new_ptr;
    }

    template <typename U>
    void deallocate(U * ptr, size_type count = 1) {
        assert(ptr != nullptr);
        ::operator delete[]((void *)ptr, count * sizeof(value_type));
    }

    bool is_auto_release() { return true; }
    bool is_nothrow() { return false; }
};

template <class T, std::size_t Alignment = align_of<T>::value, bool ThrowEx = false>
struct nothrow_allocator : public allocator_base<
            nothrow_allocator<T, Alignment, ThrowEx>, T, Alignment> {
    typedef nothrow_allocator<T, Alignment, ThrowEx>    this_type;
    typedef allocator_base<this_type, T, Alignment>     base_type;

    typedef typename base_type::value_type              value_type;
    typedef typename base_type::pointer                 pointer;
    typedef typename base_type::const_pointer           const_pointer;
    typedef typename base_type::reference               reference;
    typedef typename base_type::reference               const_reference;

    typedef typename base_type::difference_type         difference_type;
    typedef typename base_type::size_type               size_type;

    nothrow_allocator() noexcept {}
    nothrow_allocator(const this_type & other) noexcept {}
    template <typename U>
    nothrow_allocator(const nothrow_allocator<U, Alignment, ThrowEx> & other) noexcept {}

    this_type & operator = (const this_type & other) noexcept {
        return *this;
    }
    template <typename U>
    this_type & operator = (const nothrow_allocator<U, Alignment, ThrowEx> & other) noexcept {
        return *this;
    }

    ~nothrow_allocator() {}

    pointer allocate(size_type count = 1) {
        pointer ptr = static_cast<pointer>(::operator new[](count * sizeof(value_type), std::nothrow));
        if (ThrowEx && (ptr == nullptr)) {
            throw std::bad_alloc();
        }
        return ptr;
    }

    template <typename U>
    pointer reallocate(U * ptr, size_type count = 1) {
        pointer new_ptr = static_cast<pointer>(::operator new[](count * sizeof(value_type), std::nothrow));
        if (ThrowEx && (new_ptr == nullptr)) {
            throw std::bad_alloc();
        }
        return new_ptr;
    }

    template <typename U>
    void deallocate(U * ptr, size_type count = 1) {
        assert(ptr != nullptr);
        ::operator delete[]((void *)ptr, std::nothrow);
    }

    bool is_auto_release() { return true; }
    bool is_nothrow() { return !ThrowEx; }
};

template <typename T, std::size_t Alignment = align_of<T>::value, bool ThrowEx = false>
struct malloc_allocator : public allocator_base<
            malloc_allocator<T, Alignment, ThrowEx>, T, Alignment> {
    typedef malloc_allocator<T, Alignment, ThrowEx> this_type;
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
    malloc_allocator(const malloc_allocator<U, Alignment, ThrowEx> & other) noexcept {}

    this_type & operator = (const this_type & other) noexcept {
        return *this;
    }
    template <typename U>
    this_type & operator = (const malloc_allocator<U, Alignment, ThrowEx> & other) noexcept {
        return *this;
    }

    ~malloc_allocator() {}

    pointer allocate(size_type count = 1) {
        pointer ptr = static_cast<pointer>(std::malloc(count * sizeof(value_type)));
        return ptr;
    }

    template <typename U>
    pointer reallocate(U * ptr, size_type count = 1) {
        pointer new_ptr = static_cast<pointer>(std::realloc((void *)ptr, count * sizeof(value_type)));
        return new_ptr;
    }

    template <typename U>
    void deallocate(U * ptr, size_type count = 1) {
        assert(ptr != nullptr);
        std::free((void *)ptr);
    }

    bool is_auto_release() { return true; }
    bool is_nothrow() { return false; }
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

    pointer allocate(size_type count = 1) {
        return nullptr;
    }

    template <typename U>
    pointer reallocate(U * ptr, size_type count = 1) {
        return nullptr;
    }

    template <typename U>
    void deallocate(U * ptr, size_type count = 1) {
    }

    bool is_auto_release() { return false; }
    bool is_nothrow() { return false; }
};

} // namespace jstd

#endif // JSTD_ALLOCATOR_H
