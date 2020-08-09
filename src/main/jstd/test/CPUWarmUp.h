
#ifndef JSTD_TEST_CPU_WARMUP_H
#define JSTD_TEST_CPU_WARMUP_H

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

#include <stdio.h>

#if defined(_WIN32) || defined(WIN32) || defined(OS_WINDOWS) || defined(_WINDOWS_)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif // WIN32_LEAN_AND_MEAN
#include <mmsystem.h>
#pragma comment(lib, "winmm.lib")
#endif // _WIN32

#if !defined(_MSC_VER) || (_MSC_VER >= 1800)
#include <chrono>
#endif

namespace jtest {
namespace CPU {

#if !defined(_MSC_VER) || (_MSC_VER >= 1800)

void warmup(int delayMillsecs)
{
#if defined(NDEBUG)
    double delayTimeLimit = (double)delayMillsecs / 1.0;
    volatile int sum = 0;

    printf("CPU warm-up begin ...\n");
    ::fflush(stdout);
    std::chrono::high_resolution_clock::time_point startTime, endTime;
    std::chrono::duration<double, std::ratio<1, 1000>> elapsedTime;
    startTime = std::chrono::high_resolution_clock::now();
    do {
        for (int i = 0; i < 500; ++i) {
            sum += i;
            for (int j = 5000; j >= 0; --j) {
                sum -= j;
            }
        }
        endTime = std::chrono::high_resolution_clock::now();
        elapsedTime = endTime - startTime;
    } while (elapsedTime.count() < delayTimeLimit);

    printf("sum = %d, time: %0.3f ms\n", sum, elapsedTime.count());
    printf("CPU warm-up end   ... \n\n");
    ::fflush(stdout);
#endif // !_DEBUG
}

#else

void warmup(DWORD delayMillsecs)
{
#if defined(NDEBUG)
    volatile int sum = 0;

    printf("CPU warm-up begin ...\n");
    ::fflush(stdout);
    DWORD startTime, endTime;
    DWORD elapsedTime;
    startTime = ::timeGetTime();
    do {
        for (int i = 0; i < 500; ++i) {
            sum += i;
            for (int j = 5000; j >= 0; --j) {
                sum -= j;
            }
        }
        endTime = ::timeGetTime();
        elapsedTime = endTime - startTime;
    } while (elapsedTime < delayMillsecs);

    printf("sum = %d, time: %u ms\n", sum, elapsedTime);
    printf("CPU warm-up end   ... \n\n");
    ::fflush(stdout);
#endif // !_DEBUG
}

#endif // _MSC_VER

} // namespace CPU
} // namespace jtest

#endif // JSTD_TEST_CPU_WARMUP_H
