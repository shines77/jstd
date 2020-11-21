/* Copyright (C) 2008, 2009 Free Software Foundation, Inc.

   This file is part of GCC.

   GCC is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3, or (at your option)
   any later version.

   GCC is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   Under Section 7 of GPL version 3, you are granted additional
   permissions described in the GCC Runtime Library Exception, version
   3.1, as published by the Free Software Foundation.

   You should have received a copy of the GNU General Public License and
   a copy of the GCC Runtime Library Exception along with this program;
   see the files COPYING3 and COPYING.RUNTIME respectively.  If not, see
   <http://www.gnu.org/licenses/>.  */

#ifndef JSTD_SUPPORT_X86_INTRIN_H
#define JSTD_SUPPORT_X86_INTRIN_H

//
// See: https://sites.uclouvain.be/SystInfo/usr/include/x86intrin.h.html
//

#ifdef __MMX__
#include <mmintrin.h>
#endif

#ifdef __SSE__
#include <xmmintrin.h>
#endif

#ifdef __SSE2__
#include <emmintrin.h>
#endif

#ifdef __SSE3__
#include <pmmintrin.h>
#endif

#ifdef __SSSE3__
#include <tmmintrin.h>
#endif

#if defined(__SSE4A__) || defined(__SSE4a__)
#include <ammintrin.h>
#endif

#ifdef __SSE4_1__
#include <smmintrin.h>
#endif

#ifdef __SSE4_2__
#include <nmmintrin.h>
#endif

#ifdef __SSE5__
//#include <bmmintrin.h>
#endif

#if defined(__AES__) || defined(__PCLMUL__)
/* For AES && PCLMULQDQ */
#include <wmmintrin.h>
#endif

#if defined(__AVX__) || defined(__AVX2__)
/* For including AVX instructions */
#include <immintrin.h>
#endif

#ifdef __3dNOW__
#include <mm3dnow.h>
#endif

#ifdef __FMA4__
//#include <fma4intrin.h>
#endif

#if defined(__GNUC__) || defined(__llvm__)

#ifdef __F16C__
  #ifndef INCLUDE_X86INTRIN
    #define INCLUDE_X86INTRIN     1
  #elif (INCLUDE_X86INTRIN == 0)
    #undef  INCLUDE_X86INTRIN
    #define INCLUDE_X86INTRIN     1
  #endif
#endif

#ifdef __FMA__
  #ifndef INCLUDE_X86INTRIN
    #define INCLUDE_X86INTRIN     1
  #elif (INCLUDE_X86INTRIN == 0)
    #undef  INCLUDE_X86INTRIN
    #define INCLUDE_X86INTRIN     1
  #endif
#endif

#ifdef __FMA4__
  #ifndef INCLUDE_X86INTRIN
    #define INCLUDE_X86INTRIN     1
  #elif (INCLUDE_X86INTRIN == 0)
    #undef  INCLUDE_X86INTRIN
    #define INCLUDE_X86INTRIN     1
  #endif
#endif

#ifdef __XOP__
#include <xopintrin.h>
#endif

#ifdef __LWP__
  #ifndef INCLUDE_X86INTRIN
    #define INCLUDE_X86INTRIN     1
  #elif (INCLUDE_X86INTRIN == 0)
    #undef  INCLUDE_X86INTRIN
    #define INCLUDE_X86INTRIN     1
  #endif
#endif

#ifdef __RDRND__
  #ifndef INCLUDE_X86INTRIN
    #define INCLUDE_X86INTRIN     1
  #elif (INCLUDE_X86INTRIN == 0)
    #undef  INCLUDE_X86INTRIN
    #define INCLUDE_X86INTRIN     1
  #endif
#endif

#ifdef __FSGSBASE__
  #ifndef INCLUDE_X86INTRIN
    #define INCLUDE_X86INTRIN     1
  #elif (INCLUDE_X86INTRIN == 0)
    #undef  INCLUDE_X86INTRIN
    #define INCLUDE_X86INTRIN     1
  #endif
#endif

#ifdef __POPCNT__
    #include <popcntintrin.h>
#endif

#ifdef __LZCNT__
  #ifndef INCLUDE_X86INTRIN
    #define INCLUDE_X86INTRIN     1
  #elif (INCLUDE_X86INTRIN == 0)
    #undef  INCLUDE_X86INTRIN
    #define INCLUDE_X86INTRIN     1
  #endif
#endif

#ifdef __TBM__
  #ifndef INCLUDE_X86INTRIN
    #define INCLUDE_X86INTRIN     1
  #elif (INCLUDE_X86INTRIN == 0)
    #undef  INCLUDE_X86INTRIN
    #define INCLUDE_X86INTRIN     1
  #endif
#endif

#ifdef __BMI__
  #ifndef INCLUDE_X86INTRIN
    #define INCLUDE_X86INTRIN     1
  #elif (INCLUDE_X86INTRIN == 0)
    #undef  INCLUDE_X86INTRIN
    #define INCLUDE_X86INTRIN     1
  #endif
#endif

#ifdef __BMI2__
  #ifndef INCLUDE_X86INTRIN
    #define INCLUDE_X86INTRIN     1
  #elif (INCLUDE_X86INTRIN == 0)
    #undef  INCLUDE_X86INTRIN
    #define INCLUDE_X86INTRIN     1
  #endif
#endif

#if defined(INCLUDE_X86INTRIN) && (INCLUDE_X86INTRIN != 0)
    #include <x86intrin.h>
#endif

#endif // __GNUC__ || __llvm__

#undef INCLUDE_X86INTRIN

#endif // JSTD_SUPPORT_X86_INTRIN_H
