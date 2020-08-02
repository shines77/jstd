##
## Author: Hank Anderson <hank@statease.com>
## Description: Ported from portion of OpenBLAS/Makefile.system
##              Sets various variables based on architecture.

if (X86 OR X86_64)

  if (X86)
    if (NOT BINARY)
      set(NO_BINARY_MODE 1)
    endif()
  endif()

  if (NOT NO_EXPRECISION)
    if (${F_COMPILER} MATCHES "GFORTRAN")
      # N.B. I'm not sure if CMake differentiates between GCC and LSB -hpa
      if (${CMAKE_C_COMPILER_ID} STREQUAL "GNU" OR ${CMAKE_C_COMPILER_ID} STREQUAL "LSB")
        set(EXPRECISION	1)
        set(CCOMMON_OPT "${CCOMMON_OPT} -DEXPRECISION -m128bit-long-double")
        set(FCOMMON_OPT	"${FCOMMON_OPT} -m128bit-long-double")
      endif()
      if (${CMAKE_C_COMPILER_ID} STREQUAL "Clang")
        set(EXPRECISION	1)
        set(CCOMMON_OPT "${CCOMMON_OPT} -DEXPRECISION")
        set(FCOMMON_OPT	"${FCOMMON_OPT} -m128bit-long-double")
      endif()
    endif()
  endif()
endif()

if (${CMAKE_C_COMPILER_ID} STREQUAL "Intel")
  set(CCOMMON_OPT "${CCOMMON_OPT} -wd981")
endif()

if (USE_OPENMP)
  # USE_SIMPLE_THREADED_LEVEL3 = 1
  # NO_AFFINITY = 1
  find_package(OpenMP REQUIRED)
  if (OpenMP_FOUND)
    set(CCOMMON_OPT "${CCOMMON_OPT} ${OpenMP_C_FLAGS} -DUSE_OPENMP")
    set(FCOMMON_OPT "${FCOMMON_OPT} ${OpenMP_Fortran_FLAGS}")
  endif()
endif()

if (${ARCH} STREQUAL "ia64")
  set(NO_BINARY_MODE 1)
  set(BINARY_DEFINED 1)

  if (${F_COMPILER} MATCHES "GFORTRAN")
    if (${CMAKE_C_COMPILER_ID} STREQUAL "GNU")
      # EXPRECISION	= 1
      # CCOMMON_OPT	+= -DEXPRECISION
    endif()
  endif()
endif()

if (MIPS64)
  set(NO_BINARY_MODE 1)
endif()

if (${ARCH} STREQUAL "alpha")
  set(NO_BINARY_MODE 1)
  set(BINARY_DEFINED 1)
endif()

if (ARM)
  set(NO_BINARY_MODE 1)
  set(BINARY_DEFINED 1)
endif()

if (ARM64)
  set(NO_BINARY_MODE 1)
  set(BINARY_DEFINED 1)
endif()
