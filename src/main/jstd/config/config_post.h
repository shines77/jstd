
#ifndef JSTD_CONFIG_CONFIG_POST_H
#define JSTD_CONFIG_CONFIG_POST_H

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

#if defined(_DEBUG) || !defined(NDEBUG)
#define JSTD_IS_DEBUG                       1
#define JSTD_USE_ASSERT                     1
#else
#define JSTD_IS_DEBUG                       0
#define JSTD_USE_ASSERT                     0
#endif

#ifndef _MSC_VER
#undef  JSTD_USE_CRTDBG_CHECK
#undef  JSTD_MSVC_CLANG
#else
#ifndef JSTD_USE_CRTDBG_CHECK
#define JSTD_USE_CRTDBG_CHECK               1
#endif
/* The compiler is clang-cl.exe for Visual Studio 20xx ? */
#ifndef JSTD_MSVC_CLANG
#define JSTD_MSVC_CLANG                     0
#endif
#endif // _MSC_VER

/* Little endian or big endian, 0 is little endian. */
#if (JSTD_BYTE_ORDER == JSTD_BIG_ENDIAN)
#define JSTD_IS_LITTLE_ENDIAN               0
#else
#define JSTD_IS_LITTLE_ENDIAN               1
#endif

#if defined(__linux__) || defined(__LINUX__)
//
#elif defined(__MINGW__) || defined(__MINGW32__) || defined(__MINGW64__)
//
#elif defined(__CYGWIN__)
//
#elif defined(_WIN32) || defined(_MSC_VER) || defined(__ICL) || defined(__INTEL_COMPILER)
//
#else
//
#endif

#endif // JSTD_CONFIG_CONFIG_POST_H
