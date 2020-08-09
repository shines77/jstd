
#ifndef JSTD_CONFIG_DEFAULT_CONFIG_H
#define JSTD_CONFIG_DEFAULT_CONFIG_H

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

#if !defined(JIMIC_USE_DEFAULT_CONFIG) || (JIMIC_USE_DEFAULT_CONFIG == 0)

#define JSTD_USE_DEFAULT_CONFIG             1

/* Use Visual Leaker Dector for Visual Studio ? */
#ifndef JSTD_ENABLE_VLD
#define JSTD_ENABLE_VLD              		0
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

#endif // JSTD_USE_DEFAULT_CONFIG

#endif // JSTD_CONFIG_DEFAULT_CONFIG_H
