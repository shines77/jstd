
#ifndef JSTD_CONFIG_MACOS_CONFIG_H
#define JSTD_CONFIG_MACOS_CONFIG_H

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

#if defined(__apple__) || defined(__APPLE__)

#define JSTD_USE_DEFAULT_CONFIG             0

/* Use Visual Leaker Dector for Visual Studio ? */
#ifndef JSTD_ENABLE_VLD
#define JSTD_ENABLE_VLD                   	0
#endif

#define JSTD_USE_THREADING_TOOLS            0
#define JSTD_MALLOC_BUILD                   0

#define JSTD_ATTRIBUTE_ALIGNED_PRESENT                  1
//#define JSTD_DECLSPEC_ALIGN_PRESENT                   0
#define JSTD_ALIGNOF_NOT_INSTANTIATED_TYPES_BROKEN      1

#define JSTD_CONSTEXPR

//
// The variable length parameter template of C++ 11 standard is introduced
//
//  See: http://www.cnblogs.com/zenny-chen/archive/2013/02/03/2890917.html
//

/* Does the compiler support variable length parameter templates of C++ 11 ? */
#define JSTD_HAS_CXX11_VARIADIC_TEMPLATES   1

#define JSTD_HAS_DEFAULTED_FUNCTIONS        0
#define JSTD_HAS_DELETED_FUNCTIONS          0

#define JSTD_HAS_CXX11_MOVE_FUNCTIONS       1

#define JSTD_HAS_BOOST                      0
#define JSTD_HAS_BOOST_LOCALE               0

#endif /* __apple__ || __APPLE__ */

#endif // JSTD_CONFIG_MACOS_CONFIG_H
