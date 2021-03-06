
#ifndef JSTD_SYSTEM_LIBCRANDOM_H
#define JSTD_SYSTEM_LIBCRANDOM_H

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

#include "jstd/system/LibcRand.h"

namespace jstd {

class LibcRandom {
public:
    typedef std::uint32_t   value_type;
    typedef LibcRandom      this_type;

    explicit LibcRandom(std::uint32_t initSeed = 0) {
        this->srand(initSeed);
    }

    ~LibcRandom() {}

    value_type rand_max() const {
        return static_cast<value_type>(LibcRand::rand_max());
    }

    void srand(value_type initSeed = 0) {
        LibcRand::srand(initSeed);
    }

    value_type rand() {
        return static_cast<value_type>(LibcRand::rand());
    }

    std::uint32_t nextUInt32() {
        return static_cast<std::uint32_t>(LibcRand::rand32());
    }

    std::int32_t nextInt32() {
        return static_cast<std::int32_t>(this->nextUInt32());
    }

    std::uint64_t nextUInt64() {
        return static_cast<std::uint64_t>(LibcRand::rand64());
    }

    std::int64_t nextInt64() {
        return static_cast<std::int64_t>(this->nextUInt64());
    }
};

} // namespace jstd

#endif // JSTD_SYSTEM_LIBCRANDOM_H
