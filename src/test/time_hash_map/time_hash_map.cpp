
// Enable Visual Leak Detector (For Visual Studio)
#define JSTD_ENABLE_VLD         1

#ifdef _MSC_VER
#include <jstd/basic/vld.h>
#endif

#ifndef __SSE4_2__
#define __SSE4_2__              1
#endif

// For avoid the MSVC stdext::hasp_map<K,V>'s deprecation warnings.
#define _SILENCE_STDEXT_HASH_DEPRECATION_WARNINGS

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

#include <iostream>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <string>
#include <atomic>
#include <memory>
#include <utility>
#include <vector>
#include <unordered_map>
#if defined(_MSC_VER)
#include <hash_map>
#else
#include <ext/hash_map>
#endif
#include <algorithm>

/* SIMD support features */
#define JSTD_HAVE_MMX           1
#define JSTD_HAVE_SSE           1
#define JSTD_HAVE_SSE2          1
#define JSTD_HAVE_SSE3          1
#define JSTD_HAVE_SSSE3         1
#define JSTD_HAVE_SSE4          1
#define JSTD_HAVE_SSE4A         1
#define JSTD_HAVE_SSE4_1        1
#define JSTD_HAVE_SSE4_2        1

#if __SSE4_2__

// Support SSE 4.2: _mm_crc32_u32(), _mm_crc32_u64().
#define JSTD_HAVE_SSE42_CRC32C  1

// Support Intel SMID SHA module: sha1 & sha256, it's higher than SSE 4.2 .
// _mm_sha1msg1_epu32(), _mm_sha1msg2_epu32() and so on.
#define JSTD_HAVE_SMID_SHA      0

#endif // __SSE4_2__

// String compare mode
#define STRING_UTILS_STL        0
#define STRING_UTILS_U64        1
#define STRING_UTILS_SSE42      2

#define STRING_UTILS_MODE       STRING_UTILS_SSE42

// Use in <jstd/support/PowerOf2.h>
#define JSTD_SUPPORT_X86_BITSCAN_INSTRUCTION    1

#define USE_JSTD_HASH_TABLE     0
#define USE_JSTD_DICTIONARY     0

#include <jstd/basic/stddef.h>
#include <jstd/basic/stdint.h>
#include <jstd/basic/inttypes.h>

#include <jstd/hash/dictionary.h>
#include <jstd/hash/hashmap_analyzer.h>
#include <jstd/string/string_view.h>
#include <jstd/string/string_view_array.h>
#include <jstd/system/Console.h>
#include <jstd/system/RandomGen.h>
#include <jstd/test/StopWatch.h>
#include <jstd/test/CPUWarmUp.h>
#include <jstd/test/ProcessMemInfo.h>

#include <jstd/hasher/fnv1a.h>

//#include <jstd/all.h>

#include "BenchmarkResult.h"

#if defined(_MSC_VER)
using namespace stdext;
#else
using namespace __gnu_cxx;
#endif

using namespace jstd;
using namespace jtest;

static const bool FLAGS_test_sparse_hash_map = true;
static const bool FLAGS_test_dense_hash_map = true;

#if defined(_MSC_VER)
static const bool FLAGS_test_std_hash_map = false;
#else
static const bool FLAGS_test_std_hash_map = true;
#endif
static const bool FLAGS_test_std_unordered_map = true;
static const bool FLAGS_test_jstd_dictionary = true;
static const bool FLAGS_test_map = true;

static const bool FLAGS_test_4_bytes = true;
static const bool FLAGS_test_8_bytes = true;
static const bool FLAGS_test_16_bytes = true;
static const bool FLAGS_test_256_bytes = true;

#ifdef NDEBUG
static const std::size_t kDefaultIters = 10000000;
#else
static const std::size_t kDefaultIters = 1000;
#endif

static const std::size_t kInitCapacity = 16;

// Returns the number of hashes that have been done since the last
// call to NumHashesSinceLastCall().  This is shared across all
// HashObject instances, which isn't super-OO, but avoids two issues:
// (1) making HashObject bigger than it ought to be (this is very
// important for our testing), and (2) having to pass around
// HashObject objects everywhere, which is annoying.
static std::size_t g_num_hashes = 0;
static std::size_t g_num_constructor = 0;
static std::size_t g_num_copies = 0;
static std::size_t g_num_moves = 0;

static void reset_counter()
{
    g_num_hashes = 0;
    g_num_constructor = 0;
    g_num_copies = 0;
    g_num_moves  = 0;
}

/*
 * These are the objects we hash.  Size is the size of the object
 * (must be > sizeof(int).  Hashsize is how many of these bytes we
 * use when hashing (must be > sizeof(int) and < Size).
 */
template <typename Key, std::size_t Size, std::size_t HashSize>
class HashObject {
public:
    typedef Key                             key_type;
    typedef HashObject<Key, Size, HashSize> this_type;

    static const std::size_t kMinSize = (jstd::max)(sizeof(Key), sizeof(int));
    static const std::size_t kSize = (jstd::max)(Size, kMinSize);
    static const std::size_t kHashSize = (jstd::min)((jstd::max)(HashSize, kMinSize), kSize);
    static const std::size_t kBufLen = (kSize > sizeof(Key)) ? (kSize - sizeof(Key)) : 0;
    static const std::size_t kHashLen = (kHashSize > sizeof(Key)) ? (kHashSize - sizeof(Key)) : 0;

private:
    key_type key_;   // the key used for hashing
    char buffer_[kBufLen];

public:
    HashObject() : key_(0) {
        ::memset(this->buffer_, 0, sizeof(this->buffer_));
        g_num_constructor++;
    }
    HashObject(key_type key) : key_(key) {
        ::memset(this->buffer_, (int)(key & 0xFFUL), sizeof(this->buffer_));   // a "random" char
        g_num_constructor++;
    }
    HashObject(const this_type & that) {
        operator = (that);
    }

    void operator = (const this_type & that) {
        this->key_ = that.key_;
        ::memcpy(this->buffer_, that.buffer_, sizeof(this->buffer_));
        g_num_copies++;
    }

    std::uint32_t Hash() const {
        std::uint32_t hash_val = static_cast<std::uint32_t>(this->key_);
        //for (std::size_t i = 0; i < kHashLen; ++i) {
        //    hash_val += this->buffer_[i];
        //}
        hash_val += static_cast<std::uint32_t>((this->key_ & 0xFFUL) * kHashLen);
        g_num_hashes++;
        return hash_val;
    }

    bool operator == (const this_type & that) const {
        return this->key_ == that.key_;
    }
    bool operator < (const this_type & that) const {
        return this->key_ < that.key_;
    }
    bool operator <= (const this_type & that) const {
        return this->key_ <= that.key_;
    }
};

// A specialization for the case sizeof(buffer_) == 0
template <>
class HashObject<std::uint32_t, sizeof(std::uint32_t), sizeof(std::uint32_t)> {
public:
    typedef std::uint32_t   key_type;
    typedef HashObject<std::uint32_t, sizeof(std::uint32_t), sizeof(std::uint32_t)>
                            this_type;
private:
    std::uint32_t key_;   // the key used for hashing

public:
    HashObject() : key_(0) {
        g_num_constructor++;
    }
    HashObject(std::uint32_t key) : key_(key) {
        g_num_constructor++;
    }
    HashObject(const this_type & that) {
        operator = (that);
    }

    void operator = (const this_type & that) {
        this->key_ = that.key_;
        g_num_copies++;
    }

    std::uint32_t Hash() const {
        g_num_hashes++;
        return static_cast<std::uint32_t>(this->key_);
    }

    bool operator == (const this_type & that) const {
        return this->key_ == that.key_;
    }
    bool operator < (const this_type & that) const {
        return this->key_ < that.key_;
    }
    bool operator <= (const this_type & that) const {
        return this->key_ <= that.key_;
    }
};

#if defined(WIN64) || defined(_WIN64) || defined(_M_X64) || defined(_M_AMD64) \
 || defined(__amd64__) || defined(__x86_64__) || defined(__LP64__)

// A specialization for the case sizeof(buffer_) == 0
template <>
class HashObject<std::size_t, sizeof(std::size_t), sizeof(std::size_t)> {
public:
    typedef std::size_t     key_type;
    typedef HashObject<std::size_t, sizeof(std::size_t), sizeof(std::size_t)>
                            this_type;
private:
    std::size_t key_;   // the key used for hashing

public:
    HashObject() : key_(0) {
        g_num_constructor++;
    }
    HashObject(std::size_t key) : key_(key) {
        g_num_constructor++;
    }
    HashObject(const this_type & that) {
        operator = (that);
    }

    void operator = (const this_type & that) {
        this->key_ = that.key_;
        g_num_copies++;
    }

    std::uint32_t Hash() const {
        g_num_hashes++;
        return static_cast<std::uint32_t>(this->key_);
    }

    bool operator == (const this_type & that) const {
        return this->key_ == that.key_;
    }
    bool operator < (const this_type & that) const {
        return this->key_ < that.key_;
    }
    bool operator <= (const this_type & that) const {
        return this->key_ <= that.key_;
    }
};

#endif  // _WIN64 || __amd64__

template <typename HashObj, typename ResultType = std::uint32_t>
class HashFn {
public:
    typedef HashObj                     hash_object_t;
    typedef typename HashObj::key_type  key_type;
    typedef ResultType                  result_type;

    // These two public members are required by msvc.  4 and 8 are defaults.
    static const std::size_t bucket_size = 4;
    static const std::size_t min_buckets = 8;

    //result_type operator () (const hash_object_t & obj) const {
    //    return static_cast<result_type>(obj.Hash());
    //}

    //// Do the identity hash for pointers.
    //result_type operator () (const hash_object_t * obj) const {
    //    return reinterpret_cast<result_type>(obj);
    //}

    //// Less operator for MSVC's hash containers.
    //bool operator () (const hash_object_t & a,
    //                  const hash_object_t & b) const {
    //    return (a < b);
    //}

    //bool operator () (const hash_object_t * a,
    //                  const hash_object_t * b) const {
    //    return (a < b);
    //}

    template <std::size_t Size, std::size_t HashSize>
    result_type operator () (const HashObject<key_type, Size, HashSize> & obj) const {
        return static_cast<result_type>(obj.Hash());
    }

    // Do the identity hash for pointers.
    template <std::size_t Size, std::size_t HashSize>
    result_type operator () (const HashObject<key_type, Size, HashSize> * obj) const {
        return reinterpret_cast<result_type>(obj);
    }

    // Less operator for MSVC's hash containers.
    template <std::size_t Size, std::size_t HashSize>
    bool operator () (const HashObject<key_type, Size, HashSize> & a,
                      const HashObject<key_type, Size, HashSize> & b) const {
        return (a < b);
    }

    template <std::size_t Size, std::size_t HashSize>
    bool operator () (const HashObject<key_type, Size, HashSize> * a,
                      const HashObject<key_type, Size, HashSize> * b) const {
        return (a < b);
    }
};

#if defined(_MSC_VER)
template <typename Key, typename Value, typename Hasher>
class StdHashMap : public hash_map<Key, Value, Hasher> {
public:
    typedef hash_map<Key, Value, Hasher> this_type;

    StdHashMap() : this_type() {}
    StdHashMap(std::size_t initCapacity) : this_type() {
    }

    void resize(size_t r) { /* Not support */ }
};
#else
template <typename Key, typename Value, typename Hasher>
class StdHashMap : public hash_map<Key, Value, Hasher> {
public:
    typedef hash_map<Key, Value, Hasher> this_type;
    typedef Value                        mapped_type;
    typedef typename Key::key_type       ident_type;

    StdHashMap() : this_type() {}
    StdHashMap(std::size_t initCapacity) : this_type() {
    }

    void emplace(ident_type id, mapped_type value) {
        this->operator [](id) = value;
    }

    // Don't need to do anything: hash_map is already easy to use!
};
#endif

template <typename Key, typename Value, typename Hasher>
class StdUnorderedMap : public std::unordered_map<Key, Value, Hasher> {
public:
    typedef std::unordered_map<Key, Value, Hasher> this_type;

    StdUnorderedMap() : this_type() {}
    StdUnorderedMap(std::size_t initCapacity) : this_type(initCapacity) {
    }

    // resize() is called rehash() in tr1
    void resize(std::size_t newSize) {
        this->rehash(newSize);
    }
};

template <typename Vector>
void shuffle_vector(Vector & vector) {
    // shuffle
    for (std::size_t n = vector.size() - 1; n > 0; n--) {
        std::size_t rnd_idx = jstd::MtRandomGen::nextUInt32(static_cast<std::uint32_t>(n));
        std::swap(vector[n], vector[rnd_idx]);
    }
}

template <typename Container>
void print_test_time(std::size_t checksum, double elapsedTime)
{
    // printf("---------------------------------------------------------------------------\n");
    if (jstd::has_name<Container>::value)
        printf(" %-36s  ", jstd::call_name<Container>::name().c_str());
    else
        printf(" %-36s  ", "std::unordered_map<K, V>");
    printf("sum = %-10" PRIuPTR "  time: %8.3f ms\n", checksum, elapsedTime);
}

static void report_result(char const * title, double ut, std::size_t iters,
                          size_t start_memory, size_t end_memory) {
    // Construct heap growth report text if applicable
    char heap[128] = "";
    if (end_memory > start_memory) {
        snprintf(heap, sizeof(heap), "%7.1f MB",
                 (end_memory - start_memory) / 1048576.0);
    }

    printf("%-24s %8.2f ns  (%8" PRIuPTR " hashes, %8" PRIuPTR " copies, %8" PRIuPTR " ctor) %s\n",
           title, (ut * 1000000000.0 / iters),
           g_num_hashes, g_num_copies, g_num_constructor,
           heap);
    ::fflush(stdout);
}

template <class MapType, class Vector>
static void time_map_find(char const * title, std::size_t iters,
                          const Vector & indices) {
    typedef typename MapType::mapped_type mapped_type;

    MapType hashmap(kInitCapacity);
    StopWatch sw;
    std::size_t r;
    mapped_type i;
    mapped_type max_iters = static_cast<mapped_type>(iters);

    for (i = 0; i < max_iters; i++) {
        hashmap.emplace(i, i + 1);
    }

    r = 1;
    reset_counter();
    sw.start();
    for (i = 0; i < max_iters; i++) {
        r ^= static_cast<std::size_t>(hashmap.find(indices[i]) != hashmap.end());
    }
    sw.stop();
    double ut = sw.getElapsedSecond();

    ::srand(static_cast<unsigned int>(r));   // keep compiler from optimizing away r (we never call rand())
    report_result(title, ut, iters, 0, 0);
}

template <class MapType>
static void time_map_find_sequential(std::size_t iters) {
    typedef typename MapType::mapped_type mapped_type;

    mapped_type max_iters = static_cast<mapped_type>(iters);
    std::vector<mapped_type> v(iters);
    for (mapped_type i = 0; i < max_iters; i++) {
        v[i] = i + 1;
    }

    time_map_find<MapType>("map_find_sequential", iters, v);
}

template <class MapType>
static void time_map_find_random(std::size_t iters) {
    typedef typename MapType::mapped_type mapped_type;

    mapped_type max_iters = static_cast<mapped_type>(iters);
    std::vector<mapped_type> v(iters);
    for (mapped_type i = 0; i < max_iters; i++) {
        v[i] = i + 1;
    }

    shuffle_vector(v);

    time_map_find<MapType>("map_find_random", iters, v);
}

template <class MapType>
static void time_map_find_failed(std::size_t iters) {
    typedef typename MapType::mapped_type mapped_type;

    MapType hashmap(kInitCapacity);
    StopWatch sw;
    std::size_t r;
    mapped_type i;
    mapped_type max_iters = static_cast<mapped_type>(iters);

    for (i = 0; i < max_iters; i++) {
        hashmap.emplace(i, i + 1);
    }

    r = 1;
    reset_counter();
    sw.start();
    for (i = max_iters; i < max_iters * 2; i++) {
        r ^= static_cast<std::size_t>(hashmap.find(i) != hashmap.end());
    }
    sw.stop();
    double ut = sw.getElapsedSecond();

    ::srand(static_cast<unsigned int>(r));   // keep compiler from optimizing away r (we never call rand())
    report_result("map_find_failed", ut, iters, 0, 0);
}

template <class MapType>
static void time_map_find_empty(std::size_t iters) {
    typedef typename MapType::mapped_type mapped_type;

    MapType hashmap(kInitCapacity);
    StopWatch sw;
    std::size_t r;
    mapped_type i;
    mapped_type max_iters = static_cast<mapped_type>(iters);

    r = 1;
    reset_counter();
    sw.start();
    for (i = 0; i < max_iters; i++) {
        r ^= static_cast<std::size_t>(hashmap.find(i) != hashmap.end());
    }
    sw.stop();
    double ut = sw.getElapsedSecond();

    ::srand(static_cast<unsigned int>(r));   // keep compiler from optimizing away r (we never call rand())
    report_result("map_find_empty", ut, iters, 0, 0);
}

template <class MapType>
static void time_map_insert(std::size_t iters) {
    typedef typename MapType::mapped_type mapped_type;

    MapType hashmap(kInitCapacity);
    StopWatch sw;

    mapped_type max_iters = static_cast<mapped_type>(iters);
    const std::size_t start = GetCurrentMemoryUsage();

    reset_counter();
    sw.start();
    for (mapped_type i = 0; i < max_iters; i++) {
        hashmap.insert(std::make_pair(i, i + 1));
    }
    sw.stop();

    double ut = sw.getElapsedSecond();
    const std::size_t finish = GetCurrentMemoryUsage();
    report_result("map_insert", ut, iters, start, finish);
}

template <class MapType>
static void time_map_insert_predicted(std::size_t iters) {
    typedef typename MapType::mapped_type mapped_type;

    MapType hashmap(kInitCapacity);
    StopWatch sw;

    mapped_type max_iters = static_cast<mapped_type>(iters);
    const std::size_t start = GetCurrentMemoryUsage();

    hashmap.resize(iters);

    reset_counter();
    sw.start();
    for (mapped_type i = 0; i < max_iters; i++) {
        hashmap.insert(std::make_pair(i, i + 1));
    }
    sw.stop();

    double ut = sw.getElapsedSecond();
    const std::size_t finish = GetCurrentMemoryUsage();
    report_result("map_insert_predicted", ut, iters, start, finish);
}

template <class MapType>
static void time_map_insert_replace(std::size_t iters) {
    typedef typename MapType::mapped_type mapped_type;

    MapType hashmap(kInitCapacity);
    StopWatch sw;

    mapped_type max_iters = static_cast<mapped_type>(iters);
    for (mapped_type i = 0; i < max_iters; i++) {
        hashmap.insert(std::make_pair(i, i + 1));
    }

    const std::size_t start = GetCurrentMemoryUsage();

    reset_counter();
    sw.start();
    for (mapped_type i = 0; i < max_iters; i++) {
        hashmap.insert(std::make_pair(i, i + 2));
    }
    sw.stop();

    double ut = sw.getElapsedSecond();
    const std::size_t finish = GetCurrentMemoryUsage();
    report_result("map_insert_replace", ut, iters, start, finish);
}

template <class MapType>
static void time_map_emplace(std::size_t iters) {
    typedef typename MapType::mapped_type mapped_type;

    MapType hashmap(kInitCapacity);
    StopWatch sw;

    mapped_type max_iters = static_cast<mapped_type>(iters);
    const std::size_t start = GetCurrentMemoryUsage();

    reset_counter();
    sw.start();
    for (mapped_type i = 0; i < max_iters; i++) {
        hashmap.emplace(i, i + 1);
    }
    sw.stop();

    double ut = sw.getElapsedSecond();
    const std::size_t finish = GetCurrentMemoryUsage();
    report_result("map_emplace", ut, iters, start, finish);
}

template <class MapType>
static void time_map_emplace_predicted(std::size_t iters) {
    typedef typename MapType::mapped_type mapped_type;

    MapType hashmap(kInitCapacity);
    StopWatch sw;

    mapped_type max_iters = static_cast<mapped_type>(iters);
    const std::size_t start = GetCurrentMemoryUsage();

    hashmap.resize(iters);

    reset_counter();
    sw.start();
    for (mapped_type i = 0; i < max_iters; i++) {
        hashmap.emplace(i, i + 1);
    }
    sw.stop();

    double ut = sw.getElapsedSecond();
    const std::size_t finish = GetCurrentMemoryUsage();
    report_result("map_emplace_predicted", ut, iters, start, finish);
}

template <class MapType>
static void time_map_emplace_replace(std::size_t iters) {
    typedef typename MapType::mapped_type mapped_type;

    MapType hashmap(kInitCapacity);
    StopWatch sw;

    mapped_type max_iters = static_cast<mapped_type>(iters);
    for (mapped_type i = 0; i < max_iters; i++) {
        hashmap.emplace(i, i + 1);
    }

    const std::size_t start = GetCurrentMemoryUsage();

    reset_counter();
    sw.start();
    for (mapped_type i = 0; i < max_iters; i++) {
        hashmap.emplace(i, i + 2);
    }
    sw.stop();

    double ut = sw.getElapsedSecond();
    const std::size_t finish = GetCurrentMemoryUsage();
    report_result("map_emplace_replace", ut, iters, start, finish);
}

template <class MapType>
static void time_map_erase(std::size_t iters) {
    typedef typename MapType::mapped_type mapped_type;

    MapType hashmap(kInitCapacity);
    StopWatch sw;

    mapped_type max_iters = static_cast<mapped_type>(iters);
    for (mapped_type i = 0; i < max_iters; i++) {
        hashmap.emplace(i, i + 1);
    }

    const std::size_t start = GetCurrentMemoryUsage();

    reset_counter();
    sw.start();
    for (mapped_type i = 0; i < max_iters; i++) {
        hashmap.erase(i);
    }
    sw.stop();

    double ut = sw.getElapsedSecond();
    const std::size_t finish = GetCurrentMemoryUsage();
    report_result("map_erase", ut, iters, start, finish);
}

template <class MapType>
static void time_map_erase_failed(std::size_t iters) {
    typedef typename MapType::mapped_type mapped_type;

    MapType hashmap(kInitCapacity);
    StopWatch sw;

    mapped_type max_iters = static_cast<mapped_type>(iters);
    for (mapped_type i = 0; i < max_iters; i++) {
        hashmap.emplace(i, i + 1);
    }

    const std::size_t start = GetCurrentMemoryUsage();

    reset_counter();
    sw.start();
    for (mapped_type i = max_iters; i < max_iters * 2; i++) {
        hashmap.erase(i);
    }
    sw.stop();

    double ut = sw.getElapsedSecond();
    const std::size_t finish = GetCurrentMemoryUsage();
    report_result("map_erase_failed", ut, iters, start, finish);
}

template <class MapType>
static void time_map_toggle(std::size_t iters) {
    typedef typename MapType::mapped_type mapped_type;

    MapType hashmap(kInitCapacity);
    StopWatch sw;

    mapped_type max_iters = static_cast<mapped_type>(iters);
    const std::size_t start = GetCurrentMemoryUsage();

    reset_counter();
    sw.start();
    for (mapped_type i = 0; i < max_iters; i++) {
        hashmap.emplace(i, i + 1);
        hashmap.erase(i);
    }
    sw.stop();

    double ut = sw.getElapsedSecond();
    const std::size_t finish = GetCurrentMemoryUsage();
    report_result("map_toggle", ut, iters, start, finish);
}

template <class MapType>
static void time_map_iterate(std::size_t iters) {
    typedef typename MapType::mapped_type       mapped_type;
    typedef typename MapType::const_iterator    const_iterator;

    MapType hashmap(kInitCapacity);
    StopWatch sw;
    mapped_type r;

    mapped_type max_iters = static_cast<mapped_type>(iters);
    for (mapped_type i = 0; i < max_iters; i++) {
        hashmap.emplace(i, i + 1);
    }

    const std::size_t start = GetCurrentMemoryUsage();

    r = 1;
    reset_counter();
    sw.start();
    for (const_iterator it = hashmap.begin(), it_end = hashmap.end(); it != it_end; ++it) {
        r ^= it->second;
    }
    sw.stop();

    double ut = sw.getElapsedSecond();
    const std::size_t finish = GetCurrentMemoryUsage();
    ::srand(static_cast<unsigned int>(r));   // keep compiler from optimizing away r (we never call rand())
    report_result("map_iterate", ut, iters, start, finish);
}

template <class MapType>
static void stress_hash_function(std::size_t desired_insertions,
                                 std::size_t map_size,
                                 std::size_t stride) {
}

template <class MapType>
static void stress_hash_function(std::size_t num_inserts) {
    static const std::size_t kMapSizes [] = { 256, 1024 };
    std::size_t len = sizeof(kMapSizes) / sizeof(kMapSizes[0]);
    for (std::size_t i = 0; i < len; i++) {
        const std::size_t map_size = kMapSizes[i];
        for (std::size_t stride = 1; stride <= map_size; stride *= map_size) {
            stress_hash_function<MapType>(num_inserts, map_size, stride);
        }
    }
}

template <class MapType, class StressMapType>
static void measure_hashmap(const char * name, std::size_t obj_size, std::size_t iters,
                            bool is_stress_hash_function) {
    printf("%s (%" PRIuPTR " byte objects, %" PRIuPTR " iterations):\n", name, obj_size, iters);

    if (1) time_map_insert<MapType>(iters);
    if (1) time_map_insert_predicted<MapType>(iters);
    if (1) time_map_insert_replace<MapType>(iters);

    if (1) time_map_emplace<MapType>(iters);
    if (1) time_map_emplace_predicted<MapType>(iters);
    if (1) time_map_emplace_replace<MapType>(iters);

    if (1) time_map_find_sequential<MapType>(iters);
    if (1) time_map_find_random<MapType>(iters);
    if (1) time_map_find_failed<MapType>(iters);
    if (1) time_map_find_empty<MapType>(iters);

    if (1) time_map_erase<MapType>(iters);
    if (1) time_map_erase_failed<MapType>(iters);

    if (1) time_map_toggle<MapType>(iters);
    if (1) time_map_iterate<MapType>(iters);

    printf("\n");

    // This last test is useful only if the map type uses hashing.
    // And it's slow, so use fewer iterations.
    if (is_stress_hash_function) {
        // Blank line in the output makes clear that what follows isn't part of the
        // table of results that we just printed.
        stress_hash_function<StressMapType>(iters / 4);
        //printf("\n");
    }
}

template <typename HashObj, typename Value>
static void test_all_hashmaps(std::size_t obj_size, std::size_t iters) {
    const bool stress_hash_function = (obj_size <= 8);

    if (FLAGS_test_std_hash_map) {
        measure_hashmap<StdHashMap<HashObj,   Value, HashFn<HashObj>>,
                        StdHashMap<HashObj *, Value, HashFn<HashObj>>
                        >(
            "stdext::hash_map<K, V>", obj_size, iters, stress_hash_function);
    }

    if (FLAGS_test_std_unordered_map) {
        measure_hashmap<StdUnorderedMap<HashObj,   Value, HashFn<HashObj>>,
                        StdUnorderedMap<HashObj *, Value, HashFn<HashObj>>
                        >(
            "std::unordered_map<K, V>", obj_size, iters, stress_hash_function);
    }

    if (FLAGS_test_jstd_dictionary) {
        measure_hashmap<jstd::Dictionary<HashObj,   Value, HashFn<HashObj>>,
                        jstd::Dictionary<HashObj *, Value, HashFn<HashObj>>
                        >(
            "jstd::Dectionary<K, V>", obj_size, iters, stress_hash_function);
    }
}

void benchmark_all_hashmaps(std::size_t iters)
{
    // It would be nice to set these at run-time, but by setting them at
    // compile-time, we allow optimizations that make it as fast to use
    // a HashObject as it would be to use just a straight int/char
    // buffer.  To keep memory use similar, we normalize the number of
    // iterations based on size.
    if (FLAGS_test_4_bytes) {
        test_all_hashmaps<HashObject<std::uint32_t, 4, 4>, std::uint32_t>(4, iters / 1);
    }

    if (FLAGS_test_8_bytes) {
        test_all_hashmaps<HashObject<std::size_t, 8, 8>, std::size_t>(8, iters / 2);
    }

    if (FLAGS_test_16_bytes) {
        test_all_hashmaps<HashObject<std::size_t, 16, 16>, std::size_t>(16, iters / 4);
    }

    if (FLAGS_test_256_bytes) {
        test_all_hashmaps<HashObject<std::size_t, 256, 32>, std::size_t>(256, iters / 32);
    }
}

int main(int argc, char * argv[])
{
    jstd::MtRandomGen mtRandomGen(20200831);

    std::size_t iters = kDefaultIters;
    if (argc > 1) {
        // first arg is # of iterations
        iters = atoi(argv[1]);
    }

    const char * test_str = "abcdefghijklmnopqrstuvwxyz";
    uint32_t fnv1a_1 = hasher::FNV1A_Yoshimura(test_str, libc::StrLen(test_str));
    uint32_t fnv1a_2 = hasher::FNV1A_Yoshimitsu_TRIADii_XMM(test_str, libc::StrLen(test_str));
    uint32_t fnv1a_3 = hasher::FNV1A_penumbra(test_str, libc::StrLen(test_str));

    jtest::CPU::warmup(1000);

    benchmark_all_hashmaps(iters);

    jstd::Console::ReadKey();
    return 0;
}