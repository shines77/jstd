
#ifndef JSTD_TEST_STOPWATCH_H
#define JSTD_TEST_STOPWATCH_H

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

#include <chrono>
#include <ratio>

using namespace std::chrono;

namespace jstd {

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
          endTime_(std::chrono::high_resolution_clock::now()) {
    }
    ~StopWatch() {}

    static time_point now() {
        return std::chrono::high_resolution_clock::now();
    }

    void restart() {
        this->startTime_ = StopWatch::now();
        this->endTime_ = this->startTime_;
    }

    void start() {
        this->startTime_ = StopWatch::now();
    }

    void stop () {
        this->endTime_ = StopWatch::now();
    }

    float_type getElapsedTime() {
        return this->getMillisecs();
    }

    float_type getMillisecs() {
        duration_ms elapsedTime = this->endTime_ - this->startTime_;
        return elapsedTime.count();
    }

    float_type getElapsedSeconds() {
        duration_ms elapsedTime = this->endTime_ - this->startTime_;
        return (elapsedTime.count() * float_type(1000.0));
    }
};

} // namespace jstd

#endif // JSTD_TEST_STOPWATCH_H
