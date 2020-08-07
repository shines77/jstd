
#ifndef JSTD_TEST_STOPWATCH_V1_H
#define JSTD_TEST_STOPWATCH_V1_H

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

#include <chrono>
#include <ratio>

#ifndef __COMPILER_BARRIER
#if defined(_MSC_VER) || defined(__ICL) || defined(__INTEL_COMPILER)
#define __COMPILER_BARRIER()        _ReadWriteBarrier()
#else
#define __COMPILER_BARRIER()        asm volatile ("" : : : "memory")
#endif
#endif

using namespace std::chrono;

namespace jstd {
namespace v1 {

template <typename T = double>
class StopWatch {
public:
    typedef typename std::remove_reference<
                typename std::remove_cv<T>::type
            >::type     float_type;

    typedef std::chrono::high_resolution_clock::time_point      time_point;
    typedef std::chrono::duration<float_type, std::milli>       duration_ms;

private:
    time_point  startTime_, endTime_;

public:
    StopWatch()
        : startTime_(std::chrono::high_resolution_clock::now()),
          endTime_(startTime_) {
    }
    ~StopWatch() {}

    static time_point now() {
        __COMPILER_BARRIER();
        return std::chrono::high_resolution_clock::now();
    }

    void restart() {
        this->startTime_ = StopWatch::now();
        this->endTime_ = this->startTime_;
        __COMPILER_BARRIER();
    }

    void start() {
        this->startTime_ = StopWatch::now();
        __COMPILER_BARRIER();
    }

    void stop () {
        __COMPILER_BARRIER();
        this->endTime_ = StopWatch::now();
    }

    float_type getElapsedTime() {
        __COMPILER_BARRIER();
        duration_ms elapsedTime = this->endTime_ - this->startTime_;
        return elapsedTime.count();
    }

    float_type getElapsedNanosec() {
        return (this->getElapsedTime() * float_type(1.0 / 1000000.0));
    }

    float_type getElapsedMicrosec() {
        return (this->getElapsedTime() * float_type(1.0 / 1000.0));
    }

    float_type getElapsedMillisec() {
        return this->getElapsedTime();
    }

    float_type getElapsedSecond() {
        return (this->getElapsedTime() * float_type(1000.0));
    }
};

} // namespace v1
} // namespace jstd

#endif // JSTD_TEST_STOPWATCH_V1_H
