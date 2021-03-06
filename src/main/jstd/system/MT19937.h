
//************************************************************************
//  This is an improved version of the Equamen mersenne twister.
//
//  Copyright (C) 2017-2020 Guo XiongHui
//
//  This program is free software: you can redistribute it and/or modify
//  it under the terms of the GNU Affero General Public License as
//  published by the Free Software Foundation, either version 3 of the
//  License, or (at your option) any later version.
//
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
//  GNU Affero General Public License for more details.
//
//  You should have received a copy of the GNU Affero General Public License
//  along with this program. If not, see <http://www.gnu.org/licenses/>.
//************************************************************************

//************************************************************************
//  This is a slightly modified version of Equamen mersenne twister.
//
//  Copyright (C) 2009 Chipset
//
//  This program is free software: you can redistribute it and/or modify
//  it under the terms of the GNU Affero General Public License as
//  published by the Free Software Foundation, either version 3 of the
//  License, or (at your option) any later version.
//
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
//  GNU Affero General Public License for more details.
//
//  You should have received a copy of the GNU Affero General Public License
//  along with this program. If not, see <http://www.gnu.org/licenses/>.
//************************************************************************

//
// Original Coyright (c) 1997 - 2002, Makoto Matsumoto and Takuji Nishimura
//
// Functions for MT19937, with initialization improved 2002/2/10.
// Coded by Takuji Nishimura and Makoto Matsumoto.
// This is a faster version by taking Shawn Cokus's optimization,
// Matthe Bellew's simplification, Isaku Wada's real version.
// C++ version by Lyell Haynes (Equamen)
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions
// are met:
//
// 1. Redistributions of source code must retain the above copyright
//    notice, this list of conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above copyright
//    notice, this list of conditions and the following disclaimer in the
//    documentation and/or other materials provided with the distribution.
//
// 3. The names of its contributors may not be used to endorse or promote
//    products derived from this software without specific prior written
//    permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR
// CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
// EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
// PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
// LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
// NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//

//////////////////////////////////////////////////////////////////////////
//
// TGFSR (twisted generalised feedback shift register)
//
//    w:        length (in bits)
//    n:        recursion length
//    m:        period parameter, used as the offset of the third stage
//    r:        low-order mask / low-order bits to be extracted
//    a:        the parameters of the rotation matrix
//    f:        Initialize the required parameters of the Mason rotating chain
//    b, c:     TGFSR mask
//    s, t:     displacement of TGFSR
//    u, d, l:  mask and displacement required for additional mason rotation
//
// MT19937-32:
//    (w, n, m, r) = (32, 624, 397, 31)
//    a = 0x9908B0DF
//    f = 1812433253
//    (u, d) = (11, 0xFFFFFFFF)
//    (s, b) = (7,  0x9D2C5680)
//    (t, c) = (15, 0xEFC60000)
//    l = 18
//
// MT19937-64:
//    (w, n, m, r) = (64, 312, 156, 31)
//    a = 0xB5026F5AA96619E9
//    f = 6364136223846793005
//    (u, d) = (29, 0x5555555555555555)
//    (s, b) = (17, 0x71D67FFFEDA60000)
//    (t, c) = (37, 0xFFF7EEE000000000)
//    l = 43
//
//
// Mersenne Twister MT19937
//
//    http://www.cppblog.com/Chipset/archive/2009/02/07/73177.html
//    https://blog.csdn.net/tick_tock97/article/details/78657851
//
//////////////////////////////////////////////////////////////////////////

#ifndef JSTD_SYSTEM_MT19937_H
#define JSTD_SYSTEM_MT19937_H

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
#include <vector>

#include "jstd/system/LibcRand.h"

namespace jstd {

class MT19937
{
public:
    typedef std::size_t value_type;

    static const value_type kMTDefaultSeed = 19650218UL;
    static const value_type kMTRandMax = 0xFFFFFFFFUL;

    static const std::size_t N = 624, M = 397;

private:
    value_type * next;
    std::size_t  left;
    value_type   state[N];

public:
    explicit MT19937(value_type initSeed = kMTDefaultSeed) : next(nullptr), left(1) {
        this->init(initSeed);
    }

    MT19937(const value_type * init_key, std::size_t key_len, value_type initSeed = kMTDefaultSeed)
        : next(nullptr), left(1) {
        this->init(init_key, key_len, initSeed);
    }

    MT19937(const std::vector<value_type> & init_key, value_type initSeed = kMTDefaultSeed)
        : next(nullptr), left(1) {
        this->init(init_key, initSeed);
    }

    ~MT19937() {}

    value_type rand_max() const {
        return static_cast<value_type>(kMTRandMax);
    }

private:
    void init(value_type initSeed = kMTDefaultSeed) {
        if (initSeed == 0) {
            time_t timer;
            ::time(&timer);
            initSeed = static_cast<value_type>(timer);
        }
        this->state[0] = initSeed & 0xFFFFFFFFUL;
        for (std::size_t j = 1; j < N; ++j) {
            this->state[j] = (1812433253UL * (this->state[j - 1] ^ (this->state[j - 1] >> 30)) + j);
            // See Knuth TAOCP Vol2. 3rd Ed. P.106 for multiplier.
            // In the previous versions, MSBs of the seed affect
            // only MSBs of the array state[].
            // 2002/01/09 modified by Makoto Matsumoto
            this->state[j] &= 0xFFFFFFFFUL;  // For WORDSIZE > 32 bit machines
        }
    }

    void init(const value_type * init_key, std::size_t key_len, value_type initSeed = kMTDefaultSeed) {
        this->init(initSeed);
        this->init_keys(init_key, key_len);
    }

    void init(const std::vector<value_type> & init_key, value_type initSeed = kMTDefaultSeed) {
        this->init(initSeed);
        this->init_keys(init_key, init_key.size());
    }

    void init_keys(const std::vector<value_type> & init_key) {
        this->init_keys(init_key, init_key.size());
    }

    template <typename ValueList>
    void init_keys(const ValueList & init_key, std::size_t key_len) {
        std::size_t i = 1, j = 0;
        std::size_t k = (N > key_len) ? N : key_len;
        for (; k > 0; --k) {
            // non linear
            this->state[i] = (this->state[i] ^ ((this->state[i - 1] ^ (this->state[i - 1] >> 30))
                                                * 1664525UL)) + init_key[j] + j;
            this->state[i] &= 0xFFFFFFFFUL;   // For WORDSIZE > 32 bit machines
            ++i;
            ++j;
            if (i >= N) {
                this->state[0] = this->state[N - 1];
                i = 1;
            }
            if (j >= key_len)
                j = 0;
        }

        for (k = N - 1; k > 0; --k) {
            // non linear
            this->state[i] = (this->state[i] ^ ((this->state[i - 1] ^ (this->state[i - 1] >> 30))
                                                * 1566083941UL)) - i;
            this->state[i] &= 0xFFFFFFFFUL;   // For WORDSIZE > 32 bit machines
            ++i;
            if (i >= N) {
                this->state[0] = this->state[N - 1];
                i = 1;
            }
        }

        this->state[0] = 0x80000000UL;    // MSB is 1; assuring non-zero initial array
    };

    void next_state() {
        value_type * p = &this->state[0];

        for (std::size_t i = N - M + 1; --i; ++p) {
            *p = (p[M] ^ this->twist(p[0], p[1]));
        }

        for (std::size_t i = M; --i; ++p) {
            *p = (p[M - N] ^ this->twist(p[0], p[1]));
        }

        *p = p[M - N] ^ this->twist(p[0], this->state[0]);
        this->left = N;
        this->next = &this->state[0];
    }

    value_type mixbits(value_type u, value_type v) const {
        return (u & 0x80000000UL) | (v & 0x7FFFFFFFUL);
    }

    value_type twist(value_type u, value_type v) const {
        return ((this->mixbits(u, v) >> 1) ^ ((v & 1UL) ? 2567483615UL : 0UL));
    }

public:
    void srand(value_type initSeed = kMTDefaultSeed) {
        this->init(initSeed);
        this->next_state();
    }

    void srand(const value_type * init_key, std::size_t key_len, value_type initSeed = kMTDefaultSeed) {
        this->init(init_key, key_len, initSeed);
        this->next_state();
    }

    void srand(const std::vector<value_type> & init_key, value_type initSeed = kMTDefaultSeed) {
        this->init(init_key, initSeed);
        this->next_state();
    }

    value_type rand() {
        value_type y;
        if (0 == --this->left) {
            this->next_state();
        }

        if (next != nullptr)
            y = *(this->next++);
        else
            y = static_cast<value_type>(LibcRand::rand32());

        // Tempering
        y ^= (y >> 11);
        y ^= (y <<  7) & 0x9d2c5680UL;
        y ^= (y << 15) & 0xefc60000UL;
        y ^= (y >> 18);
        return y;
    }

    std::int32_t nextInt32() {
        return static_cast<std::int32_t>(this->rand());
    }

    std::uint32_t nextUInt32() {
        return static_cast<std::uint32_t>(this->rand());
    }

    std::int64_t nextInt64() {
        return static_cast<std::int64_t>(((std::uint64_t)this->nextUInt32() << 32) |
                                          (this->nextUInt32() & 0xFFFFFFFFUL));
    }

    std::uint64_t nextUInt64() {
        return static_cast<std::uint64_t>(((std::uint64_t)this->nextUInt32() << 32) |
                                           (this->nextUInt32() & 0xFFFFFFFFUL));
    }
};

} // namespace jstd

#endif // JSTD_SYSTEM_MT19937_H
