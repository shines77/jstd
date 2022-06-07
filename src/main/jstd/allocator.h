
#ifndef JSTD_ALLOCATOR_H
#define JSTD_ALLOCATOR_H

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

#include "jstd/basic/stddef.h"
#include "jstd/basic/stdint.h"
#include "jstd/basic/stdsize.h"

#include "jstd/basic/base.h"
#include "jstd/support/PowerOf2.h"
#include "jstd/memory/c_aligned_malloc.h"
#include "jstd/memory/aligned_allocator.h"

#include <stddef.h>
#include <malloc.h>
#include <memory.h>
#include <assert.h>

#include <cstdint>
#include <cstddef>
#include <cstdlib>      // For std::malloc(), std::free()
#include <memory>       // For std::pointer_traits<T>
#include <limits>       // For std::numeric_limits<T>::max()
#include <type_traits>  // For std::alignment_of<T>

#include <new>          // For ::operator new, ::operator new[], ::operator delete

#define USE_JM_ALIGNED_MALLOC   1

#define JSTD_DEFAULT_ALIGNMENT  alignof(std::max_align_t)

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

#if USE_JM_ALIGNED_MALLOC

static inline
void * _AlignedAllocate(std::size_t size, std::size_t alignment) {
    void * ptr = jm_aligned_malloc(size, alignment);
    return ptr;
}

static inline
void * _AlignedReallocate(void * ptr, std::size_t size, std::size_t alignment) {
    void * new_ptr = jm_aligned_realloc(ptr, size, alignment);
    return new_ptr;
}

static inline
void _AlignedDeallocate(void * p, std::size_t size = 0, std::size_t alignment = JSTD_DEFAULT_ALIGNMENT) {
    jm_aligned_free(p, alignment);
}

#else // !USE_JM_ALIGNED_MALLOC

static inline
void * _AlignedAllocate(std::size_t size, std::size_t alignment) {
    assert(size != 0);
#if defined(_WIN32) || defined(WIN32) || defined(OS_WINDOWS) || defined(__WINDOWS__)
    void * ptr = _aligned_malloc(size, alignment);
#else
    void * ptr = memalign(alignment, size);
#endif
    return ptr;
}

static inline
void * _AlignedReallocate(void * ptr, std::size_t size, std::size_t alignment) {
    assert(size != 0);
#if defined(_WIN32) || defined(WIN32) || defined(OS_WINDOWS) || defined(__WINDOWS__)
    void * new_ptr = _aligned_realloc(ptr, size, alignment);
#else
    // On Unix/Linux, it's have no posix_memalign_realloc() function.
    // See: https://stackoverflow.com/questions/9078259/does-realloc-keep-the-memory-alignment-of-posix-memalign
    // See: https://www.jb51.cc/c/115220.html
    void * new_ptr = memalign(alignment, size);
#endif
    return new_ptr;
}

static inline
void _AlignedDeallocate(void * p, std::size_t size = 0, std::size_t alignment = JSTD_DEFAULT_ALIGNMENT) {
#if defined(_WIN32) || defined(WIN32) || defined(OS_WINDOWS) || defined(__WINDOWS__)
    _aligned_free(p);
#else
    free(p);
#endif
}

#endif // USE_JM_ALIGNED_MALLOC

static inline
std::size_t align_to(std::size_t size, std::size_t alignment)
{
    assert(size >= 1);
    if (likely((size & (size - 1)) == 0)) return size;

    assert((alignment & (alignment - 1)) == 0);
    size = (size + alignment - 1) & ~(alignment - 1);
    assert((size / alignment * alignment) == size);
    return size;
}

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
            pThis->construct(cur);
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
            pThis->construct(cur, std::forward<Args>(args)...);
            cur++;
        }
        return ptr;
    }

    template <typename U>
    pointer respawn(U * ptr) {
        return this->respawn_array(ptr, 1);
    }

    template <typename U, typename ...Args>
    pointer respawn(U * ptr, Args && ... args) {
        return this->respawn_array(ptr, 1, std::forward<Args>(args)...);
    }

    template <typename U>
    pointer respawn_array(U * ptr, size_type count) {
        derive_type * pThis = static_cast<derive_type *>(this);
        pointer new_ptr = pThis->reallocate(ptr, count);
        pointer cur = new_ptr;
        for (size_type i = count; i != 0; i--) {
            pThis->construct(cur);
            cur++;
        }
        return new_ptr;
    }

    template <typename U, typename ...Args>
    pointer respawn_array(U * ptr, size_type count, Args && ... args) {
        derive_type * pThis = static_cast<derive_type *>(this);
        pointer new_ptr = pThis->reallocate(ptr, count);
        pointer cur = new_ptr;
        for (size_type i = count; i != 0; i--) {
            pThis->construct(cur, std::forward<Args>(args)...);
            cur++;
        }
        return new_ptr;
    }

    template <typename U>
    void destroy(U * ptr) {
        this->destroy_array(ptr, 1);
    }

    template <typename U>
    void destroy_array(U * ptr, size_type count) {
        derive_type * pThis = static_cast<derive_type *>(this);
        assert(ptr != nullptr);
        U * cur = ptr;
        for (size_type i = count; i != 0; i--) {
            pThis->destruct(cur);
            cur++;
        }
        pThis->deallocate(ptr, count);
    }

    // placement new
    pointer construct(pointer ptr) {
        assert(ptr != nullptr);
        void * v_ptr = static_cast<void *>(ptr);
        // call ::operator new (size_t size, void * p);
        return ::new (v_ptr) value_type();
    }

    // placement new
    pointer construct(void * ptr) {
        return this->construct(static_cast<pointer>(ptr));
    }

    // placement new (Args...)
    template <typename ...Args>
    pointer construct(pointer ptr, Args && ... args) {
        assert(ptr != nullptr);
        void * v_ptr = static_cast<void *>(ptr);
        // call ::operator new (size_t size, void * p, Args &&... args);
        return static_cast<pointer>(::new (v_ptr) value_type(std::forward<Args>(args)...));
    }

    // placement new (Args...)
    template <typename ...Args>
    pointer construct(void * ptr, Args && ... args) {
        return this->construct(static_cast<pointer>(ptr), std::forward<Args>(args)...);
    }

    template <typename U>
    void destruct(U * ptr) {
        assert(ptr != nullptr);
        ptr->~U();
    }

    void destruct(void * ptr) {
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

template <class T, std::size_t Alignment = align_of<T>::value, bool ThrowEx = true>
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

    static const bool kThrowEx = ThrowEx;
    static const size_type kAlignOf = base_type::kAlignOf;
    static const size_type kAlignment = base_type::kAlignment;
#if defined(MALLOC_ALIGNMENT)
    static const size_type kMallocDefaultAlignment = compile_time::round_up_to_pow2<MALLOC_ALIGNMENT>::value;
#else
#if defined(WIN64) || defined(_WIN64) || defined(_M_X64) || defined(_M_AMD64) \
 || defined(_M_IA64) || defined(__amd64__) || defined(__x86_64__) || defined(_M_ARM64)
    static const size_type kMallocDefaultAlignment = 16;
#else
    static const size_type kMallocDefaultAlignment = 8;
#endif
#endif // MALLOC_ALIGNMENT

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

    template <typename Other>
    struct rebind {
        typedef allocator<Other, Alignment, ThrowEx> type;
    };

    bool needAlignedAllocaote() const {
#if 1
        return (kAlignment > kMallocDefaultAlignment);
#else
        return ((kAlignment != 0) && (kAlignment != kMallocDefaultAlignment));
#endif
    }

    pointer allocate(size_type count = 1) {
        pointer ptr;
        if (needAlignedAllocaote())
            ptr = aligned_allocator<T, kAlignment>::malloc(count * sizeof(value_type));
        else
            ptr = static_cast<pointer>(std::malloc(count * sizeof(value_type)));
        if (ThrowEx && (ptr == nullptr)) {
            throw std::bad_alloc();
        }
        return ptr;
    }

    template <typename U>
    pointer reallocate(U * ptr, size_type count = 1) {
        pointer new_ptr;
        if (needAlignedAllocaote())
            new_ptr = aligned_allocator<T, kAlignment>::realloc(ptr, count * sizeof(value_type));
        else
            new_ptr = static_cast<pointer>(std::realloc((void *)ptr, count * sizeof(value_type)));
        if (ThrowEx && (new_ptr == nullptr)) {
            throw std::bad_alloc();
        }
        return new_ptr;
    }

    template <typename U>
    void deallocate(U * ptr, size_type count = 1) {
        assert(ptr != nullptr);
        if (needAlignedAllocaote())
            aligned_allocator<T, kAlignment>::free((void *)ptr);
        else
            std::free((void *)ptr);
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

    static const bool kThrowEx = true;

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

    template <typename Other>
    struct rebind {
        typedef std_new_allocator<Other, Alignment> type;
    };

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
    bool is_nothrow() { return !kThrowEx; }
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

    static const bool kThrowEx = ThrowEx;

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

    template <typename Other>
    struct rebind {
        typedef nothrow_allocator<Other, Alignment, ThrowEx> type;
    };

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

    static const bool kThrowEx = ThrowEx;

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

    template <typename Other>
    struct rebind {
        typedef malloc_allocator<Other, Alignment, ThrowEx> type;
    };

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
    bool is_nothrow() { return !ThrowEx; }
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

    static const bool kThrowEx = false;

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

    template <typename Other>
    struct rebind {
        typedef dummy_allocator<Other, Alignment> type;
    };

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
    bool is_nothrow() { return !kThrowEx; }
};

} // namespace jstd

#endif // JSTD_ALLOCATOR_H
