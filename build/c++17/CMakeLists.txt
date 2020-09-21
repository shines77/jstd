CMAKE_MINIMUM_REQUIRED(VERSION 2.8)

project(JStd)

set(CMAKE_ALLOW_LOOSE_LOOP_CONSTRUCTS ON)

set(JSTD_MAJOR_VERSION 0)
set(JSTD_MINOR_VERSION 1)
set(JSTD_PATCH_VERSION 1.dev)
set(JSTD_VERSION "${JSTD_MAJOR_VERSION}.${JSTD_MINOR_VERSION}.${JSTD_PATCH_VERSION}")

set(CMAKE_SOURCE_DIR "${CMAKE_SOURCE_DIR}/../..")
set(PROJECT_SOURCE_DIR "${PROJECT_SOURCE_DIR}/../..")

if (UNIX)
    # Adhere to GNU filesystem layout conventions
    include(GNUInstallDirs)
endif()

include(CMakePackageConfigHelpers)

set(CMAKE_VERBOSE_MAKEFILE ON)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/Modules/")

include("${PROJECT_SOURCE_DIR}/cmake/detect_cpu_architectures.cmake")

enable_language(ASM_NASM)

if (CMAKE_CL_64)
    set(CMAKE_CL_ARCH x64)
else()
    set(CMAKE_CL_ARCH x86)
endif()

##
## See: https://stackoverflow.com/questions/39258250/how-to-detect-if-64-bit-msvc-with-cmake
##
if (CMAKE_SIZEOF_VOID_P EQUAL 8)
    # 64 bits
    set(CMAKE_PLATFORM_ARCH Arch64)
elseif(CMAKE_SIZEOF_VOID_P EQUAL 4)
    # 32 bits
    set(CMAKE_PLATFORM_ARCH Arch32)
endif()

if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

message("------------ Options -------------")
message("  CMAKE_BUILD_TYPE         : ${CMAKE_BUILD_TYPE}")
message("  CMAKE_CL_ARCH            : ${CMAKE_CL_ARCH}")
message("  CMAKE_PLATFORM_ARCH      : ${CMAKE_PLATFORM_ARCH}")
message("  CMAKE_CPU_ARCHITECTURES  : ${CMAKE_CPU_ARCHITECTURES}")
message("----------------------------------")

message("-------------- Env ---------------")
message("  CMAKE_SOURCE_DIR: ${CMAKE_SOURCE_DIR}")
message("  CMAKE_BINARY_DIR: ${CMAKE_BINARY_DIR}")
message("  PROJECT_SOURCE_DIR: ${PROJECT_SOURCE_DIR}")
message("  PROJECT_BINARY_DIR: ${PROJECT_BINARY_DIR}")
message("----------------------------------")

if (CMAKE_BUILD_TYPE STREQUAL Debug)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D_DEBUG")
else()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DNDEBUG")
endif()

if (${CMAKE_SYSTEM_NAME} MATCHES "Linux")
    option(NO_AFFINITY "Disable support for CPU affinity masks to avoid binding processes from e.g. R or numpy/scipy to a single core" ON)
else()
    set(NO_AFFINITY 1)
endif()

include("${PROJECT_SOURCE_DIR}/cmake/utils.cmake")
include("${PROJECT_SOURCE_DIR}/cmake/system.cmake")

if (NOT MSVC)
    ## -Wall -Werror -Wextra -Wno-format -Wno-unused-function
    # set(CMAKE_CXX_FLAGS_DEFAULT "${CMAKE_CXX_FLAGS} -std=gnu++17 -march=native -mmmx -msse -msse2 -msse3 -mssse3 -msse4 -msse4a -msse4.1 -msse4.2 -Wall -Wno-unused-function -Wno-deprecated-declarations -Wno-unused-variable -Wno-register -Wno-deprecated -fPIC -U__STRICT_ANSI__")
    ## add_compile_options(-D__SSE3__ -D__SSE4A__ -D__SSE4_1__ -D__SSE4_2__)
    # set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_DEFAULT} -O3 -DNDEBUG")
    # set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEFAULT} -g -pg -D_DEBUG")

    set(CMAKE_CXX_FLAGS_COMMON          "-Wno-deprecated-declarations -Wno-float-equal -Wno-padded -Wno-old-style-cast -Wno-uninitialized -Wno-unused-function -Wno-unused-variable -Wno-unused-parameter -Wno-sign-conversion -Wno-cast-qual -Wno-cast-align -Wno-register -Wno-deprecated -fPIC -U__STRICT_ANSI__")

    set(CMAKE_C_FLAGS                   "-std=c99 -march=native -Wall ${CMAKE_CXX_FLAGS_COMMON} -Wno-missing-prototypes")
    set(CMAKE_C_FLAGS_DEBUG             "-g -pg -D_DEBUG")
    set(CMAKE_C_FLAGS_MINSIZEREL        "-Os -DNDEBUG")
    set(CMAKE_C_FLAGS_RELEASE           "-O3 -DNDEBUG")
    set(CMAKE_C_FLAGS_RELWITHDEBINFO    "-O2 -g")

    set(CMAKE_CXX_FLAGS                 "-std=gnu++17 -march=native -Wall ${CMAKE_CXX_FLAGS_COMMON}")
    set(CMAKE_CXX_FLAGS_DEBUG           "-g -pg -D_DEBUG")
    set(CMAKE_CXX_FLAGS_MINSIZEREL      "-Os -DNDEBUG")
    set(CMAKE_CXX_FLAGS_RELEASE         "-O3 -DNDEBUG")
    set(CMAKE_CXX_FLAGS_RELWITHDEBINFO  "-O2 -g")
endif()

if (MSVC AND MSVC_STATIC_CRT)
    set(CompilerFlags
            CMAKE_CXX_FLAGS
            CMAKE_CXX_FLAGS_DEBUG
            CMAKE_CXX_FLAGS_RELEASE
            CMAKE_C_FLAGS
            CMAKE_C_FLAGS_DEBUG
            CMAKE_C_FLAGS_RELEASE
            )
    foreach (CompilerFlag ${CompilerFlags})
        string(REPLACE "/MD" "/MT" ${CompilerFlag} "${${CompilerFlag}}")
    endforeach()
endif()

if (WIN32)
    add_compile_options("-D_WIN32_WINNT=0x0601 -D_CRT_SECURE_NO_WARNINGS")
    ## set(EXTRA_LIBS ${EXTRA_LIBS} ws2_32 mswsock)
endif()

if (UNIX)
    set(EXTRA_LIBS ${EXTRA_LIBS} pthread)
else()
    set(EXTRA_LIBS ${EXTRA_LIBS} ${CMAKE_THREAD_LIBS_INIT})
endif()

set(JSTD_ENABLED ON)

if (JSTD_ENABLED)

set(JSTD_LIBNAME jstd${SUFFIX64_UNDERSCORE})

# Set default library output directory
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/lib/c++17)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/lib/c++17)
if (MSVC)
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_DEBUG ${PROJECT_BINARY_DIR}/lib/msvc/Debug)
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_RELEASE ${PROJECT_BINARY_DIR}/lib/msvc/Release)
endif()

# get obj vars into format that add_library likes: $<TARGET_OBJS:objlib>
# (see http://www.cmake.org/cmake/help/v3.0/command/add_library.html)
set(TARGET_OBJS "")
foreach (SUBDIR ${SUBDIRS})
    add_subdirectory(${SUBDIR})
    string(REPLACE "/" "_" subdir_obj ${SUBDIR})
    list(APPEND TARGET_OBJS "$<TARGET_OBJECTS:${subdir_obj}>")
endforeach()

# Only generate .def for dll on MSVC and always produce pdb files for debug and release
if (MSVC)
    if (${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION} VERSION_LESS 3.4)
        set(JSTD_DEF_FILE "${PROJECT_BINARY_DIR}/jstd.def")
    endif()
    set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} /Zi")
    set(CMAKE_SHARED_LINKER_FLAGS_RELEASE "${CMAKE_SHARED_LINKER_FLAGS_RELEASE} /DEBUG /OPT:REF /OPT:ICF")
endif()

include_directories(../../include)
include_directories(../../src)
include_directories(../../src/main)

set(JSTD_SOURCE_FILES
    ../../src/main/jstd/system/getchar.c
    ../../src/main/jstd/system/sleep.c
    ../../src/main/jstd/system/crtdbg_env.c
    ../../src/main/jstd/basic/assert-c.c
    )

# add objects to the jstd lib
add_library(${JSTD_LIBNAME} ${JSTD_SOURCE_FILES} ${TARGET_OBJS} ${JSTD_DEF_FILE})
target_include_directories(${JSTD_LIBNAME} INTERFACE $<INSTALL_INTERFACE:jstd/all${SUFFIX64}>)

# Android needs to explicitly link against libm
if (ANDROID)
    target_link_libraries(${JSTD_LIBNAME} m)
endif()

# Handle MSVC exports
if (MSVC AND BUILD_SHARED_LIBS)
    if (${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION} VERSION_LESS 3.4)
        include("${PROJECT_SOURCE_DIR}/cmake/export.cmake")
    else()
        # Creates verbose .def file (51KB vs 18KB)
        set_target_properties(${JSTD_LIBNAME} PROPERTIES WINDOWS_EXPORT_ALL_SYMBOLS true)
    endif()
endif()

# Set output for libopenblas
set_target_properties( ${JSTD_LIBNAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/lib/c++17)
set_target_properties( ${JSTD_LIBNAME} PROPERTIES LIBRARY_OUTPUT_NAME_DEBUG "${JSTD_LIBNAME}_debug")
set_target_properties( ${JSTD_LIBNAME} PROPERTIES EXPORT_NAME "jstd")

foreach (OUTPUTCONFIG ${CMAKE_CONFIGURATION_TYPES})
    string( TOUPPER ${OUTPUTCONFIG} OUTPUTCONFIG )

    set_target_properties( ${JSTD_LIBNAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY_${OUTPUTCONFIG} ${PROJECT_BINARY_DIR}/lib/c++17/${OUTPUTCONFIG} )
    set_target_properties( ${JSTD_LIBNAME} PROPERTIES LIBRARY_OUTPUT_DIRECTORY_${OUTPUTCONFIG} ${PROJECT_BINARY_DIR}/lib/c++17/${OUTPUTCONFIG} )
    set_target_properties( ${JSTD_LIBNAME} PROPERTIES ARCHIVE_OUTPUT_DIRECTORY_${OUTPUTCONFIG} ${PROJECT_BINARY_DIR}/lib/c++17/${OUTPUTCONFIG} )
endforeach()

if (UNIX)
    set(EXTRA_LIBS ${EXTRA_LIBS} pthread)
    if (USE_THREAD)
        # Add threading library to linker
        find_package(Threads)
        if (THREADS_HAVE_PTHREAD_ARG)
            set_property(TARGET ${JSTD_LIBNAME} PROPERTY COMPILE_OPTIONS "-pthread")
            set_property(TARGET ${JSTD_LIBNAME} PROPERTY INTERFACE_COMPILE_OPTIONS "-pthread")
        endif()
    endif()
else()
    set(EXTRA_LIBS ${EXTRA_LIBS} ${CMAKE_THREAD_LIBS_INIT})
endif()

target_link_libraries(${JSTD_LIBNAME} ${EXTRA_LIBS})

set_target_properties(${JSTD_LIBNAME} PROPERTIES
    VERSION ${JSTD_MAJOR_VERSION}.${JSTD_MINOR_VERSION}
    SOVERSION ${JSTD_MAJOR_VERSION}
)

if (BUILD_SHARED_LIBS)
    if (NOT MSVC)
        target_link_libraries(${JSTD_LIBNAME} "-Wl,-allow-multiple-definition")
    else()
        set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} /FORCE:MULTIPLE")
    endif()
endif()

if (BUILD_SHARED_LIBS AND NOT ${SYMBOLPREFIX}${SYMBOLSUFFIX} STREQUAL "")
    if (NOT DEFINED ARCH)
        set(ARCH_IN "x86_64")
    else()
        set(ARCH_IN ${ARCH})
    endif()

    if (NOT ${SYMBOLPREFIX} STREQUAL "")
        message(STATUS "adding prefix ${SYMBOLPREFIX} to names of exported symbols in ${JSTD_LIBNAME}")
    endif()
    if (NOT ${SYMBOLSUFFIX} STREQUAL "")
        message(STATUS "adding suffix ${SYMBOLSUFFIX} to names of exported symbols in ${JSTD_LIBNAME}")
    endif()
endif()

endif(JSTD_ENABLED)

add_custom_target(release
    COMMAND ${CMAKE_COMMAND} -DCMAKE_BUILD_TYPE=Release ${CMAKE_SOURCE_DIR}
    COMMAND ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR} --target all
    COMMENT "Switch CMAKE_BUILD_TYPE to Release"
)

project(dict-test)

include_directories(../../include)
include_directories(../../src)
include_directories(../../src/main)
include_directories(../../src/test)
include_directories(../../src/unittest)
include_directories(../../src/test/dict_test)

set(SOURCE_FILES
    ../../src/test/dict_test/dict_test.cpp
    )

add_executable(dict-test ${SOURCE_FILES})
target_link_libraries(dict-test ${EXTRA_LIBS} ${JSTD_LIBNAME})

project(benchmark)

include_directories(../../include)
include_directories(../../src)
include_directories(../../src/main)
include_directories(../../src/test)
include_directories(../../src/unittest)
include_directories(../../src/test/benchmark)

set(SOURCE_FILES
    ../../src/test/benchmark/benchmark.cpp
    )

add_executable(benchmark ${SOURCE_FILES})
target_link_libraries(benchmark ${EXTRA_LIBS} ${JSTD_LIBNAME})

project(time_hash_map)

include_directories(../../include)
include_directories(../../src)
include_directories(../../src/main)
include_directories(../../src/test)
include_directories(../../src/unittest)
include_directories(../../src/test/time_hash_map)

set(SOURCE_FILES
    ../../src/test/time_hash_map/time_hash_map.cpp
    )

add_executable(time_hash_map ${SOURCE_FILES})
target_link_libraries(time_hash_map ${EXTRA_LIBS} ${JSTD_LIBNAME})
