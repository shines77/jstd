
#ifndef JSTD_CONFIG_WIN32_CONFIG_H
#define JSTD_CONFIG_WIN32_CONFIG_H

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

#if defined(_MSC_VER) || defined(__ICL) || defined(__INTEL_COMPILER)

#define JSTD_USE_DEFAULT_CONFIG             0

/* Use Visual Leaker Dector for Visual Studio ? */
#ifndef JSTD_ENABLE_VLD
#define JSTD_ENABLE_VLD                   	1
#endif

#define JSTD_USE_THREADING_TOOLS            0
#define JSTD_MALLOC_BUILD                   0

#if defined(JSTD_MSVC_CLANG) && (JSTD_MSVC_CLANG != 0)
#define JSTD_ATTRIBUTE_ALIGNED_PRESENT                  1
#else
#define JSTD_DECLSPEC_ALIGN_PRESENT                     1
#endif

#define JSTD_ALIGNOF_NOT_INSTANTIATED_TYPES_BROKEN      1

#define JSTD_CONSTEXPR

#if defined(_MSC_VER) && (_MSC_VER < 1600)
#define JSTD_HAS_SHARED_PTR                 0
#else
#define JSTD_HAS_SHARED_PTR                 1
#endif  /* _MSC_VER */

#if defined(_MSC_VER) && (_MSC_VER < 1600)
#define JSTD_HAS_CXX11_TYPE_TRAITS          0
#else
#define JSTD_HAS_CXX11_TYPE_TRAITS          1
#endif  /* _MSC_VER */

#if defined(_MSC_VER) && (_MSC_VER < 1700)
#define JSTD_IS_CXX11                       0
#else
#define JSTD_IS_CXX11                       1
#endif  /* _MSC_VER */

//
// The variable length parameter template of C++ 11 standard is introduced
//
//  See: http://www.cnblogs.com/zenny-chen/archive/2013/02/03/2890917.html
//

/* Does the compiler support variable length parameter templates of C++ 11 ? */
#define JSTD_HAS_CXX11_VARIADIC_TEMPLATES   0

#define JSTD_HAS_DEFAULTED_FUNCTIONS        0
#define JSTD_HAS_DELETED_FUNCTIONS          0

#define JSTD_HAS_CXX11_MOVE_FUNCTIONS       0

#define JSTD_HAS_BOOST                      0
#define JSTD_HAS_BOOST_LOCALE               0

//
// See: http://msdn.microsoft.com/zh-cn/library/e5ewb1h3%28v=vs.90%29.aspx
// See: http://msdn.microsoft.com/en-us/library/x98tx3cf.aspx
//
#if defined(JSTD_USE_CRTDBG_CHECK) && (JSTD_USE_CRTDBG_CHECK != 0)
#if defined(_DEBUG) || !defined(NDEBUG)
#ifndef _CRTDBG_MAP_ALLOC
#define _CRTDBG_MAP_ALLOC
#endif
#endif /* _DEBUG */
#endif /* JSTD_USE_CRTDBG_CHECK */

#endif /* _MSC_VER || __ICL || __INTEL_COMPILER */

#endif // JSTD_CONFIG_WIN32_CONFIG_H
