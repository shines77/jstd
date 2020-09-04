
// Enable Visual Leak Detector (For Visual Studio)
#define JSTD_ENABLE_VLD         1

#ifdef _MSC_VER
#include <jstd/basic/vld.h>
#endif

#ifndef __SSE4_2__
#define __SSE4_2__              1
#endif

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

//#include <jstd/all.h>

#include "BenchmarkResult.h"

using namespace jstd;
using namespace jtest;

static const bool FLAGS_test_sparse_hash_map = true;
static const bool FLAGS_test_dense_hash_map = true;
static const bool FLAGS_test_hash_map = true;
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
static std::size_t g_num_hashes;
static std::size_t g_num_copies;

static std::vector<std::string> dict_words;

static std::string dict_filename;
static bool dict_words_is_ready = false;

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

    static const std::size_t kMinSize = (std::max)(sizeof(Key), sizeof(int));
    static const std::size_t kSize = (std::max)(Size, kMinSize);
    static const std::size_t kHashSize = (std::min)((std::max)(HashSize, kMinSize), kSize);
    static const std::size_t kBufLen = (kSize > sizeof(Key)) ? (kSize - sizeof(Key)) : 0;
    static const std::size_t kHashLen = (kHashSize > sizeof(Key)) ? (kHashSize - sizeof(Key)) : 0;

private:
    key_type key_;   // the key used for hashing
    char buffer_[kBufLen];

public:
    HashObject() : key_(0) {
        ::memset(this->buffer_, 0, sizeof(this->buffer_));
    }
    HashObject(std::size_t key) : key_(key) {
        ::memset(this->buffer_, key & 0xFFUL, sizeof(this->buffer_));   // a "random" char
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
        return SPARSEHASH_HASH<int>()(hash_val);
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

    HashObject() : key_(0) {}
    HashObject(std::uint32_t key) : key_(key) {}
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

// A specialization for the case sizeof(buffer_) == 0
template <>
class HashObject<std::size_t, sizeof(std::size_t), sizeof(std::size_t)> {
public:
    typedef std::size_t     key_type;
    typedef HashObject<std::size_t, sizeof(std::size_t), sizeof(std::size_t)>
                            this_type;
private:
    std::size_t key_;   // the key used for hashing

    HashObject() : key_(0) {}
    HashObject(std::size_t key) : key_(key) {}
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

template <typename Key>
class HashFn {
public:
    // These two public members are required by msvc.  4 and 8 are defaults.
    static const std::size_t bucket_size = 4;
    static const std::size_t min_buckets = 8;

    template <std::size_t Size, std::size_t HashSize>
    std::size_t operator()(const HashObject<Key, Size, HashSize> & obj) const {
        return obj.Hash();
    }

    // Do the identity hash for pointers.
    template <std::size_t Size, std::size_t HashSize>
    std::size_t operator () (const HashObject<Key, Size, HashSize> * obj) const {
        return reinterpret_cast<std::uintptr_t>(obj);
    }

    // Less operator for MSVC's hash containers.
    template <std::size_t Size, std::size_t HashSize>
    bool operator () (const HashObject<Key, Size, HashSize> & a,
                    const HashObject<Key, Size, HashSize> & b) const {
        return (a < b);
    }

    template <std::size_t Size, std::size_t HashSize>
    bool operator () (const HashObject<Key, Size, HashSize> * a,
                    const HashObject<Key, Size, HashSize> * b) const {
        return (a < b);
    }
};

template <typename Key, typename Value, typename Hash>
class StdUnorderedMap : public std::unordered_map<Key, Value, Hash> {
public:
    // resize() is called rehash() in tr1
    void resize(std::size_t newSize) {
        this->rehash(newSize);
    }
};

template <typename Key, typename Value>
void print_error(std::size_t index, const Key & key, const Value & value)
{
    std::string skey   = std::to_string(key);
    std::string svalue = std::to_string(value);

    printf("[%6" PRIuPTR "]: key = \"%s\", value = \"%s\"\n",
           index + 1, skey.c_str(), svalue.c_str());
}

template <>
void print_error(std::size_t index, const std::string & key, const std::string & value)
{
    printf("[%6" PRIuPTR "]: key = \"%s\", value = \"%s\"\n",
           index + 1, key.c_str(), value.c_str());
}

template <>
void print_error(std::size_t index, const jstd::string_view & key, const jstd::string_view & value)
{
    std::string skey   = key.to_string();
    std::string svalue = value.to_string();

    printf("[%6" PRIuPTR "]: key = \"%s\", value = \"%s\"\n",
           index + 1, skey.c_str(), svalue.c_str());
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

template <class MapType, class StressMapType>
static void measure_hashmap(const char * name, std::size_t obj_size, std::size_t iters,
                            bool is_stress_hash_function) {
    printf("\n%s (%" PRIuPTR " byte objects, %" PRIuPTR " iterations):\n", name, obj_size, iters);
    if (1) time_map_grow<MapType>(iters);
    if (1) time_map_grow_predicted<MapType>(iters);
    if (1) time_map_replace<MapType>(iters);
    if (1) time_map_fetch_random<MapType>(iters);
    if (1) time_map_fetch_sequential<MapType>(iters);
    if (1) time_map_fetch_empty<MapType>(iters);
    if (1) time_map_remove<MapType>(iters);
    if (1) time_map_toggle<MapType>(iters);
    if (1) time_map_iterate<MapType>(iters);

    // This last test is useful only if the map type uses hashing.
    // And it's slow, so use fewer iterations.
    if (is_stress_hash_function) {
        // Blank line in the output makes clear that what follows isn't part of the
        // table of results that we just printed.
        puts("");
        stress_hash_function<StressMapType>(iters / 4);
    }
}

template <typename HashObj, typename Value>
static void test_all_hashmaps(std::size_t obj_size, std::size_t iters) {
    typedef typename HashObj::key_type key_type;

    const bool stress_hash_function = (obj_size <= 8);

    if (FLAGS_test_hash_map) {
        measure_hashmap<StdUnorderedMap<key_type, Value, HashFn>,
                        StdUnorderedMap<key_type *, Value, HashFn>>(
            "std::unordered_map<K, V>", obj_size, iters, stress_hash_function);
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

bool read_dict_words(const std::string & filename)
{
    bool is_ok = false;
    try {
        std::ifstream dict(filename.c_str());

        if (dict.is_open()) {
            std::string word;
            while (!dict.eof()) {
                char buf[256];
                dict.getline(buf, sizeof(buf));
                word = buf;
                dict_words.push_back(word);
            }

            is_ok = true;
            dict.close();
        }
    }
    catch (const std::exception & ex) {
        std::cout << "read_dict_file() Exception: " << ex.what() << std::endl << std::endl;
        is_ok = false;
    }
    return is_ok;
}

int main(int argc, char * argv[])
{
    jstd::MtRandomGen mtRandomGen(20200831);

    std::size_t iters = kDefaultIters;
    if (argc == 2) {
        std::string filename = argv[1];
        bool read_ok = read_dict_words(filename);
        dict_words_is_ready = read_ok;
        dict_filename = filename;
    }

    jtest::CPU::warmup(1000);

    benchmark_all_hashmaps();

    jstd::Console::ReadKey();
    return 0;
}
