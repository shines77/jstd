
#ifndef JSTD_CONFIG_CONFIG_H
#define JSTD_CONFIG_CONFIG_H

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

// Minimum requirements: gcc/clang C++ 11 or MSVC 2015 Update 3.
#if (!defined(_MSC_VER) && defined(__cplusplus) && (__cplusplus < 201103L)) \
 || (defined(_MSVC_LANG) && (_MSVC_LANG < 201103L)) \
 || (defined(_MSC_VER) && (_MSC_FULL_VER < 190024210))
#error "jstd requires C++11 support."
#endif

/// \macro __GNUC_PREREQ
/// \brief Defines __GNUC_PREREQ if glibc's features.h isn't available.
#ifndef __GNUC_PREREQ
  #if defined(__GNUC__) && defined(__GNUC_MINOR__)
    #define __GNUC_PREREQ(major, minor) \
        (((__GNUC__ << 16) + __GNUC_MINOR__) >= (((major) << 16) + (minor)))
  #else
    #define __GNUC_PREREQ(major, minor)     0
  #endif
#endif

#ifndef __CLANG_PREREQ
  #if defined(__clang__) && defined(__clang_major__) && defined(__clang_minor__)
    #define __CLANG_PREREQ(major, minor) \
        (((__clang_major__ << 16) + __clang_minor__) >= (((major) << 16) + (minor)))
  #else
    #define __CLANG_PREREQ(major, minor)    0
  #endif
#endif

#if defined(_M_X64) || defined(_M_AMD64) \
 || defined(_M_IA64) || defined(__amd64__) || defined(__x86_64__)
  #define JSTD_IS_X86           1
  #define JSTD_IS_X86_64        1
  #define JSTD_WORD_LEN         64
#elif defined (_M_IX86) || defined(__i386__)
  #define JSTD_IS_X86           1
  #define JSTD_IS_X86_I386      1
  #define JSTD_WORD_LEN         32
#elif defined(__aarch64__) || defined(_M_ARM64) || defined(__ARM64__) || defined(__arm64__)
  #define JSTD_IS_ARM           1
  #define JSTD_IS_ARM_64        1
  #define JSTD_WORD_LEN         64
#elif defined(__aarch32__) || defined(_M_ARM) || defined(__ARM__) || defined(__arm__)
  #define JSTD_IS_ARM           1
  #define JSTD_IS_ARM_32        1
  #define JSTD_WORD_LEN         32
#elif defined(_M_MPPC)
  // Power Macintosh PowerPC
  #define JSTD_WORD_LEN         32
#elif defined(_M_PPC)
  // PowerPC
  #define JSTD_WORD_LEN         32
#endif

#ifndef JSTD_WORD_LEN
  #if defined(WIN32) || defined(_WIN32)
    #define JSTD_WORD_LEN       32
  #elif defined(WIN64) || defined(_WIN64)
    #define JSTD_WORD_LEN       64
  #endif
#endif // !JSTD_WORD_LEN

//
// What compiler is it?
//
#if defined(__INTEL_COMPILER) || defined(__ICL) || defined(__ICC) || defined(__ECC) \
 || defined(__ICPC) || defined(__ECL)
  #ifndef JSTD_IS_ICC
  #define JSTD_IS_ICC     1
  #endif
#endif

#if defined(_MSC_VER) && !defined(__clang__)
  #ifndef JSTD_IS_MSVC
  #define JSTD_IS_MSVC    1
  #endif
#elif defined(__GNUC__) && !defined(__clang__)
  #ifndef JSTD_IS_GCC
  #define JSTD_IS_GCC     1
  #endif
#elif defined(__clang__)
  #ifndef JSTD_IS_CLANG
  #define JSTD_IS_CLANG   1
  #endif
#elif !defined(JSTD_IS_ICC)
  #ifndef JSTD_IS_UNKNOWN_COMPILER
  #define JSTD_IS_UNKNOWN_COMPILER   1
  #endif
#endif // _MSC_VER

#if defined(__GNUC__) || defined(__clang__) || (defined(JSTD_IS_ICC) && defined(__linux__))
#define JSTD_GCC_STYLE_ASM  1
#endif

#if defined(__GNUC__) && (!defined(__clang__) && !defined(JSTD_IS_ICC))
#define JSTD_IS_PURE_GCC    1
#endif

//
// Intel C++ compiler version
//
#if defined(__INTEL_COMPILER)
  #if (__INTEL_COMPILER == 9999)
    #define __INTEL_CXX_VERSION     1200    // Intel's bug in 12.1.
  #else
    #define __INTEL_CXX_VERSION     __INTEL_COMPILER
  #endif
#elif defined(__ICL)
#  define __INTEL_CXX_VERSION       __ICL
#elif defined(__ICC)
#  define __INTEL_CXX_VERSION       __ICC
#elif defined(__ECC)
#  define __INTEL_CXX_VERSION       __ECC
#endif

//
// [Visual Studio version] ==> _MSC_VER
// https://docs.microsoft.com/en-us/cpp/preprocessor/predefined-macros?view=msvc-170
//
// 1. MSVC
//
// default: __cplusplus = 199711L, /Zc:__cplusplus
//
// C++11: from Visual Studio 2015 Update 3:
// _MSC_FULL_VER = 190024210, _MSVC_LANG = 201402L
// or __cplusplus >= 201103L
//
// C++14: _MSVC_LANG = 201402L, /std:c++14
// C++17: _MSVC_LANG = 201703L, /std:c++17
// C++20: _MSVC_LANG = 202002L, /std:c++20
//
// _MSVC_LANG: Since Visual Studio 2015 Update 3
//
// Visual Studio 2015 Update 3: _MSC_FULL_VER = 190024210, _MSVC_LANG = 201402L
//
// 2. gcc and clang
//
// C++11: __cplusplus >= 201103L
// C++14: __cplusplus >= 201402L
// C++17: __cplusplus >= 201703L,
//
// GCC 9.0.0: 201709L (C++ 2a), Clang 8.0.0: 201707L, VC++ 15.9.3: 201704L
//
// C++20: __cplusplus >= 202002L
//
// GCC 11.0.0, Clang 10.0.0, VC++ 19.29, ICX: 2021, ICC
//

#if defined(_MSC_VER) && !defined(__clang__)
  #if (defined(_MSVC_LANG) && (_MSVC_LANG >= 202002L)) || (__cplusplus >= 202002L)
    #ifndef JSTD_IS_CXX_20
    #define JSTD_IS_CXX_20  1
    #endif
  #elif (defined(_MSVC_LANG) && (_MSVC_LANG >= 201703L && _MSVC_LANG < 202002L)) \
     || (__cplusplus >= 201703L && __cplusplus < 202002L)
    #ifndef JSTD_IS_CXX_17
    #define JSTD_IS_CXX_17  1
    #endif
  #elif (defined(_MSVC_LANG) && (_MSVC_LANG >= 201402L && _MSVC_LANG < 201703L \
     && _MSC_VER >= 1910)) \
     || (__cplusplus >= 201402L && __cplusplus < 201703L && _MSC_VER >= 1910)
    #ifndef JSTD_IS_CXX_14
    #define JSTD_IS_CXX_14  1
    #endif
  #elif defined(_MSC_VER) && (_MSC_FULL_VER >= 190024210) \
     || (__cplusplus >= 201103L)
    #ifndef JSTD_IS_CXX_11
    #define JSTD_IS_CXX_11  1
    #endif
  #else
    #ifndef JSTD_IS_CXX_98
    #define JSTD_IS_CXX_98  1
    #endif
  #endif
#elif defined(__GNUC__) || defined(__clang__)
  #if (__cplusplus >= 202002L)
    #ifndef JSTD_IS_CXX_20
    #define JSTD_IS_CXX_20  1
    #endif
  #elif (__cplusplus >= 201703L && __cplusplus < 202002L)
    #ifndef JSTD_IS_CXX_17
    #define JSTD_IS_CXX_17  1
    #endif
  #elif (__cplusplus >= 201402L && __cplusplus < 201703L)
    #ifndef JSTD_IS_CXX_14
    #define JSTD_IS_CXX_14  1
    #endif
  #elif (__cplusplus >= 201103L && __cplusplus < 201402L)
    #ifndef JSTD_IS_CXX_11
    #define JSTD_IS_CXX_11  1
    #endif
  #else
    #ifndef JSTD_IS_CXX_98
    #define JSTD_IS_CXX_98  1
    #endif
  #endif
#endif // _MSC_VER && !__clang__

////////////////////////////////////////////////////////////////////////////////
//
// Clang Language Extensions
//
// See: http://clang.llvm.org/docs/LanguageExtensions.html#checking_language_features
//
////////////////////////////////////////////////////////////////////////////////

//
// Feature testing (C++20)
// See: https://en.cppreference.com/w/cpp/feature_test
//
#ifndef __has_feature                               // Optional of course.
  #define __has_feature(x)              0           // Compatibility with non-clang compilers.
#endif

#ifndef __has_builtin                               // Optional of course.
  #define __has_builtin(x)              0           // Compatibility with non-clang compilers.
#endif

#ifndef __has_extension
  #define __has_extension               __has_feature   // Compatibility with pre-3.0 compilers.
#endif

#ifndef __has_cpp_attribute                         // Optional of course.
  #define __has_cpp_attribute(x)        0           // Compatibility with non-clang compilers.
#endif

#ifndef __has_c_attribute                           // Optional of course.
  #define __has_c_attribute(x)          0           // Compatibility with non-clang compilers.
#endif

#ifndef __has_attribute                             // Optional of course.
  #define __has_attribute(x)            0           // Compatibility with non-clang compilers.
#endif

#ifndef __has_declspec_attribute                    // Optional of course.
  #define __has_declspec_attribute(x)   0           // Compatibility with non-clang compilers.
#endif

#ifndef __is_identifier                             // Optional of course.
  // It evaluates to 1 if the argument x is just a regular identifier and not a reserved keyword.
  #define __is_identifier(x)            1           // Compatibility with non-clang compilers.
#endif

// Since C++ 17
#ifndef __has_include
  #define __has_include(x)              0
#endif

#if defined(_MSC_VER) && !defined(__clang__)
#ifndef __attribute__
  #define __attribute__(x)
#endif
#endif

////////////////////////////////////////////////////////////////////////////////

#if __has_feature(cxx_rvalue_references)
    // This code will only be compiled with the -std=c++11 and -std=gnu++11
    // options, because rvalue references are only standardized in C++11.
#endif

#if __has_extension(cxx_rvalue_references)
    // This code will be compiled with the -std=c++11, -std=gnu++11, -std=c++98
    // and -std=gnu++98 options, because rvalue references are supported as a
    // language extension in C++98.
#endif

////////////////////////////////////////////////////////////////////////////////

#if (__has_cpp_attribute(noreturn) >= 200809L) || defined(JSTD_IS_CXX_11)
#ifndef JSTD_NORETURN
#define JSTD_NORETURN   [[noreturn]]
#endif
#endif

#if (__has_cpp_attribute(deprecated) >= 201309L) || defined(JSTD_IS_CXX_14)
#ifndef JSTD_DEPRECATED
#define JSTD_DEPRECATED [[deprecated]]
#endif
#endif

#if (__has_cpp_attribute(maybe_unused) >= 201603L) || defined(JSTD_IS_CXX_17)
#ifndef JSTD_MAYBE_UNUSED
#define JSTD_MAYBE_UNUSED  [[maybe_unused]]
#endif
#endif

//
// See: https://blog.csdn.net/qq_38617319/article/details/115099855
//
#if (__has_cpp_attribute(nodiscard) >= 201603L) || defined(JSTD_IS_CXX_17)
#ifndef JSTD_NODISCARD
#define JSTD_NODISCARD  [[nodiscard]]
#endif
#endif

#if (__has_cpp_attribute(fallthrough) >= 201603L) || defined(JSTD_IS_CXX_17)
#ifndef JSTD_FALLTHROUGH
#define JSTD_FALLTHROUGH  [[fallthrough]]
#endif
#endif

#if (__has_cpp_attribute(likely) >= 201803L) || defined(JSTD_IS_CXX_20)
#ifndef JSTD_LIKELY
#define JSTD_LIKELY     [[likely]]
#endif
#endif

#if (__has_cpp_attribute(unlikely) >= 201803L) || defined(JSTD_IS_CXX_20)
#ifndef JSTD_UNLIKELY
#define JSTD_UNLIKELY   [[unlikely]]
#endif
#endif

#if (__has_cpp_attribute(no_unique_address) >= 201803L) || defined(JSTD_IS_CXX_20)
#ifndef JSTD_NO_UNIQUE_ADDRESS
#define JSTD_NO_UNIQUE_ADDRESS  [[no_unique_address]]
#endif
#endif

////////////////////////////////////////////////////////////////////////////////

#if __has_builtin(__builtin_addressof) || \
    (defined(__GNUC__) && (__GNUC__ >= 7)) || defined(_MSC_VER)
#define JSTD_BUILTIN_ADDRESSOF
#endif

#if defined(__cpp_constexpr) && (__cpp_constexpr >= 200704L) && \
    !(defined(__GNUC__) && (__GNUC__ == 4) && (__GNUC_MINOR__ == 9))
#define JSTD_CPP11_CONSTEXPR
#endif

#if defined(__cpp_constexpr) && (__cpp_constexpr >= 201304L)
#define JSTD_CPP14_CONSTEXPR
#endif

#if __has_builtin(__builtin_unreachable) || defined(__GNUC__)
#define JSTD_BUILTIN_UNREACHABLE    __builtin_unreachable()
#elif defined(_MSC_VER)
#define JSTD_BUILTIN_UNREACHABLE    __assume(false)
#else
#define JSTD_BUILTIN_UNREACHABLE
#endif

#if __has_feature(cxx_exceptions) || defined(__cpp_exceptions) || \
    (defined(_MSC_VER) && defined(_CPPUNWIND)) || \
    defined(__EXCEPTIONS)
#define JSTD_EXCEPTIONS
#endif

#if defined(__cpp_generic_lambdas) || defined(_MSC_VER)
#define JSTD_GENERIC_LAMBDAS
#endif

#if defined(__cpp_lib_integer_sequence)
#define JSTD_INTEGER_SEQUENCE
#endif

#if (defined(__cpp_decltype_auto) && defined(__cpp_return_type_deduction)) || defined(_MSC_VER)
#define JSTD_RETURN_TYPE_DEDUCTION
#endif

#if defined(__cpp_lib_transparent_operators) || defined(_MSC_VER)
#define JSTD_TRANSPARENT_OPERATORS
#endif

#if defined(__cpp_variable_templates) || defined(_MSC_VER)
#define JSTD_VARIABLE_TEMPLATES
#endif

#if !defined(__GLIBCXX__) || __has_include(<codecvt>)  // >= libstdc++-5
#define JSTD_TRIVIALITY_TYPE_TRAITS
#define JSTD_INCOMPLETE_TYPE_TRAITS
#endif

////////////////////////////////////////////////////////////////////////////////

//
// C++ 17: std::launder()
//
// See: https://stackoverflow.com/questions/39382501/what-is-the-purpose-of-stdlaunder
// See: https://wanghenshui.github.io/2019/04/27/launder.html
// See: https://en.cppreference.com/w/cpp/utility/launder
//

//
// msvc: since VC 2017 in version 15.7.0
// gcc: >= 7.0
// clang:
//
// Note: libc++ 6+ adds std::launder but does not define __cpp_lib_launder
//
#if (defined(__cpp_lib_launder) && (__cpp_lib_launder >= 201606L)) || \
    (defined(_MSC_VER) && (_HAS_LAUNDER != 0 || _MSC_VER >= 1914)) || \
    ((_LIBCPP_VERSION >= (__ANDROID__ ? 7000 : 6000)) && (__cplusplus >= 201703L))
#define JSTD_STD_LAUNDER
#endif

// __builtin_launder
#if __has_builtin(__builtin_launder) || (__GNUC__ >= 7)
#define JSTD_BUILTIN_LAUNDER
#endif

////////////////////////////////////////////////////////////////////////////////

#endif // JSTD_CONFIG_CONFIG_H
