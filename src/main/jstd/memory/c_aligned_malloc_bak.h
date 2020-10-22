
#ifndef JSTD_C_ALIGNED_MALLOC_H
#define JSTD_C_ALIGNED_MALLOC_H

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

#include "jstd/basic/stddef.h"
#include "jstd/basic/stdint.h"
#include "jstd/basic/stdsize.h"

#include <malloc.h>
#include <memory.h>
#include <errno.h>
#include <assert.h>

/////////////////////////////////////////////////////////////////////////////

#if defined(WIN64) || defined(_WIN64) || defined(_M_X64) || defined(_M_AMD64) \
 || defined(_M_IA64) || defined(__amd64__) || defined(__x86_64__) || defined(_M_ARM64)
#  define JM_MALLOC_IS_X64      1
#else
#  define JM_MALLOC_IS_X64      0
#endif

#undef  JM_X86_CDECL
#if !defined(JM_MALLOC_IS_X64) || (JM_MALLOC_IS_X64 == 0)
#define JM_X86_CDECL    __cdecl
#else
#define JM_X86_CDECL
#endif

/* The align sign size of aligned_block */
#define JM_ALIGN_SIGN_SIZE      sizeof(void *)

#ifndef __cplusplus
#ifndef nullptr
#define nullptr     ((void *)NULL)
#endif
#endif

#if defined(_MSC_VER)
#define JM_MALLOC_INLINE    __inline
#elif defined(__GNUC__) || defined(__clang__)
#define JM_MALLOC_INLINE    __inline__
#else
#define JM_MALLOC_INLINE    inline
#endif

#ifndef _MSC_VER
  #ifndef _RPT1
    #define _RPT1(rptno, msg, arg1)
  #endif
#endif

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
static unsigned char kcAlignSignFill  = 0xE9;

/* fill no-man's sign for free routines */
static unsigned char kcClearSignFill  = 0x00;

/* fill no-man's land with this */
static unsigned char kcNoMansLandFill = 0xFD;

/* fill no-man's land for aligned routines */
static unsigned char kcAlignLandFill  = 0xED;

/* fill free objects with this */
static unsigned char kcDeadLandFill   = 0xDD;

/* fill new objects with this */
static unsigned char kcCleanLandFill  = 0xCD;

struct _aligned_block_header {
    void *          pvAlloc;
#if JM_USE_ALIGN_SIGN
    unsigned char   sign[JM_ALIGN_SIGN_SIZE];
#endif
};

typedef struct _aligned_block_header    aligned_block_header_t;

/////////////////////////////////////////////////////////////////////////////

bool   JM_X86_CDECL jm_is_power_of_2(size_t v);
size_t JM_X86_CDECL jm_next_power_of_2(size_t n);

size_t JM_X86_CDECL jm_aligned_usable_size(void * ptr, size_t alignment);
size_t JM_X86_CDECL jm_aligned_offset_usable_size(void * ptr, size_t alignment, size_t offset);

void * JM_X86_CDECL jm_aligned_malloc(size_t size, size_t alignment);
void * JM_X86_CDECL jm_aligned_realloc(void *ptr, size_t new_size, size_t alignment);
void * JM_X86_CDECL jm_aligned_calloc(size_t count, size_t size, size_t alignment);
void * JM_X86_CDECL jm_aligned_recalloc(void *ptr, size_t count, size_t new_size, size_t alignment);

void * JM_X86_CDECL jm_aligned_offset_malloc(size_t size, size_t alignment, size_t offset);
void * JM_X86_CDECL jm_aligned_offset_realloc(void * ptr, size_t new_size, size_t alignment, size_t offset);
void * JM_X86_CDECL jm_aligned_offset_calloc(size_t count, size_t size, size_t alignment, size_t offset);
void * JM_X86_CDECL jm_aligned_offset_recalloc(void * ptr, size_t count, size_t new_size, size_t alignment, size_t offset);

/////////////////////////////////////////////////////////////////////////////

static int jm_malloc_errno = 0;

#ifdef __cplusplus
}
#endif

/////////////////////////////////////////////////////////////////////////////

static JM_MALLOC_INLINE
bool JM_X86_CDECL
jm_is_power_of_2(size_t v)
{
    return ((v & (v - 1)) == 0);
}

static JM_MALLOC_INLINE
size_t JM_X86_CDECL
jm_next_power_of_2(size_t n)
{
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

static JM_MALLOC_INLINE
size_t JM_X86_CDECL
jm_adjust_alignment(size_t alignment)
{
    if (jm_is_power_of_2(alignment)) {
        assert(alignment > 0);
        return alignment;
    }
    else {
        alignment = jm_next_power_of_2(alignment);
        assert(alignment > 0);
        assert(jm_next_power_of_2(alignment));
        return alignment;
    }
}

static JM_MALLOC_INLINE
size_t JM_X86_CDECL
jm_aligned_usable_size(void * ptr, size_t alignment)
{
    size_t header_size;     /* Size of the header block */
    size_t footer_size;     /* Size of the footer block */
    size_t total_size;      /* total size of the allocated block */
    size_t user_size;       /* size of the user block */

    /* HEADER_SIZE + FOOTER_SIZE = (ALIGNMENT - 1) + SIZE_OF_A_POINTER(pBlockHdr->pvAlloc) */
    /* HEADER_SIZE + USER_SIZE + FOOTER_SIZE = TOTAL_SIZE */
    assert(ptr != nullptr);

    /* points to the beginning of the allocated block */
    aligned_block_header_t * pBlockHdr;
    pBlockHdr = (aligned_block_header_t *)((uintptr_t)ptr & ~(sizeof(uintptr_t) - 1)) - 1;

#ifdef _MSC_VER
    total_size = _msize(pBlockHdr->pvAlloc);
#else
    total_size = malloc_usable_size(pBlockHdr->pvAlloc);
#endif

    header_size = (uintptr_t)ptr - (uintptr_t)(pBlockHdr->pvAlloc);
    /* The alignment cannot be smaller than the sizeof(ptrdiff_t) */
    alignment = (alignment > sizeof(uintptr_t) ? alignment : sizeof(uintptr_t));
    footer_size = sizeof(aligned_block_header_t) + (alignment - 1) - header_size;

    user_size = total_size - header_size - footer_size;
    return user_size;
}

static JM_MALLOC_INLINE
size_t JM_X86_CDECL
jm_aligned_offset_usable_size(void * ptr, size_t alignment, size_t offset)
{
    size_t header_size;         /* Size of the header block */
    size_t footer_size;         /* Size of the footer block */
    size_t total_size;          /* total size of the allocated block */
    size_t user_size;           /* size of the user block */
    uintptr_t uintptr_offset;   /* keep the alignment of the data block */
                                /* after the sizeof(void *) aligned pointer */
                                /* to the beginning of the allocated block */

    /* HEADER_SIZE + FOOTER_SIZE = uintptr_offset + ALIGNMENT + SIZE_OF_A_POINTER(pBlockHdr->pvAlloc) */
    /* HEADER_SIZE + USER_SIZE + FOOTER_SIZE = TOTAL_SIZE */
    assert(ptr != nullptr);

    /* points to the beginning of the allocated block */
    aligned_block_header_t * pBlockHdr;
    pBlockHdr = (aligned_block_header_t *)((uintptr_t)ptr & ~(sizeof(uintptr_t) - 1)) - 1;

#ifdef _MSC_VER
    total_size = _msize(pBlockHdr->pvAlloc);
#else
    total_size = malloc_usable_size(pBlockHdr->pvAlloc);
#endif

    header_size = (uintptr_t)ptr - (uintptr_t)(pBlockHdr->pvAlloc);
    uintptr_offset = ((uintptr_t)0 - offset) & (sizeof(uintptr_t) - 1);
    /* The alignment cannot be smaller than the sizeof(ptrdiff_t) */
    alignment = (alignment > sizeof(uintptr_t) ? alignment : sizeof(uintptr_t));
    footer_size = sizeof(aligned_block_header_t) + (alignment - 1) + uintptr_offset - header_size;

    user_size = total_size - header_size - footer_size;
    return user_size;
}

static JM_MALLOC_INLINE
void * JM_X86_CDECL
jm_aligned_malloc(size_t size, size_t alignment)
{
    size_t alloc_size;
    uintptr_t pvAlloc, pvData;
    aligned_block_header_t * pBlockHdr;
#ifndef NDEBUG
    ptrdiff_t nFrontPaddedSize;
    ptrdiff_t nLastPaddedSize;
#endif

    alignment = (alignment > sizeof(uintptr_t)) ? alignment : sizeof(uintptr_t);
    alignment = jm_adjust_alignment(alignment);
    assert(alignment > 0);
    assert(jm_is_power_of_2(alignment));

    // Let alloc_size aligned to alignment bytes (isn't must need)
    alloc_size = sizeof(aligned_block_header_t) + size + (alignment - 1);

    pvAlloc = (uintptr_t)malloc(alloc_size);
    if (pvAlloc != (uintptr_t)nullptr) {
        // The output data pointer aligned to alignment bytes
        pvData = (uintptr_t)((pvAlloc + sizeof(aligned_block_header_t) + (alignment - 1))
            & (~(alignment - 1)));

        pBlockHdr = (aligned_block_header_t *)(pvData) - 1;
        assert((uintptr_t)pBlockHdr >= pvAlloc);

#ifndef NDEBUG
        memset((void *)pBlockHdr->sign, kcAlignSignFill, JM_ALIGN_SIGN_SIZE);
#endif
        pBlockHdr->pvAlloc = (void *)pvAlloc;

#ifndef NDEBUG
        // For debug diagnose
        nFrontPaddedSize = (ptrdiff_t)pvData - (ptrdiff_t)pvAlloc;
        nLastPaddedSize  = (ptrdiff_t)alloc_size - (ptrdiff_t)size - nFrontPaddedSize;

        assert(nFrontPaddedSize >= (ptrdiff_t)sizeof(aligned_block_header_t));
        assert(nLastPaddedSize >= 0);
#endif
        return (void *)pvData;
    }
    else {
        // MSVC: If size bigger than _HEAP_MAXREQ, return ENOMEM.
        jm_malloc_errno = errno;
    }

    return nullptr;
}

static JM_MALLOC_INLINE
void * JM_X86_CDECL
jm_aligned_realloc(void * ptr, size_t new_size, size_t alignment)
{
    return nullptr;
}

static JM_MALLOC_INLINE
void * JM_X86_CDECL
jm_aligned_calloc(size_t count, size_t size, size_t alignment)
{
    return nullptr;
}

static JM_MALLOC_INLINE
void * JM_X86_CDECL
jm_aligned_recalloc(void *ptr, size_t count, size_t new_size, size_t alignment)
{
    return nullptr;
}

static JM_MALLOC_INLINE
void * JM_X86_CDECL
jm_aligned_offset_malloc(size_t size, size_t alignment, size_t offset)
{
    size_t alloc_size;
    uintptr_t pvAlloc, pvData, uintptr_offset;
    aligned_block_header_t * pBlockHdr;
#ifndef NDEBUG
    ptrdiff_t nFrontPaddedSize;
    ptrdiff_t nLastPaddedSize;
#endif

    if (offset >= size) {
        jm_malloc_errno = EINVAL;
        return nullptr;
    }

    alignment = (alignment > sizeof(uintptr_t)) ? alignment : sizeof(uintptr_t);
    alignment = jm_adjust_alignment(alignment);
    assert(alignment > 0);
    assert(jm_is_power_of_2(alignment));

    uintptr_offset = ((uintptr_t)0 - offset) & (sizeof(uintptr_t) - 1);

    // Let alloc_size aligned to alignment bytes (isn't must need)
    alloc_size = sizeof(aligned_block_header_t) + size + (alignment - 1) + uintptr_offset + offset;

    pvAlloc = (uintptr_t)malloc(alloc_size);
    if (pvAlloc != (uintptr_t)nullptr) {
        // The output data pointer aligned to alignment bytes
        pvData = (uintptr_t)(((pvAlloc + sizeof(aligned_block_header_t) + (alignment - 1)
            + uintptr_offset + offset) & (~(alignment - 1))) - offset);

        pBlockHdr = (aligned_block_header_t *)(pvData - uintptr_offset) - 1;
        assert((uintptr_t)pBlockHdr >= pvAlloc);

#ifndef NDEBUG
        memset((void *)pBlockHdr->sign, kcAlignSignFill, JM_ALIGN_SIGN_SIZE);
#endif
        pBlockHdr->pvAlloc = (void *)pvAlloc;

#ifndef NDEBUG
        // For debug diagnose
        nFrontPaddedSize = (ptrdiff_t)pvData - (ptrdiff_t)pvAlloc;
        nLastPaddedSize  = (ptrdiff_t)alloc_size - (ptrdiff_t)size - nFrontPaddedSize;

        assert(nFrontPaddedSize >= (ptrdiff_t)sizeof(aligned_block_header_t));
        assert(nLastPaddedSize >= 0);
#endif
        return (void *)pvData;
    }
    else {
        // MSVC: If size bigger than _HEAP_MAXREQ, return ENOMEM.
        jm_malloc_errno = errno;
    }

    return nullptr;
}

static JM_MALLOC_INLINE
void * JM_X86_CDECL
jm_aligned_offset_realloc(void * ptr, size_t new_size, size_t alignment)
{
    return nullptr;
}

static JM_MALLOC_INLINE
void * JM_X86_CDECL
jm_aligned_offset_calloc(size_t count, size_t size, size_t alignment)
{
    return nullptr;
}

static JM_MALLOC_INLINE
void * JM_X86_CDECL
jm_aligned_offset_recalloc(void *ptr, size_t count, size_t new_size, size_t alignment)
{
    return nullptr;
}

/////////////////////////////////////////////////////////////////////////////

#endif // JSTD_C_ALIGNED_MALLOC_H
