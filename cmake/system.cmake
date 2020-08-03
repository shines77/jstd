##
## Author: Hank Anderson <hank@statease.com>
## Description: Ported from jstd/Makefile.system
##

# System detection, via CMake.
include("${PROJECT_SOURCE_DIR}/cmake/system_check.cmake")

if(CMAKE_CROSSCOMPILING AND NOT DEFINED TARGET)
  # Detect target without running getarch
  if (ARM64)
    set(TARGET "ARMV8")
  elseif(ARM)
    set(TARGET "ARMV7") # TODO: Ask compiler which arch this is
  else()
    message(FATAL_ERROR "When cross compiling, a TARGET is required.")
  endif()
endif()

# Other files expect CORE, which is actually TARGET and will become TARGET_CORE for kernel build. Confused yet?
# It seems we are meant to use TARGET as input and CORE internally as kernel.
if(NOT DEFINED CORE AND DEFINED TARGET)
  set(CORE ${TARGET})
endif()

# TARGET_CORE will override TARGET which is used in DYNAMIC_ARCH=1.
if (DEFINED TARGET_CORE)
  set(TARGET ${TARGET_CORE})
endif()

# Force fallbacks for 32bit
if (DEFINED BINARY AND DEFINED TARGET AND BINARY EQUAL 32)
  message(STATUS "Compiling a ${BINARY}-bit binary.")
  set(NO_AVX 1)
  if (${TARGET} STREQUAL "HASWELL" OR ${TARGET} STREQUAL "SANDYBRIDGE" OR ${TARGET} STREQUAL "SKYLAKEX")
    set(TARGET "NEHALEM")
  endif()
  if (${TARGET} STREQUAL "BULLDOZER" OR ${TARGET} STREQUAL "PILEDRIVER" OR ${TARGET} STREQUAL "ZEN")
    set(TARGET "BARCELONA")
  endif()
  if (${TARGET} STREQUAL "ARMV8" OR ${TARGET} STREQUAL "CORTEXA57" OR ${TARGET} STREQUAL "CORTEXA53")
    set(TARGET "ARMV7")
  endif()
endif()

if (DEFINED TARGET)
  if (${TARGET} STREQUAL "SKYLAKEX" AND NOT NO_AVX512)
    set (KERNEL_DEFINITIONS "${KERNEL_DEFINITIONS} -march=skylake-avx512")
  endif()
  if (${TARGET} STREQUAL "HASWELL" AND NOT NO_AVX2)
    if (${CMAKE_C_COMPILER_ID} STREQUAL "GNU")
      execute_process(COMMAND ${CMAKE_C_COMPILER} -dumpversion OUTPUT_VARIABLE GCC_VERSION)
      if (${GCC_VERSION} VERSION_GREATER 4.7 OR ${GCC_VERSION} VERSION_EQUAL 4.7)
        set (KERNEL_DEFINITIONS "${KERNEL_DEFINITIONS} -mavx2")
      endif()
    elseif (${CMAKE_C_COMPILER_ID} STREQUAL "CLANG")
      set (KERNEL_DEFINITIONS "${KERNEL_DEFINITIONS} -mavx2")
    endif()
  endif()
endif()

if (DEFINED TARGET)
  message(STATUS "Targeting the ${TARGET} architecture.")
  set(GETARCH_FLAGS "-DFORCE_${TARGET}")
endif()

# On x86_64 build getarch with march=native. This is required to detect AVX512 support in getarch.
if (X86_64 AND NOT ${CMAKE_C_COMPILER_ID} STREQUAL "PGI")
  set(GETARCH_FLAGS "${GETARCH_FLAGS} -march=native")
endif()

# On x86 no AVX support is available
if (X86 OR X86_64)
if ((DEFINED BINARY AND BINARY EQUAL 32) OR ("$CMAKE_SIZEOF_VOID_P}" EQUAL "4"))
  set(GETARCH_FLAGS "${GETARCH_FLAGS} -DNO_AVX -DNO_AVX2 -DNO_AVX512")
endif()
endif()

if (INTERFACE64)
  message(STATUS "Using 64-bit integers.")
  set(GETARCH_FLAGS	"${GETARCH_FLAGS} -DUSE64BITINT")
endif()

if (NOT DEFINED GEMM_MULTITHREAD_THRESHOLD)
  set(GEMM_MULTITHREAD_THRESHOLD 4)
endif()
message(STATUS "GEMM multithread threshold set to ${GEMM_MULTITHREAD_THRESHOLD}.")
set(GETARCH_FLAGS	"${GETARCH_FLAGS} -DGEMM_MULTITHREAD_THRESHOLD=${GEMM_MULTITHREAD_THRESHOLD}")

if (NO_AVX)
  message(STATUS "Disabling Advanced Vector Extensions (AVX).")
  set(GETARCH_FLAGS "${GETARCH_FLAGS} -DNO_AVX")
endif()

if (NO_AVX2)
  message(STATUS "Disabling Advanced Vector Extensions 2 (AVX2).")
  set(GETARCH_FLAGS "${GETARCH_FLAGS} -DNO_AVX2")
endif()

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
  set(GETARCH_FLAGS "${GETARCH_FLAGS} ${CMAKE_C_FLAGS_DEBUG}")
endif()

if (NOT DEFINED NO_PARALLEL_MAKE)
  set(NO_PARALLEL_MAKE 0)
endif()
set(GETARCH_FLAGS	"${GETARCH_FLAGS} -DNO_PARALLEL_MAKE=${NO_PARALLEL_MAKE}")

if (CMAKE_C_COMPILER STREQUAL loongcc)
  set(GETARCH_FLAGS	"${GETARCH_FLAGS} -static")
endif()

#if don't use Fortran, it will only compile CBLAS.
if (ONLY_CBLAS)
  set(NO_LAPACK 1)
else ()
  set(ONLY_CBLAS 0)
endif()

# N.B. this is NUM_THREAD in Makefile.system which is probably a bug -hpa
if (NOT CMAKE_CROSSCOMPILING)
  if (NOT DEFINED NUM_CORES)
    include(ProcessorCount)
    ProcessorCount(NUM_CORES)
  endif()
endif()

if (NOT DEFINED NUM_PARALLEL)
  set(NUM_PARALLEL 1)
endif()

if (NOT DEFINED NUM_THREADS)
  if (DEFINED NUM_CORES AND NOT NUM_CORES EQUAL 0)
    # HT?
    set(NUM_THREADS ${NUM_CORES})
  else ()
    set(NUM_THREADS 0)
  endif()
endif()

if (${NUM_THREADS} LESS 2)
  set(USE_THREAD 0)
elseif(NOT DEFINED USE_THREAD)
  set(USE_THREAD 1)
endif()

if (USE_THREAD)
  message(STATUS "Multi-threading enabled with ${NUM_THREADS} threads.")
else()
  if (${USE_LOCKING})
    set(CCOMMON_OPT "${CCOMMON_OPT} -DUSE_LOCKING")
  endif()
endif()

include("${PROJECT_SOURCE_DIR}/cmake/prebuild.cmake")
if (DEFINED BINARY)
  message(STATUS "Compiling a ${BINARY}-bit binary.")
endif()
if (NOT DEFINED NEED_PIC)
  set(NEED_PIC 1)
endif()

# OS dependent settings
include("${PROJECT_SOURCE_DIR}/cmake/os.cmake")

# Architecture dependent settings
include("${PROJECT_SOURCE_DIR}/cmake/arch.cmake")

# C Compiler dependent settings
include("${PROJECT_SOURCE_DIR}/cmake/cc.cmake")

if (BINARY64)
  if (INTERFACE64)
    # CCOMMON_OPT += -DUSE64BITINT
  endif()
endif()

if (NEED_PIC)
  if (${CMAKE_C_COMPILER} STREQUAL "IBM")
    set(CCOMMON_OPT "${CCOMMON_OPT} -qpic=large")
  else ()
    set(CCOMMON_OPT "${CCOMMON_OPT} -fPIC")
  endif()
endif()

if (NO_AVX)
  set(CCOMMON_OPT "${CCOMMON_OPT} -DNO_AVX")
endif()

if (X86)
  set(CCOMMON_OPT "${CCOMMON_OPT} -DNO_AVX")
endif()

if (NO_AVX2)
  set(CCOMMON_OPT "${CCOMMON_OPT} -DNO_AVX2")
endif()

if (USE_THREAD)
  # USE_SIMPLE_THREADED_LEVEL3 = 1
  # NO_AFFINITY = 1
  set(CCOMMON_OPT "${CCOMMON_OPT} -DSMP_SERVER")

  if (MIPS64)
    if (NOT ${CORE} STREQUAL "LOONGSON3B")
      set(USE_SIMPLE_THREADED_LEVEL3 1)
    endif()
  endif()

  if (BIGNUMA)
    set(CCOMMON_OPT "${CCOMMON_OPT} -DBIGNUMA")
  endif()
endif()

if (NO_WARMUP)
  set(CCOMMON_OPT "${CCOMMON_OPT} -DNO_WARMUP")
endif()

if (CONSISTENT_FPCSR)
  set(CCOMMON_OPT "${CCOMMON_OPT} -DCONSISTENT_FPCSR")
endif()

if (USE_TLS)
  set(CCOMMON_OPT "${CCOMMON_OPT} -DUSE_TLS")
endif()

# Only for development
# set(CCOMMON_OPT "${CCOMMON_OPT} -DPARAMTEST")
# set(CCOMMON_OPT "${CCOMMON_OPT} -DPREFETCHTEST")
# set(CCOMMON_OPT "${CCOMMON_OPT} -DNO_SWITCHING")
# set(USE_PAPI 1)

if (USE_PAPI)
  set(CCOMMON_OPT "${CCOMMON_OPT} -DUSE_PAPI")
  set(EXTRALIB "${EXTRALIB} -lpapi -lperfctr")
endif()

if (DYNAMIC_THREADS)
  set(CCOMMON_OPT "${CCOMMON_OPT} -DDYNAMIC_THREADS")
endif()

set(CCOMMON_OPT "${CCOMMON_OPT} -DMAX_CPU_NUMBER=${NUM_THREADS}")

set(CCOMMON_OPT "${CCOMMON_OPT} -DMAX_PARALLEL_NUMBER=${NUM_PARALLEL}")

if (BUFFERSIZE)
  set(CCOMMON_OPT "${CCOMMON_OPT} -DBUFFERSIZE=${BUFFERSIZE}")
endif()

if (USE_SIMPLE_THREADED_LEVEL3)
  set(CCOMMON_OPT "${CCOMMON_OPT} -DUSE_SIMPLE_THREADED_LEVEL3")
endif()

if (NOT ${CMAKE_SYSTEM_NAME} STREQUAL "Windows")
    if (DEFINED MAX_STACK_ALLOC)
        if (NOT ${MAX_STACK_ALLOC} EQUAL 0)
            set(CCOMMON_OPT "${CCOMMON_OPT} -DMAX_STACK_ALLOC=${MAX_STACK_ALLOC}")
        endif()
    else ()
        set(CCOMMON_OPT "${CCOMMON_OPT} -DMAX_STACK_ALLOC=2048")
    endif()
endif()

if (DEFINED LIBNAMESUFFIX)
  set(LIBPREFIX "libjstd_${LIBNAMESUFFIX}")
else ()
  set(LIBPREFIX "libjstd")
endif()

if (NOT DEFINED SYMBOLPREFIX)
  set(SYMBOLPREFIX "")
endif()

if (NOT DEFINED SYMBOLSUFFIX)
  set(SYMBOLSUFFIX "")
endif()

# TODO: need to convert these Makefiles
# include ${PROJECT_SOURCE_DIR}/cmake/${ARCH}.cmake

if (${CORE} STREQUAL "PPC440")
  set(CCOMMON_OPT "${CCOMMON_OPT} -DALLOC_QALLOC")
endif()

if (${CORE} STREQUAL "PPC440FP2")
  set(STATIC_ALLOCATION 1)
endif()

if (NOT ${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
  set(NO_AFFINITY 1)
endif()

if (NOT X86_64 AND NOT X86 AND NOT ${CORE} STREQUAL "LOONGSON3B")
  set(NO_AFFINITY 1)
endif()

if (NO_AFFINITY)
  set(CCOMMON_OPT "${CCOMMON_OPT} -DNO_AFFINITY")
endif()

if (FUNCTION_PROFILE)
  set(CCOMMON_OPT "${CCOMMON_OPT} -DFUNCTION_PROFILE")
endif()

if (HUGETLB_ALLOCATION)
  set(CCOMMON_OPT "${CCOMMON_OPT} -DALLOC_HUGETLB")
endif()

if (DEFINED HUGETLBFILE_ALLOCATION)
  set(CCOMMON_OPT "${CCOMMON_OPT} -DALLOC_HUGETLBFILE -DHUGETLB_FILE_NAME=${HUGETLBFILE_ALLOCATION})")
endif()

if (STATIC_ALLOCATION)
  set(CCOMMON_OPT "${CCOMMON_OPT} -DALLOC_STATIC")
endif()

if (DEVICEDRIVER_ALLOCATION)
  set(CCOMMON_OPT "${CCOMMON_OPT} -DALLOC_DEVICEDRIVER -DDEVICEDRIVER_NAME=\"/dev/mapper\"")
endif()

if (MIXED_MEMORY_ALLOCATION)
  set(CCOMMON_OPT "${CCOMMON_OPT} -DMIXED_MEMORY_ALLOCATION")
endif()

set(CCOMMON_OPT "${CCOMMON_OPT} -DVERSION=\"\\\"${JSTD_VERSION}\\\"\"")

set(REVISION "-r${JSTD_VERSION}")
set(MAJOR_VERSION ${JSTD_MAJOR_VERSION})

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${CCOMMON_OPT}")
if(NOT MSVC)
    set(CMAKE_ASM_FLAGS "${CMAKE_ASM_FLAGS} ${CCOMMON_OPT}")
endif()

# TODO: not sure what PFLAGS is -hpa
set(PFLAGS "${PFLAGS} ${CCOMMON_OPT} -I${TOPDIR} -DPROFILE ${COMMON_PROF}")

if (${CMAKE_SYSTEM_NAME} STREQUAL "Windows")
  set(LAPACK_CFLAGS "${LAPACK_CFLAGS} -DOPENBLAS_OS_WINDOWS")
endif()

if (${CMAKE_C_COMPILER} STREQUAL "LSB" OR ${CMAKE_SYSTEM_NAME} STREQUAL "Windows")
  set(LAPACK_CFLAGS "${LAPACK_CFLAGS} -DLAPACK_COMPLEX_STRUCTURE")
endif()

if (NOT DEFINED SUFFIX)
  set(SUFFIX o)
endif()

if (NOT DEFINED PSUFFIX)
  set(PSUFFIX po)
endif()

if (NOT DEFINED LIBSUFFIX)
  set(LIBSUFFIX a)
endif()

if (DYNAMIC_ARCH)
  if (USE_THREAD)
    set(LIBNAME "${LIBPREFIX}p${REVISION}.${LIBSUFFIX}")
    set(LIBNAME_P	"${LIBPREFIX}p${REVISION}_p.${LIBSUFFIX}")
  else ()
    set(LIBNAME "${LIBPREFIX}${REVISION}.${LIBSUFFIX}")
    set(LIBNAME_P	"${LIBPREFIX}${REVISION}_p.${LIBSUFFIX}")
  endif()
else ()
  if (USE_THREAD)
    set(LIBNAME "${LIBPREFIX}_${LIBCORE}p${REVISION}.${LIBSUFFIX}")
    set(LIBNAME_P	"${LIBPREFIX}_${LIBCORE}p${REVISION}_p.${LIBSUFFIX}")
  else ()
    set(LIBNAME	"${LIBPREFIX}_${LIBCORE}${REVISION}.${LIBSUFFIX}")
    set(LIBNAME_P	"${LIBPREFIX}_${LIBCORE}${REVISION}_p.${LIBSUFFIX}")
  endif()
endif()

set(LIBDLLNAME "${LIBPREFIX}.dll")
set(LIBSONAME "${LIBNAME}.${LIBSUFFIX}.so")
set(LIBDYNNAME "${LIBNAME}.${LIBSUFFIX}.dylib")
set(LIBDEFNAME "${LIBNAME}.${LIBSUFFIX}.def")
set(LIBEXPNAME "${LIBNAME}.${LIBSUFFIX}.exp")
set(LIBZIPNAME "${LIBNAME}.${LIBSUFFIX}.zip")

set(LIBS "${PROJECT_SOURCE_DIR}/${LIBNAME}")
set(LIBS_P "${PROJECT_SOURCE_DIR}/${LIBNAME_P}")

if (DEFINED ARCH)
  if (X86 OR X86_64 OR ${ARCH} STREQUAL "ia64" OR MIPS64)
    set(USE_GEMM3M 1)
  endif()

  if (${CORE} STREQUAL "generic")
    set(USE_GEMM3M 0)
  endif()
endif()

#export OSNAME
#export ARCH
#export CORE
#export LIBCORE
#export PGCPATH
#export CONFIG
#export CC
#export FC
#export BU
#export FU
#export NEED2UNDERSCORES
#export USE_THREAD
#export NUM_THREADS
#export NUM_CORES
#export SMP
#export MAKEFILE_RULE
#export NEED_PIC
#export BINARY
#export BINARY32
#export BINARY64
#export F_COMPILER
#export C_COMPILER
#export USE_OPENMP
#export CROSS
#export CROSS_SUFFIX
#export NOFORTRAN
#export NO_FBLAS
#export EXTRALIB
#export CEXTRALIB
#export FEXTRALIB
#export HAVE_SSE
#export HAVE_SSE2
#export HAVE_SSE3
#export HAVE_SSSE3
#export HAVE_SSE4_1
#export HAVE_SSE4_2
#export HAVE_SSE4A
#export HAVE_SSE5
#export HAVE_AVX
#export HAVE_VFP
#export HAVE_VFPV3
#export HAVE_VFPV4
#export HAVE_NEON
#export KERNELDIR
#export FUNCTION_PROFILE
#export TARGET_CORE
#
#export SHGEMM_UNROLL_M
#export SHGEMM_UNROLL_N
#export SGEMM_UNROLL_M
#export SGEMM_UNROLL_N
#export DGEMM_UNROLL_M
#export DGEMM_UNROLL_N
#export QGEMM_UNROLL_M
#export QGEMM_UNROLL_N
#export CGEMM_UNROLL_M
#export CGEMM_UNROLL_N
#export ZGEMM_UNROLL_M
#export ZGEMM_UNROLL_N
#export XGEMM_UNROLL_M
#export XGEMM_UNROLL_N
#export CGEMM3M_UNROLL_M
#export CGEMM3M_UNROLL_N
#export ZGEMM3M_UNROLL_M
#export ZGEMM3M_UNROLL_N
#export XGEMM3M_UNROLL_M
#export XGEMM3M_UNROLL_N

#if (USE_CUDA)
#  export CUDADIR
#  export CUCC
#  export CUFLAGS
#  export CULIB
#endif
