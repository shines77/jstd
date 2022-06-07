
#ifndef JSTD_ALIGNED_ALLOCATOR_H
#define JSTD_ALIGNED_ALLOCATOR_H

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

#include "jstd/basic/stddef.h"
#include "jstd/basic/stdint.h"
#include "jstd/basic/stdsize.h"

#include <stdio.h>
#include <malloc.h>
#include <memory.h>
#include <errno.h>
#include <assert.h>

#include <stdlib.h>
#include <inttypes.h>

#include "jstd/allocator.h"
#include "jstd/type_traits.h"

#include "jstd/memory/aligned_malloc.h"

namespace jstd {

template <typename T, std::size_t Alignment = align_of<T>::value>
class aligned_allocator {
public:
    typedef T                               value_type;
    typedef T *                             pointer;
    typedef const T *                       const_pointer;
    typedef T &                             reference;
    typedef const T &                       const_reference;

    typedef std::ptrdiff_t                  difference_type;
    typedef std::size_t                     size_type;

    typedef aligned_allocator<T, Alignment> this_type;

    static const std::size_t kAlignment = compile_time::round_up_to_pow2<Alignment>::value;

    /*
     * The following values are non-zero, constant, odd, large, and atypical
     *      Non-zero values help find bugs assuming zero filled data.
     *      Constant values are good so that memory filling is deterministic
     *          (to help make bugs reproducable).  Of course it is bad if
     *          the constant filling of weird values masks a bug.
     *      Mathematically odd numbers are good for finding bugs assuming a cleared
     *          lower bit.
     *      Large numbers (byte values at least) are less typical, and are good
     *          at finding bad addresses.
     *      Atypical values (i.e. not too often) are good since they typically
     *          cause early detection in code.
     *      For the case of no-man's land and free blocks, if you store to any
     *          of these locations, the memory integrity checker will detect it.
     *
     *      kcAlignLandFill has been changed from 0xBD to 0xED, to ensure that
     *      4 bytes of that (0xEDEDEDED) would give an inaccessible address under 3gb.
     */

    /* fill no-man's sign for aligned routines */
    static const unsigned char kcAlignSignFill  = 0xE9;

    /* fill no-man's sign for free routines */
    static const unsigned char kcClearSignFill  = 0x00;

    /* fill no-man's land with this */
    static const unsigned char kcNoMansLandFill = 0xFD;

    /* fill no-man's land for aligned routines */
    static const unsigned char kcAlignLandFill  = 0xED;

    /* fill free objects with this */
    static const unsigned char kcDeadLandFill   = 0xDD;

    /* fill new objects with this */
    static const unsigned char kcCleanLandFill  = 0xCD;

#pragma pack(push, 1)
    struct aligned_block_header {
        void *          pvAlloc;
#ifndef NDEBUG
        unsigned char   sign[JM_ALIGN_SIGN_SIZE];
#endif
    };
#pragma pack(pop)

private:
    /////////////////////////////////////////////////////////////////////////////

    static JM_INLINE
    bool JM_X86_CDECL
    check_bytes(unsigned char * pb,
                unsigned char bCheck,
                size_t nSize) {
        bool bOkay = true;
        while (nSize--) {
            if (*pb++ != bCheck) {
                bOkay = false;
            }
        }
        return bOkay;
    }

    /////////////////////////////////////////////////////////////////////////////

    static JM_INLINE
    bool JM_X86_CDECL
    is_power_of_2(size_t n) {
        return ((n & (n - 1)) == 0);
    }

    static JM_INLINE
    size_t JM_X86_CDECL
    next_power_of_2(size_t n) {
        if (n != 0) {
            // ms1b
            --n;
            n |= n >> 1;
            n |= n >> 2;
            n |= n >> 4;
            n |= n >> 8;
            n |= n >> 16;
#if JM_MALLOC_IS_X64
            n |= n >> 32;
#endif
            return ++n;
        }
    
        return 0;
    }

    static JM_INLINE
    size_t JM_X86_CDECL
    round_up_power_of_2(size_t alignment) {
        if (this_type::is_power_of_2(alignment)) {
            assert(alignment > 0);
            return alignment;
        }
        else {
            alignment = this_type::next_power_of_2(alignment);
            assert(alignment > 0);
            assert(this_type::next_power_of_2(alignment));
            return alignment;
        }
    }

    static JM_INLINE
    size_t JM_X86_CDECL
    adjust_alignment(size_t alignment) {
        //
        // The alignment must be a power of 2,
        // Although we will fix the value of alignment,
        // we must also report the assertion in debug mode.
        //
        assert(this_type::is_power_of_2(alignment));
        if (sizeof(uintptr_t) > JM_MALLOC_ALIGNMENT) {
            alignment = (alignment >= sizeof(uintptr_t)) ? alignment : sizeof(uintptr_t);
        }
        return alignment;
    }

    static JM_INLINE
    void * JM_X86_CDECL
    adjust_pointer(void * ptr) {
        void * pvData;
#if JM_SUPPORT_ALIGNED_OFFSET_MALLOC
        // The ptr value aligned to sizeof(uintptr_t) bytes
        pvData = (void *)((uintptr_t)ptr & ~(sizeof(uintptr_t) - 1));
#else
        assert(((uintptr_t)ptr & (sizeof(uintptr_t) - 1)) == 0);
        pvData = ptr;
#endif
        return pvData;
    }

    /////////////////////////////////////////////////////////////////////////////

    static JM_INLINE
    int JM_X86_CDECL
    check_param(void * ptr, size_t alignment) {
#ifndef NDEBUG
        int malloc_errno;
        uintptr_t pvAlloc, pvData;
        aligned_block_header * pBlockHdr;
        ptrdiff_t headerPaddingSize;
   
        errno = 0;

#if JM_SUPPORT_ALIGNED_OFFSET_MALLOC
        // The ptr value aligned to sizeof(uintptr_t) bytes
        pvData = (uintptr_t)ptr & ~(sizeof(uintptr_t) - 1);
#else
        assert(((uintptr_t)ptr & (sizeof(uintptr_t) - 1)) == 0);
        pvData = (uintptr_t)ptr;
#endif

        //
        // The alignment must be a power of 2,
        // the behavior is undefined if alignment is not a power of 2.
        //
        alignment = this_type::adjust_alignment(alignment);

        // Points to the beginning of the allocated block
        pBlockHdr = (aligned_block_header *)pvData - 1;
        assert(((uintptr_t)pBlockHdr & (sizeof(uintptr_t) - 1)) == 0);

        pvAlloc = (uintptr_t)pBlockHdr->pvAlloc;

        // For debug diagnose
        headerPaddingSize = (ptrdiff_t)pvData - (ptrdiff_t)(pvAlloc);
        if (headerPaddingSize < (ptrdiff_t)sizeof(aligned_block_header) ||
            headerPaddingSize > (ptrdiff_t)(sizeof(aligned_block_header) + (alignment - 1))) {
            errno = EINVAL;
            // We don't know where pvData was allocated
            fprintf(stderr, "Damage before 0x%p which was allocated by aligned routine\n\n", (void *)pvData);
        }

        assert(headerPaddingSize >= (ptrdiff_t)sizeof(aligned_block_header));
        assert(headerPaddingSize <= (ptrdiff_t)(sizeof(aligned_block_header) + (alignment - 1)));

        if (!this_type::check_bytes(pBlockHdr->sign, kcAlignSignFill, JM_ALIGN_SIGN_SIZE)) {
            // We don't know where (file, linenum) pvData was allocated
            fprintf(stderr, "Damage before 0x%p which was allocated by aligned routine\n\n", (void *)pvData);
            assert(false);
        }

        return errno;
#else
        return 0;
#endif // !NDEBUG
    }

    /////////////////////////////////////////////////////////////////////////////////////////

    static JM_FORCE_INLINE
    void * JM_X86_CDECL
    aligned_to_addr(void * ptr, size_t size, size_t alloc_size, size_t alignment) {
        uintptr_t pvAlloc, pvData;
        aligned_block_header * pBlockHdr;
#ifndef NDEBUG
        ptrdiff_t headerPaddingSize;
        ptrdiff_t footerPaddingSize;
#endif
        assert(ptr != nullptr);
        assert(alloc_size == sizeof(aligned_block_header) + size + (alignment - 1));
        assert(this_type::is_power_of_2(alignment));
        assert(alignment >= sizeof(uintptr_t));

        pvAlloc = (uintptr_t)ptr;
    
        // The output data pointer aligned to alignment bytes
        pvData = (uintptr_t)((pvAlloc + sizeof(aligned_block_header) + (alignment - 1))
                            & (~(alignment - 1)));

        assert(((uintptr_t)pvData & (sizeof(uintptr_t) - 1)) == 0);

        pBlockHdr = (aligned_block_header *)(pvData) - 1;
        assert((uintptr_t)pBlockHdr >= pvAlloc);

#ifndef NDEBUG
        std::memset((void *)pBlockHdr->sign, kcAlignSignFill, JM_ALIGN_SIGN_SIZE);
#endif
        pBlockHdr->pvAlloc = (void *)pvAlloc;

#ifndef NDEBUG
        // For debug diagnose
        headerPaddingSize = (ptrdiff_t)pvData - (ptrdiff_t)pvAlloc;
        footerPaddingSize = (ptrdiff_t)alloc_size - (ptrdiff_t)size - headerPaddingSize;

        assert(headerPaddingSize >= (ptrdiff_t)sizeof(aligned_block_header));
        assert(headerPaddingSize <= (ptrdiff_t)(sizeof(aligned_block_header) + (alignment - 1)));
        assert(footerPaddingSize >= 0);
#endif
        return (void *)pvData;
    }

public:
    static JM_INLINE
    size_t JM_X86_CDECL
    original_usable_size(void * ptr) {
#ifdef _MSC_VER
        size_t alloc_size = ::_msize(ptr);
#else
        size_t alloc_size = ::malloc_usable_size(ptr);
#endif
        return alloc_size;
    }

    template <typename U>
    static JM_INLINE
    size_t JM_X86_CDECL
    original_usable_size(U * ptr) {
        void * p = reinterpret_cast<void *>(ptr);
        return this_type::original_usable_size(p);
    }

    JM_INLINE
    size_t JM_X86_CDECL
    get_usable_size(void * ptr) {
        ptrdiff_t header_size;  /* Size of the header block */
        size_t alloc_size;      /* Actual alloce size of the allocated block */
        ptrdiff_t usable_size;  /* Aligned alloce size of the user block */
        uintptr_t pvData;

        //
        // The alignment must be a power of 2,
        // the behavior is undefined if alignment is not a power of 2.
        //
        size_t alignment = this_type::adjust_alignment(kAlignment);
        JSTD_UNUSED_VARS(alignment);

        if (kAlignment <= JM_MALLOC_ALIGNMENT) {
            return this_type::original_usable_size(ptr);
        }

        /* HEADER_SIZE + FOOTER_SIZE = (ALIGNMENT - 1) + SIZE_OF(aligned_block_header)) */
        /* HEADER_SIZE + USER_SIZE + FOOTER_SIZE = TOTAL_SIZE */

        // Diagnosing in debug mode
        check_param(ptr, kAlignment);

        assert(ptr != nullptr);
        //
        // The ptr value aligned to sizeof(uintptr_t) bytes if need.
        //
        pvData = (uintptr_t)this_type::adjust_pointer(ptr);

#if JM_SUPPORT_ALIGNED_OFFSET_MALLOC
        // The ptr value aligned to sizeof(uintptr_t) bytes
        pvData = (uintptr_t)ptr & ~(sizeof(uintptr_t) - 1);
#else
        assert(((uintptr_t)ptr & (sizeof(uintptr_t) - 1)) == 0);
        pvData = (uintptr_t)ptr;
#endif

        // Points to the beginning of the allocated block
        aligned_block_header * pBlockHdr = (aligned_block_header *)pvData - 1;
        assert(((uintptr_t)pBlockHdr & (sizeof(uintptr_t) - 1)) == 0);

        alloc_size = this_type::original_usable_size(pBlockHdr->pvAlloc);

#ifdef NDEBUG
        header_size = (ptrdiff_t)pvData - (ptrdiff_t)(pBlockHdr->pvAlloc);
        usable_size = (ptrdiff_t)alloc_size - header_size;
        return (size_t)usable_size;
#else
        if (alloc_size >= (sizeof(aligned_block_header) + (alignment - 1))) {
            header_size = (ptrdiff_t)pvData - (ptrdiff_t)(pBlockHdr->pvAlloc);
            if (header_size >= sizeof(aligned_block_header)) {
                usable_size = (ptrdiff_t)alloc_size - header_size;
                if (usable_size >= 0)
                    return (size_t)usable_size;
            }
        }

        fprintf(stderr, "Damage before 0x%p which was allocated by aligned routine\n\n", (void *)pvData);
        assert(false);

        return (size_t)-1;
#endif // NDEBUG
    }

    template <typename U>
    static JM_INLINE
    size_t JM_X86_CDECL
    get_usable_size(U * ptr) {
        void * p = reinterpret_cast<void *>(ptr);
        return this_type::get_usable_size(p);
    }

    static JM_INLINE
    pointer JM_X86_CDECL
    malloc(size_t size) {
        size_t alloc_size;
        uintptr_t pvAlloc, pvData;

        //
        // The alignment must be a power of 2,
        // the behavior is undefined if alignment is not a power of 2.
        //
        size_t alignment = this_type::adjust_alignment(kAlignment);

        if (kAlignment <= JM_MALLOC_ALIGNMENT) {
            return static_cast<pointer>(std::malloc(size));
        }

        // Let alloc_size aligned to alignment bytes (isn't must need)
        alloc_size = sizeof(aligned_block_header) + size + (kAlignment - 1);

        pvAlloc = (uintptr_t)std::malloc(alloc_size);
        if (pvAlloc != (uintptr_t)nullptr) {
            // The output data pointer aligned to alignment bytes
            pvData = (uintptr_t)this_type::aligned_to_addr((void *)pvAlloc, size, alloc_size, kAlignment);
#if 0
            printf("pvAlloc = 0x%p, AllocSize = %" PRIuPTR "\n", (void *)pvAlloc, alloc_size);
            printf("pvData  = 0x%p, Size      = %" PRIuPTR ", alignment = %" PRIuPTR "\n",
                    (void *)pvData, size, alignment);
            printf("usable_size() = %" PRIuPTR ", usable_size() = %" PRIuPTR "\n",
                   this_type::usable_size((void *)pvAlloc),
                   this_type::usable_size((void *)pvData, alignment));
#endif
            return static_cast<pointer>(reinterpret_cast<void *>(pvData));
        }
        else {
            // MSVC: If size bigger than _HEAP_MAXREQ, (errno) return ENOMEM.
            // print errno;
            return nullptr;
        }
    }

    //
    // About realloc()
    //
    // See: https://www.cnblogs.com/zhaoyl/p/3954232.html
    //
    //  void * realloc(void * ptr, size_t new_size);
    //
    //  1. If ptr is not NULL, it must be returned by a previous memory allocation function,
    //     such as malloc(), calloc(), or realloc();
    //  2. if ptr is NULL, it is equivalent to calling malloc(new_size);
    //  3. if ptr is not NULL and new_size is 0, it is equivalent to calling free(ptr);
    //  4. If new_ptr is not equal to ptr (the memory block has been moved),
    //     free(ptr) is called;
    //

    /*****************************************************************************************/
    //
    //   MSDN realloc()
    //
    //   realloc() returns a void pointer to the reallocated (and possibly moved) memory block.
    //   The return value is NULL if the size is zero and the buffer argument is not NULL,
    //   or if there is not enough available memory to expand the block to the given size.
    //   In the first case, the original block is freed. In the second, the original block
    //   is unchanged. The return value points to a storage space that is guaranteed to be
    //   suitably aligned for storage of any type of object. To get a pointer to a type
    //   other than void, use a type cast on the return value.
    //
    /*****************************************************************************************/

    static JM_INLINE
    pointer JM_X86_CDECL
    realloc(void * ptr, size_t new_size) {
        aligned_block_header * pBlockHdr;
        uintptr_t pvData;
        void * pvAlloc, * new_ptr;
        void * newData;
        size_t new_alloc_size;

        //
        // The alignment must be a power of 2,
        // the behavior is undefined if alignment is not a power of 2.
        //
        size_t alignment = this_type::adjust_alignment(kAlignment);

        if (kAlignment <= JM_MALLOC_ALIGNMENT) {
            return static_cast<pointer>(std::realloc(ptr, new_size));
        }

        if (likely(ptr != nullptr)) {
            if (likely(new_size != 0)) {
                // Diagnosing in debug mode
                this_type::check_param(ptr, kAlignment);

                //
                // The ptr value aligned to sizeof(uintptr_t) bytes if need.
                //
                pvData = (uintptr_t)this_type::adjust_pointer(ptr);

                //
                // The alignment must be a power of 2,
                // the behavior is undefined if alignment is not a power of 2.
                //
                alignment = this_type::adjust_alignment(alignment);

                // Points to the beginning of the allocated block
                pBlockHdr = (aligned_block_header *)pvData - 1;
                assert(((uintptr_t)pBlockHdr & (sizeof(uintptr_t) - 1)) == 0);

                pvAlloc = pBlockHdr->pvAlloc;

                // Let new_alloc_size aligned to alignment bytes (isn't must need)
                new_alloc_size = sizeof(aligned_block_header) + new_size + (alignment - 1);

                // Use old original memory block pointer to realloc().
                new_ptr = std::realloc(pvAlloc, new_alloc_size);
                if (new_ptr != nullptr) {
                    newData = this_type::aligned_to_addr(new_ptr, new_size, new_alloc_size, alignment);
                    assert(newData != nullptr);
                    return static_cast<pointer>(newData);;
                }
                else {
                    // Unknown errors (error info read from (errno))
                }
            }
            else {
                // If ptr is not null and new_size is zero, call free(ptr) and return null.
                this_type::free(ptr);
                new_ptr = nullptr;
            }
        }
        else {
            if (likely(new_size != 0)) {
                // If ptr is null and new_size is not zero, return malloc(new_size).
                new_ptr = (void *)this_type::malloc(new_size);
            }
            else {
                // If ptr is null and new_size is zero, return null.
                new_ptr = ptr;
            }
        }

        return static_cast<pointer>(new_ptr);
    }

    template <typename U>
    static JM_INLINE
    pointer JM_X86_CDECL
    realloc(U * ptr, size_t new_size) {
        void * p = reinterpret_cast<void *>(ptr);
        return this_type::realloc(p, new_size);
    }

    static JM_INLINE
    pointer JM_X86_CDECL
    calloc(size_t count, size_t size) {
        void * pvData = this_type::malloc(count * size);
        if (pvData != nullptr) {
            std::memset(pvData, 0, count * size);
        }
        return static_cast<pointer>(pvData);
    }

    static JM_INLINE
    pointer JM_X86_CDECL
    recalloc(void * ptr, size_t count, size_t new_size) {
        void * pvData = this_type::realloc(ptr, count * new_size);
        if (pvData != nullptr) {
            std::memset(pvData, 0, count * new_size);
        }
        return static_cast<pointer>(pvData);
    }

    template <typename U>
    static JM_INLINE
    pointer JM_X86_CDECL
    recalloc(U * ptr, size_t count, size_t new_size) {
        void * p = reinterpret_cast<void *>(ptr);
        return this_type::recalloc(p, count, new_size);
    }

    static JM_INLINE
    void JM_X86_CDECL
    free(void * ptr) {
        aligned_block_header * pBlockHdr;
        uintptr_t pvData;
        void * pvAlloc;

        if (kAlignment <= JM_MALLOC_ALIGNMENT) {
            return std::free(ptr);
        }

        // Diagnosing in debug mode
        this_type::check_param(ptr, kAlignment);

        //
        // The ptr value aligned to sizeof(uintptr_t) bytes if need.
        //
        pvData = (uintptr_t)this_type::adjust_pointer(ptr);

        // Points to the beginning of the allocated block
        pBlockHdr = (aligned_block_header *)pvData - 1;
        assert(((uintptr_t)pBlockHdr & (sizeof(uintptr_t) - 1)) == 0);

        pvAlloc = pBlockHdr->pvAlloc;

#ifndef NDEBUG
        //if (check_bytes(pBlockHdr->sign, kcNoMansLandFill, JM_NO_MANS_LAND_SIZE)) {
        //    // We don't know where (file, linenum) pvData was allocated
        //    fprintf(stderr, "The block at 0x%p was not allocated by aligned routines, use free()\n\n", (void *)pvData);
        //    return;
        //}
#endif

#ifndef NDEBUG
        // Set pvAlloc's value to NULL
        pBlockHdr->pvAlloc = nullptr;

        // Set and fill clear sign
        std::memset(pBlockHdr->sign, kcClearSignFill, JM_ALIGN_SIGN_SIZE);
#endif

        // Free memory block if need
        std::free(pvAlloc);
    }

    template <typename U>
    static JM_INLINE
    void JM_X86_CDECL
    free(U * ptr) {
        this_type::free(reinterpret_cast<void *>(ptr));
    }

}; // struct aligned_allocator

} // namespace jstd

/////////////////////////////////////////////////////////////////////////////

#endif // JSTD_ALIGNED_ALLOCATOR_H
