
#ifndef JSTD_C_ALIGNED_MALLOC_H
#define JSTD_C_ALIGNED_MALLOC_H

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

/////////////////////////////////////////////////////////////////////////////

#define JMC_SUPPORT_ALIGNED_OFFSET_MALLOC    0

#if defined(WIN64) || defined(_WIN64) || defined(_M_X64) || defined(_M_AMD64) \
 || defined(_M_IA64) || defined(__amd64__) || defined(__x86_64__) || defined(_M_ARM64)
#  define JMC_MALLOC_IS_X64      1
#else
#  define JMC_MALLOC_IS_X64      0
#endif

#undef  JMC_X86_CDECL
#if !defined(JMC_MALLOC_IS_X64) || (JMC_MALLOC_IS_X64 == 0)
#define JMC_X86_CDECL    __cdecl
#else
#define JMC_X86_CDECL
#endif

/* The align sign size of aligned_block */
#define JMC_ALIGN_SIGN_SIZE      sizeof(void *)
#define JMC_NO_MANS_LAND_SIZE    sizeof(void *)

#ifndef __cplusplus
#ifndef nullptr
#define nullptr     ((void *)NULL)
#endif
#endif

#if defined(_MSC_VER)
#define JMC_INLINE              __inline
#define JMC_FORCE_INLINE        __forceinline
#define JMC_NO_INLINE           __declspec(noinline)
#define JMC_RESTRICT            __restrict
#elif defined(__GNUC__) || defined(__clang__) || defined(__linux__)
#define JMC_INLINE              __inline__
#define JMC_FORCE_INLINE        __inline__ __attribute__((always_inline))
#define JMC_NO_INLINE           __attribute__((noinline))
#define JMC_RESTRICT            __restrict__
#else
#define JMC_INLINE              inline
#define JMC_FORCE_INLINE        inline
#define JMC_NO_INLINE
#define JMC_RESTRICT
#endif

#if defined(MALLOC_ALIGNMENT)
  #define JMC_MALLOC_ALIGNMENT       MALLOC_ALIGNMENT
#else
  #if JMC_MALLOC_IS_X64
    #define JMC_MALLOC_ALIGNMENT     16
  #else
    #define JMC_MALLOC_ALIGNMENT     8
  #endif
#endif // MALLOC_ALIGNMENT

#if JMC_MALLOC_IS_X64
  #define JMC_VOID_PTR_SIZE          8
#else
  #define JMC_VOID_PTR_SIZE          4
#endif

/*********************************************************************************
#ifndef INTERNAL_SIZE_T
#define INTERNAL_SIZE_T     size_t
#endif

// The corresponding word size
#define SIZE_SZ             (sizeof(INTERNAL_SIZE_T))

#ifndef MALLOC_ALIGNMENT
#define MALLOC_ALIGNMENT    (2 * SIZE_SZ < __alignof__ (long double) \
                             ? __alignof__ (long double) : 2 * SIZE_SZ)

//#define MALLOC_ALIGNMENT  (2 * SIZE_SZ)
#endif
*********************************************************************************/

/////////////////////////////////////////////////////////////////////////////

#ifdef __cplusplus
extern "C" {
#endif

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
struct _aligned_block_header {
    void *          pvAlloc;
#ifndef NDEBUG
    unsigned char   sign[JMC_ALIGN_SIGN_SIZE];
#endif
};
#pragma pack(pop)

typedef struct _aligned_block_header    aligned_block_header_t;

/////////////////////////////////////////////////////////////////////////////

bool   JMC_X86_CDECL jm_is_pow2(size_t n);
size_t JMC_X86_CDECL jm_next_pow2(size_t n);
size_t JMC_X86_CDECL jm_round_up_pow2(size_t alignment);

// For debug
bool   JMC_X86_CDECL jm_check_bytes(unsigned char * pb,
                                   unsigned char bCheck,
                                   size_t nSize);
// For debug
int    JMC_X86_CDECL jm_aligned_check_param(void * ptr, size_t alignment);

size_t JMC_X86_CDECL jm_adjust_alignment(size_t alignment);
void * JMC_X86_CDECL jm_adjust_aligned_pointer(void * ptr);

void * JMC_X86_CDECL jm_aligned_to_addr(void * ptr, size_t size, size_t alloc_size, size_t alignment);

size_t JMC_X86_CDECL jm_usable_size(void * ptr);
size_t JMC_X86_CDECL jm_aligned_usable_size(void * ptr, size_t alignment);

void * JMC_X86_CDECL jm_aligned_malloc(size_t size, size_t alignment);
void * JMC_X86_CDECL jm_aligned_realloc(void *ptr, size_t new_size, size_t alignment);
void * JMC_X86_CDECL jm_aligned_calloc(size_t count, size_t size, size_t alignment);
void * JMC_X86_CDECL jm_aligned_recalloc(void *ptr, size_t count, size_t new_size, size_t alignment);

void   JMC_X86_CDECL jm_aligned_free(void * ptr, size_t alignment);

/////////////////////////////////////////////////////////////////////////////

static int jm_malloc_errno = 0;

#ifdef __cplusplus
}
#endif

/////////////////////////////////////////////////////////////////////////////

JMC_INLINE
bool JMC_X86_CDECL
jm_check_bytes(unsigned char * pb,
               unsigned char bCheck,
               size_t nSize)
{
    bool bOkay = true;
    while (nSize--) {
        if (*pb++ != bCheck) {
            bOkay = false;
        }
    }
    return bOkay;
}

/////////////////////////////////////////////////////////////////////////////

JMC_INLINE
bool JMC_X86_CDECL
jm_is_pow2(size_t n)
{
    return ((n & (n - 1)) == 0);
}

JMC_INLINE
size_t JMC_X86_CDECL
jm_next_pow2(size_t n)
{
    if (n != 0) {
        // ms1b
        --n;
        n |= n >> 1;
        n |= n >> 2;
        n |= n >> 4;
        n |= n >> 8;
        n |= n >> 16;
#if JMC_MALLOC_IS_X64
        n |= n >> 32;
#endif
        return ++n;
    }
    
    return 0;
}

JMC_INLINE
size_t JMC_X86_CDECL
jm_round_up_pow2(size_t alignment)
{
    if (jm_is_pow2(alignment)) {
        assert(alignment > 0);
        return alignment;
    }
    else {
        alignment = jm_next_pow2(alignment);
        assert(alignment > 0);
        assert(jm_next_pow2(alignment));
        return alignment;
    }
}

JMC_INLINE
size_t JMC_X86_CDECL
jm_adjust_alignment(size_t alignment)
{
    //
    // The alignment must be a power of 2,
    // Although we will fix the value of alignment,
    // we must also report the assertion in debug mode.
    //
    assert(jm_is_pow2(alignment));
#if (JMC_VOID_PTR_SIZE > JMC_MALLOC_ALIGNMENT)
    alignment = (alignment >= sizeof(uintptr_t)) ? alignment : sizeof(uintptr_t);
#endif

#if JMC_ADJUST_ALIGNMENT
    alignment = jm_round_up_pow2(alignment);
    assert(alignment > 0);
    assert(jm_is_pow2(alignment));
#endif
    return alignment;
}

JMC_INLINE
void * JMC_X86_CDECL
jm_adjust_aligned_pointer(void * ptr)
{
    void * pvData;
#if JMC_SUPPORT_ALIGNED_OFFSET_MALLOC
    // The ptr value aligned to sizeof(uintptr_t) bytes
    pvData = (void *)((uintptr_t)ptr & ~(sizeof(uintptr_t) - 1));
#else
    assert(((uintptr_t)ptr & (sizeof(uintptr_t) - 1)) == 0);
    pvData = ptr;
#endif
    return pvData;
}

/////////////////////////////////////////////////////////////////////////////

JMC_INLINE
int JMC_X86_CDECL
jm_aligned_check_param(void * ptr, size_t alignment)
{
#ifndef NDEBUG
    int malloc_errno;
    uintptr_t pvAlloc, pvData;
    aligned_block_header_t * pBlockHdr;
    ptrdiff_t headerPaddingSize;
   
    malloc_errno = 0;

#if JMC_SUPPORT_ALIGNED_OFFSET_MALLOC
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
    alignment = jm_adjust_alignment(alignment);

    // Points to the beginning of the allocated block
    pBlockHdr = (aligned_block_header_t *)pvData - 1;
    assert(((uintptr_t)pBlockHdr & (sizeof(uintptr_t) - 1)) == 0);

    pvAlloc = (uintptr_t)pBlockHdr->pvAlloc;

    // For debug diagnose
    headerPaddingSize = (ptrdiff_t)pvData - (ptrdiff_t)(pvAlloc);
    if (headerPaddingSize < (ptrdiff_t)sizeof(aligned_block_header_t) ||
        headerPaddingSize > (ptrdiff_t)(sizeof(aligned_block_header_t) + (alignment - 1))) {
        malloc_errno = EINVAL;
        // We don't know where pvData was allocated
        fprintf(stderr, "Damage before 0x%p which was allocated by jm_aligned routine\n\n", (void *)pvData);
    }

    assert(headerPaddingSize >= (ptrdiff_t)sizeof(aligned_block_header_t));
    assert(headerPaddingSize <= (ptrdiff_t)(sizeof(aligned_block_header_t) + (alignment - 1)));

    if (!jm_check_bytes(pBlockHdr->sign, kcAlignSignFill, JMC_ALIGN_SIGN_SIZE)) {
        // We don't know where (file, linenum) pvData was allocated
        fprintf(stderr, "Damage before 0x%p which was allocated by jm_aligned routine\n\n", (void *)pvData);
        assert(false);
    }

    return malloc_errno;
#else
    return 0;
#endif // !NDEBUG
}

/////////////////////////////////////////////////////////////////////////////////////////

JMC_FORCE_INLINE
void * JMC_X86_CDECL
jm_aligned_to_addr(void * ptr, size_t size, size_t alloc_size, size_t alignment)
{
    uintptr_t pvAlloc, pvData;
    aligned_block_header_t * pBlockHdr;
#ifndef NDEBUG
    ptrdiff_t headerPaddingSize;
    ptrdiff_t footerPaddingSize;
#endif

    assert(ptr != nullptr);
    assert(alloc_size == sizeof(aligned_block_header_t) + size + (alignment - 1));
    assert(jm_is_pow2(alignment));
    assert(alignment >= sizeof(uintptr_t));

    pvAlloc = (uintptr_t)ptr;
    
    // The output data pointer aligned to alignment bytes
    pvData = (uintptr_t)((pvAlloc + sizeof(aligned_block_header_t) + (alignment - 1))
                        & (~(alignment - 1)));

    assert(((uintptr_t)pvData & (sizeof(uintptr_t) - 1)) == 0);

    pBlockHdr = (aligned_block_header_t *)(pvData) - 1;
    assert((uintptr_t)pBlockHdr >= pvAlloc);

#ifndef NDEBUG
    memset((void *)pBlockHdr->sign, kcAlignSignFill, JMC_ALIGN_SIGN_SIZE);
#endif
    pBlockHdr->pvAlloc = (void *)pvAlloc;

#ifndef NDEBUG
    // For debug diagnose
    headerPaddingSize = (ptrdiff_t)pvData - (ptrdiff_t)pvAlloc;
    footerPaddingSize = (ptrdiff_t)alloc_size - (ptrdiff_t)size - headerPaddingSize;

    assert(headerPaddingSize >= (ptrdiff_t)sizeof(aligned_block_header_t));
    assert(headerPaddingSize <= (ptrdiff_t)(sizeof(aligned_block_header_t) + (alignment - 1)));
    assert(footerPaddingSize >= 0);
#endif

    return (void *)pvData;
}

JMC_INLINE
size_t JMC_X86_CDECL
jm_usable_size(void * ptr)
{
#ifdef _MSC_VER
    size_t alloc_size = _msize(ptr);
#else
    size_t alloc_size = malloc_usable_size(ptr);
#endif
    return alloc_size;
}

JMC_INLINE
size_t JMC_X86_CDECL
jm_aligned_usable_size(void * ptr, size_t alignment)
{
    ptrdiff_t header_size;  /* Size of the header block */
    size_t alloc_size;      /* Actual alloce size of the allocated block */
    ptrdiff_t usable_size;  /* Aligned alloce size of the user block */
    uintptr_t pvData;

    if (alignment <= JMC_MALLOC_ALIGNMENT) {
        return jm_usable_size(ptr);
    }

    /* HEADER_SIZE + FOOTER_SIZE = (ALIGNMENT - 1) + SIZE_OF(aligned_block_header_t)) */
    /* HEADER_SIZE + USER_SIZE + FOOTER_SIZE = TOTAL_SIZE */

    // Diagnosing in debug mode
    jm_aligned_check_param(ptr, alignment);

    assert(ptr != nullptr);
    //
    // The ptr value aligned to sizeof(uintptr_t) bytes if need.
    //
    pvData = (uintptr_t)jm_adjust_aligned_pointer(ptr);

    //
    // The alignment must be a power of 2,
    // the behavior is undefined if alignment is not a power of 2.
    //
    alignment = jm_adjust_alignment(alignment);

#if JMC_SUPPORT_ALIGNED_OFFSET_MALLOC
    // The ptr value aligned to sizeof(uintptr_t) bytes
    pvData = (uintptr_t)ptr & ~(sizeof(uintptr_t) - 1);
#else
    assert(((uintptr_t)ptr & (sizeof(uintptr_t) - 1)) == 0);
    pvData = (uintptr_t)ptr;
#endif

    // Points to the beginning of the allocated block
    aligned_block_header_t * pBlockHdr = (aligned_block_header_t *)pvData - 1;
    assert(((uintptr_t)pBlockHdr & (sizeof(uintptr_t) - 1)) == 0);

#ifdef _MSC_VER
    alloc_size = _msize(pBlockHdr->pvAlloc);
#else
    alloc_size = malloc_usable_size(pBlockHdr->pvAlloc);
#endif

#ifdef NDEBUG
    header_size = (ptrdiff_t)pvData - (ptrdiff_t)(pBlockHdr->pvAlloc);
    usable_size = (ptrdiff_t)alloc_size - header_size;
    return (size_t)usable_size;
#else
    if (alloc_size >= (sizeof(aligned_block_header_t) + (alignment - 1))) {
        header_size = (ptrdiff_t)pvData - (ptrdiff_t)(pBlockHdr->pvAlloc);
        if (header_size >= sizeof(aligned_block_header_t)) {
            usable_size = (ptrdiff_t)alloc_size - header_size;
            if (usable_size >= 0)
                return (size_t)usable_size;
        }
    }

    fprintf(stderr, "Damage before 0x%p which was allocated by jm_aligned routine\n\n", (void *)pvData);
    assert(false);

    return (size_t)-1;
#endif // NDEBUG
}

JMC_INLINE
void * JMC_X86_CDECL
jm_aligned_malloc(size_t size, size_t alignment)
{
    size_t alloc_size;
    uintptr_t pvAlloc, pvData;

    if (alignment <= JMC_MALLOC_ALIGNMENT) {
        return malloc(size);
    }

    //
    // The alignment must be a power of 2,
    // the behavior is undefined if alignment is not a power of 2.
    //
    alignment = jm_adjust_alignment(alignment);

    // Let alloc_size aligned to alignment bytes (isn't must need)
    alloc_size = sizeof(aligned_block_header_t) + size + (alignment - 1);

    pvAlloc = (uintptr_t)malloc(alloc_size);
    if (pvAlloc != (uintptr_t)nullptr) {
        // The output data pointer aligned to alignment bytes
        pvData = (uintptr_t)jm_aligned_to_addr((void *)pvAlloc, size, alloc_size, alignment);
#if 0
        printf("pvAlloc = 0x%p, AllocSize = %" PRIuPTR "\n", (void *)pvAlloc, alloc_size);
        printf("pvData  = 0x%p, Size      = %" PRIuPTR ", alignment = %" PRIuPTR "\n",
                (void *)pvData, size, alignment);
        printf("usable_size() = %" PRIuPTR ", jm_usable_size() = %" PRIuPTR "\n",
               jm_usable_size((void *)pvAlloc),
               jm_aligned_usable_size((void *)pvData, alignment));
#endif
        return (void *)pvData;
    }
    else {
        // MSVC: If size bigger than _HEAP_MAXREQ, return ENOMEM.
        jm_malloc_errno = errno;
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

JMC_INLINE
void * JMC_X86_CDECL
jm_aligned_realloc(void * ptr, size_t new_size, size_t alignment)
{
    aligned_block_header_t * pBlockHdr;
    uintptr_t pvData;
    void * pvAlloc, * new_ptr;
    void * newData;
    size_t new_alloc_size;

    if (alignment <= JMC_MALLOC_ALIGNMENT) {
        return realloc(ptr, new_size);
    }

    if (likely(ptr != nullptr)) {
        if (likely(new_size != 0)) {
            // Diagnosing in debug mode
            jm_aligned_check_param(ptr, alignment);

            //
            // The ptr value aligned to sizeof(uintptr_t) bytes if need.
            //
            pvData = (uintptr_t)jm_adjust_aligned_pointer(ptr);

            //
            // The alignment must be a power of 2,
            // the behavior is undefined if alignment is not a power of 2.
            //
            alignment = jm_adjust_alignment(alignment);

            // Points to the beginning of the allocated block
            pBlockHdr = (aligned_block_header_t *)pvData - 1;
            assert(((uintptr_t)pBlockHdr & (sizeof(uintptr_t) - 1)) == 0);

            pvAlloc = pBlockHdr->pvAlloc;

            // Let new_alloc_size aligned to alignment bytes (isn't must need)
            new_alloc_size = sizeof(aligned_block_header_t) + new_size + (alignment - 1);

            // Use old original memory block pointer to realloc().
            new_ptr = realloc(pvAlloc, new_alloc_size);
            if (new_ptr != nullptr) {
                newData = jm_aligned_to_addr(new_ptr, new_size, new_alloc_size, alignment);
                assert(newData != nullptr);
                return newData;
            }
            else {
                // Unknown errors
                jm_malloc_errno = errno;
            }
        }
        else {
            // If ptr is not null and new_size is zero, call free(ptr) and return null.
            jm_aligned_free(ptr, alignment);
            new_ptr = nullptr;
        }
    }
    else {
        if (likely(new_size != 0)) {
            // If ptr is null and new_size is not zero, return malloc(new_size).
            new_ptr = jm_aligned_malloc(new_size, alignment);
        }
        else {
            // If ptr is null and new_size is zero, return null.
            new_ptr = ptr;
        }
    }

    return new_ptr;
}

JMC_INLINE
void * JMC_X86_CDECL
jm_aligned_calloc(size_t count, size_t size, size_t alignment)
{
    void * pvData = jm_aligned_malloc(count * size, alignment);
    if (pvData != nullptr) {
        memset(pvData, 0, count * size);
    }
    return pvData;
}

JMC_INLINE
void * JMC_X86_CDECL
jm_aligned_recalloc(void * ptr, size_t count, size_t new_size, size_t alignment)
{
    void * pvData = jm_aligned_realloc(ptr, count * new_size, alignment);
    if (pvData != nullptr) {
        memset(pvData, 0, count * new_size);
    }
    return pvData;
}

JMC_INLINE
void JMC_X86_CDECL
jm_aligned_free(void * ptr, size_t alignment)
{
    aligned_block_header_t * pBlockHdr;
    uintptr_t pvData;
    void * pvAlloc;

    if (alignment <= JMC_MALLOC_ALIGNMENT) {
        return free(ptr);
    }

    // Diagnosing in debug mode
    jm_aligned_check_param(ptr, alignment);

    //
    // The ptr value aligned to sizeof(uintptr_t) bytes if need.
    //
    pvData = (uintptr_t)jm_adjust_aligned_pointer(ptr);

    // Points to the beginning of the allocated block
    pBlockHdr = (aligned_block_header_t *)pvData - 1;
    assert(((uintptr_t)pBlockHdr & (sizeof(uintptr_t) - 1)) == 0);

    pvAlloc = pBlockHdr->pvAlloc;

#ifndef NDEBUG
    //if (jm_check_bytes(pBlockHdr->sign, kcNoMansLandFill, JMC_NO_MANS_LAND_SIZE)) {
    //    // We don't know where (file, linenum) pvData was allocated
    //    fprintf(stderr, "The block at 0x%p was not allocated by jm_aligned routines, use free()\n\n", (void *)pvData);
    //    return;
    //}
#endif

#ifndef NDEBUG
    // Set pvAlloc's value to NULL
    pBlockHdr->pvAlloc = nullptr;

    // Set and fill clear sign
    memset(pBlockHdr->sign, kcClearSignFill, JMC_ALIGN_SIGN_SIZE);
#endif

    // Free memory block if need
    free(pvAlloc);
}

/////////////////////////////////////////////////////////////////////////////

#endif // JSTD_C_ALIGNED_MALLOC_H
