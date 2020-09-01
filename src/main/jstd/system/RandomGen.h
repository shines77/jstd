
#ifndef JSTD_SYSTEM_RANDOMGEN_H
#define JSTD_SYSTEM_RANDOMGEN_H

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

#include "jstd/basic/stddef.h"
#include "jstd/basic/stdint.h"
#include "jstd/basic/stdsize.h"

#include <stdlib.h>     // For ::srand(), ::rand()
#include <time.h>

#include <cstdint>
#include <cstddef>
#include <cstdlib>      // For std::srand(), std::rand()

#include "jstd/system/LibcRandom.h"
#include "jstd/system/MT19937.h"
#include "jstd/system/MT19937_64.h"

namespace jstd {

template <typename RandomAlgorithm>
class BasicRandomGenerator {
public:
    typedef BasicRandomGenerator<RandomAlgorithm>   this_type;
    typedef RandomAlgorithm                         random_algorithm_t;
    typedef typename RandomAlgorithm::value_type    value_type;

private:
    static random_algorithm_t random_;

public:
    explicit BasicRandomGenerator(value_type initSeed = 0) {
        this_type::srand(initSeed);
    }

    ~BasicRandomGenerator() {}

    static value_type rand_max() {
        return this_type::random_.rand_max();
    }

    static void srand(value_type initSeed = 0) {
        this_type::random_.srand(initSeed);
    }

    static value_type rand() {
        return this_type::random_.rand();
    }

    static std::int32_t nextInt32()
    {
        return this_type::random_.nextInt32();
    }

    static std::uint32_t nextUInt32()
    {
        return this_type::random_.nextUInt32();
    }

    static std::int64_t nextInt64()
    {
        return this_type::random_.nextInt64();
    }

    static std::uint64_t nextUInt64()
    {
        return this_type::random_.nextUInt64();
    }

    static std::int32_t nextInt32(std::int32_t mininum, std::int32_t maxinum)
    {
        std::int32_t result;
        if (mininum < maxinum) {
            result = mininum + std::int32_t(this_type::nextUInt32() %
                                            std::uint32_t(maxinum - mininum + 1));
        }
        else if (mininum > maxinum) {
            result = maxinum + std::int32_t(this_type::nextUInt32() %
                                            std::uint32_t(mininum - maxinum + 1));
        }
        else {
            result = mininum;
        }
        return result;
    }

    static std::int32_t nextInt32(std::int32_t maxinum)
    {
        return this_type::nextInt32(0, maxinum);
    }

    static std::uint32_t nextUInt32(std::uint32_t mininum, std::uint32_t maxinum)
    {
        std::uint32_t result;
        if (mininum < maxinum) {
            result = mininum + (this_type::nextUInt32() %
                                std::uint32_t(maxinum - mininum + 1));
        }
        else if (mininum > maxinum) {
            result = maxinum + (this_type::nextUInt32() %
                                std::uint32_t(mininum - maxinum + 1));
        }
        else {
            result = mininum;
        }
        return result;
    }

    static std::uint32_t nextUInt32(std::uint32_t maxinum)
    {
        return this_type::nextUInt32(0, maxinum);
    }

    static std::int64_t nextInt64(std::int64_t mininum, std::int64_t maxinum)
    {
        std::int64_t result;
        if (mininum < maxinum) {
            result = mininum + std::int64_t(this_type::nextUInt64() %
                                            std::uint64_t(maxinum - mininum + 1));
        }
        else if (mininum > maxinum) {
            result = maxinum + std::int64_t(this_type::nextUInt64() %
                                            std::uint64_t(mininum - maxinum + 1));
        }
        else {
            result = mininum;
        }
        return result;
    }

    static std::int64_t nextInt64(std::int64_t maxinum)
    {
        return this_type::nextInt64(0, maxinum);
    }

    static std::uint64_t nextUInt64(std::uint64_t mininum, std::uint64_t maxinum)
    {
        std::uint64_t result;
        if (mininum < maxinum) {
            result = mininum + (this_type::nextUInt64() %
                                std::uint64_t(maxinum - mininum + 1));
        }
        else if (mininum > maxinum) {
            result = maxinum + (this_type::nextUInt64() %
                                std::uint64_t(mininum - maxinum + 1));
        }
        else {
            result = mininum;
        }
        return result;
    }

    static std::uint64_t nextUInt64(std::uint64_t maxinum)
    {
        return this_type::nextUInt64(0, maxinum);
    }

    static float nextFloat() {
        return ((float)this_type::nextUInt32() / 0xFFFFFFFFUL);
    }

    static double nextDouble() {
        return ((double)this_type::nextUInt32() / 0xFFFFFFFFUL);
    }

    // Generates a random number on [0, 1) with 53-bit resolution.
    static double nextDouble53() {
        value_type a = this_type::nextUInt32() >> 5, b = this_type::nextUInt32() >> 6;
        return (((double)a * 67108864.0 + b) / 9007199254740992.0);
    }
};

template <typename RandomAlgorithm>
typename BasicRandomGenerator<RandomAlgorithm>::random_algorithm_t
BasicRandomGenerator<RandomAlgorithm>::random_;

typedef BasicRandomGenerator<LibcRandom>    RandomGen;
typedef BasicRandomGenerator<MT19937>       MtRandomGen;
typedef BasicRandomGenerator<MT19937_64>    Mt64RandomGen;

} // namespace jstd

#endif // JSTD_SYSTEM_RANDOMGEN_H
