
/************************************************************************************

  CC BY-SA 4.0 License

  Copyright (c) 2020-2022 XiongHui Guo (gz_shines@msn.com)

  https://github.com/shines77/jstd_hash_map
  https://gitee.com/shines77/jstd_hash_map

*************************************************************************************

  CC Attribution-ShareAlike 4.0 International

  https://creativecommons.org/licenses/by-sa/4.0/deed.en

  You are free to:

    1. Share -- copy and redistribute the material in any medium or format.

    2. Adapt -- remix, transforn, and build upon the material for any purpose,
    even commerically.

    The licensor cannot revoke these freedoms as long as you follow the license terms.

  Under the following terms:

    * Attribution -- You must give appropriate credit, provide a link to the license,
    and indicate if changes were made. You may do so in any reasonable manner,
    but not in any way that suggests the licensor endorses you or your use.

    * ShareAlike -- If you remix, transform, or build upon the material, you must
    distribute your contributions under the same license as the original.

    * No additional restrictions -- You may not apply legal terms or technological
    measures that legally restrict others from doing anything the license permits.

  Notices:

    * You do not have to comply with the license for elements of the material
    in the public domain or where your use is permitted by an applicable exception
    or limitation.

    * No warranties are given. The license may not give you all of the permissions
    necessary for your intended use. For example, other rights such as publicity,
    privacy, or moral rights may limit how you use the material.

************************************************************************************/

#ifdef _MSC_VER
#include <jstd/basic/vld.h>
#endif

#ifdef _MSC_VER
#ifndef __SSE4_2__
#define __SSE4_2__
#endif
#endif

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <inttypes.h>
#include <string.h>
#include <math.h>

#include <iostream>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <string>
#include <cstring>
#include <atomic>
#include <memory>
#include <utility>
#include <vector>
#include <unordered_map>
#include <algorithm>
#include <cassert>

#define USE_JSTD_HASH_TABLE     0
#define USE_JSTD_DICTIONARY     0

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

#ifdef __SSE4_2__

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
#define STRING_UTILS_LIBC       3

#define STRING_UTILS_MODE       STRING_UTILS_STL

#include <jstd/basic/stddef.h>
#include <jstd/basic/stdint.h>
#include <jstd/basic/inttypes.h>

#include <jstd/hash/dictionary.h>
#include <jstd/hash/hashmap_analyzer.h>
#include <jstd/hasher/hash_helper.h>
#include <jstd/string/string_view.h>
#include <jstd/string/string_view_array.h>
#include <jstd/system/Console.h>
#include <jstd/system/RandomGen.h>
#include <jstd/test/StopWatch.h>
#include <jstd/test/CPUWarmUp.h>
#include <jstd/test/ProcessMemInfo.h>

//
// HashTable performance benchmark (CK/phmap/ska)
//
// From: https://dirtysalt.github.io/html/hashtable-perf-comparison.html
//

#define ID_STD_HASH             0   // std::hash<T>
#define ID_SIMPLE_HASH          1   // test::SimpleHash<T>
#define ID_INTEGAL_HASH         2   // test::IntegalHash<T>

#ifdef _MSC_VER
#define HASH_FUNCTION_ID        ID_SIMPLE_HASH
#else
#define HASH_FUNCTION_ID        ID_STD_HASH
#endif

#if (HASH_FUNCTION_ID == ID_SIMPLE_HASH)
  #define HASH_MAP_FUNCTION     test::SimpleHash
#elif (HASH_FUNCTION_ID == ID_INTEGAL_HASH)
  #define HASH_MAP_FUNCTION     test::IntegalHash
#else
  #define HASH_MAP_FUNCTION     std::hash
#endif // HASH_FUNCTION_MODE

#define MACRO_TO_STRING(x)      #x
#define PRINT_MACRO(x)          MACRO_TO_STRING(x)
#define PRINT_MACRO_VAR(x)      #x " = " MACRO_TO_STRING(x)

#ifndef UINT64_High
#define UINT64_High(u64)        ((uint32_t)(u64 >> 32))
#endif

#ifndef UINT64_Low
#define UINT64_Low(u64)         ((uint32_t)(u64 & 0x00000000FFFFFFFFull))
#endif

#ifndef _DEBUG
static const std::size_t kDefaultIters = 10000000;
#else
static const std::size_t kDefaultIters = 10000;
#endif

static const std::size_t kInitCapacity = 8;

static inline
std::size_t CurrentMemoryUsage()
{
    return jtest::GetCurrentMemoryUsage();
}

namespace test {

template <typename T>
struct SimpleHash {
    typedef T           argument_type;
    typedef std::size_t result_type;

    template <typename Integer, typename std::enable_if<
                                (std::is_integral<Integer>::value &&
                                (sizeof(Integer) <= 8))>::type * = nullptr>
    result_type operator () (Integer value) const noexcept {
        result_type hash = static_cast<result_type>(value);
        return hash;
    }

    template <typename Argument, typename std::enable_if<
                                  (!std::is_integral<Argument>::value ||
                                  sizeof(Argument) > 8)>::type * = nullptr>
    result_type operator () (const Argument & value) const {
        std::hash<Argument> hasher;
        return static_cast<result_type>(hasher(value));
    }
};

template <typename T>
struct IntegalHash
{
    typedef T           argument_type;
    typedef std::size_t result_type;

    template <typename UInt32, typename std::enable_if<
                                (std::is_integral<UInt32>::value &&
                                (sizeof(UInt32) <= 4))>::type * = nullptr>
    result_type operator () (UInt32 value) const noexcept {
        result_type hash = (result_type)(((std::uint64_t)value * 2654435769ul) >> 12);
        return hash;
    }

    template <typename UInt64, typename std::enable_if<
                                (std::is_integral<UInt64>::value &&
                                (sizeof(UInt64) > 4 && sizeof(UInt64) <= 8))>::type * = nullptr>
    result_type operator () (UInt64 value) const noexcept {
        result_type hash = (result_type)(((std::uint64_t)value * 11400714819323198485ull) >> 28);
        return hash;
    }

    template <typename Argument, typename std::enable_if<
                                  (!std::is_integral<Argument>::value ||
                                  sizeof(Argument) > 8)>::type * = nullptr>
    result_type operator () (const Argument & value) const {
        std::hash<Argument> hasher;
        return static_cast<result_type>(hasher(value));
    }
};

} // namespace test

//
// HashTable performance benchmark (CK/phmap/ska)
//
// From: https://dirtysalt.github.io/html/hashtable-perf-comparison.html
//

class LogBuffer {
public:
    std::ostringstream & buf() {
        return oss_;
    }

    const std::ostringstream & buf() const {
        return oss_;
    }

    void flush() {
        std::cerr << oss_.str();
    }

    template <typename Container>
    void info(const std::string & name, const Container & hashmap) {
        oss_ << name << ": hashmap size = " << hashmap.size()
             << ", load factor = " << hashmap.load_factor() << '\n';
    }

    void info(const std::string & text) {
        oss_ << text << '\n';
    }

    void println() {
        oss_ << '\n';
    }

private:
    std::ostringstream oss_;
};

LogBuffer s_log;

template <typename T>
struct type_name {
    static const char * name() {
        return typeid(T).name();
    }
};

template <>
struct type_name<int> {
    static const char * name() {
        return "int";
    }
};

template <>
struct type_name<unsigned int> {
    static const char * name() {
        return "uint";
    }
};

template <>
struct type_name<std::int64_t> {
    static const char * name() {
        return "ssize_t";
    }
};

template <>
struct type_name<std::uint64_t> {
    static const char * name() {
        return "size_t";
    }
};

std::string formatMsTime(double fMillisec) {
    char time_buf[256];

    if (fMillisec >= 1000.0 * 60.0 * 30.0) {
        snprintf(time_buf, sizeof(time_buf), "%7.2f Min", fMillisec / (60.0 * 1000.0));
    }
    else if (fMillisec >= 1000.0 * 10.0) {
        snprintf(time_buf, sizeof(time_buf), "%7.2f Sec", fMillisec / 1000.0);
    }
    else if (fMillisec >= 1.0 * 1.0) {
        snprintf(time_buf, sizeof(time_buf), "%7.2f ms ", fMillisec);
    }
    else if (fMillisec >= 0.001 * 10.0) {
        snprintf(time_buf, sizeof(time_buf), "%7.2f us ", fMillisec * 1000.0);
    }
    else {
        snprintf(time_buf, sizeof(time_buf), "%7.2f ns ", fMillisec * 1000000.0);
    }

    return std::string(time_buf);
}

template <typename Key, typename Value>
std::string get_hashmap_name(const char * fmt)
{
    char name[1024];
    snprintf(name, sizeof(name), fmt,
             type_name<Key>::name(), type_name<Value>::name());
    return std::string(name);
}

template <typename Key>
std::vector<Key> generate_random_keys(std::size_t data_size, std::size_t key_range)
{
    jstd::MtRandomGen mtRandomGen(20200831);

    std::vector<Key> keys;
    keys.resize(data_size);
    for (std::size_t i = 0; i < data_size; i++) {
        keys[i] = static_cast<Key>(mtRandomGen.nextUInt() % key_range);
    }

    return keys;
}

template <typename HashMap, typename Key = typename HashMap::key_type>
void run_insert_random(const std::string & name, std::vector<Key> & keys, std::size_t cardinal)
{
    typedef typename HashMap::mapped_type mapped_type;

    jtest::StopWatch sw;
    HashMap hashmap;

    {
        sw.start();
        for (std::size_t i = 0; i < keys.size(); i++) {
            hashmap.insert(std::make_pair(keys[i], mapped_type(i)));
        }
        sw.stop();

        double elapsed_time = sw.getElapsedMillisec();

        printf("%s: %s\n", __func__, name.c_str());
        printf("hashmap.size() = %u, cardinal = %u, load_factor = %0.3f, time: %0.2f ms\n",
               (uint32_t)hashmap.size(), (uint32_t)cardinal,
               hashmap.load_factor(), elapsed_time);
    }

    {
        std::size_t check_sum = 0;
        sw.start();
        for (std::size_t i = 0; i < keys.size(); i++) {
            auto iter = hashmap.find(keys[i]);
            check_sum += iter->second;
        }
        sw.stop();

        double elapsed_time = sw.getElapsedMillisec();
        double average_time = elapsed_time / keys.size();

        printf("hashmap.find(key), check_sum: %" PRIuPTR ", average: %s, time: %0.2f ms\n\n",
               check_sum, formatMsTime(average_time).c_str(), elapsed_time);
    }
}

template <typename Key, typename Value>
void benchmark_insert_random(std::size_t iters)
{
    static constexpr std::size_t Factor = 16;
#ifndef _DEBUG
    static constexpr std::size_t DataSize = 1024 * 1000 * Factor;
    static constexpr std::size_t Cardinal0 = 60 * Factor;
    static constexpr std::size_t Cardinal1 = 600 * Factor;
    static constexpr std::size_t Cardinal2 = 6000 * Factor;
    static constexpr std::size_t Cardinal3 = 60000 * Factor;
    static constexpr std::size_t Cardinal4 = 600000 * Factor;
    static constexpr std::size_t Cardinal5 = 6000000 * Factor;
    static constexpr std::size_t Cardinal6 = 60000000 * Factor;
#else
    static constexpr std::size_t DataSize = 1024 * 10 * Factor;
    static constexpr std::size_t Cardinal0 = 6 * Factor;
    static constexpr std::size_t Cardinal1 = 60 * Factor;
    static constexpr std::size_t Cardinal2 = 600 * Factor;
    static constexpr std::size_t Cardinal3 = 6000 * Factor;
    static constexpr std::size_t Cardinal4 = 60000 * Factor;
    static constexpr std::size_t Cardinal5 = 600000 * Factor;
    static constexpr std::size_t Cardinal6 = 600000 * Factor;
#endif

    printf("DataSize = %u\n\n", (uint32_t)DataSize);

    std::string name0, name1;
    name0 = get_hashmap_name<Key, Value>("std::unordered_map<%s, %s>");
    name1 = get_hashmap_name<Key, Value>("jstd::Dictionary<%s, %s>");

    std::vector<Key> keys;

    keys = generate_random_keys<Key>(DataSize, Cardinal0);
    run_insert_random<std::unordered_map<Key, Value>>(name0, keys, Cardinal0);
    run_insert_random<jstd::Dictionary<Key, Value>>  (name1, keys, Cardinal0);

    keys = generate_random_keys<Key>(DataSize, Cardinal1);
    run_insert_random<std::unordered_map<Key, Value>>(name0, keys, Cardinal1);
    run_insert_random<jstd::Dictionary<Key, Value>>  (name1, keys, Cardinal1);

    keys = generate_random_keys<Key>(DataSize, Cardinal2);
    run_insert_random<std::unordered_map<Key, Value>>(name0, keys, Cardinal2);
    run_insert_random<jstd::Dictionary<Key, Value>>  (name1, keys, Cardinal2);

    keys = generate_random_keys<Key>(DataSize, Cardinal3);
    run_insert_random<std::unordered_map<Key, Value>>(name0, keys, Cardinal3);
    run_insert_random<jstd::Dictionary<Key, Value>>  (name1, keys, Cardinal3);

    keys = generate_random_keys<Key>(DataSize, Cardinal4);
    run_insert_random<std::unordered_map<Key, Value>>(name0, keys, Cardinal4);
    run_insert_random<jstd::Dictionary<Key, Value>>  (name1, keys, Cardinal4);

#if 0
    keys = generate_random_keys<Key>(DataSize, Cardinal5);
    run_insert_random<std::unordered_map<Key, Value>>(name0, keys, Cardinal5);
    run_insert_random<jstd::Dictionary<Key, Value>>  (name1, keys, Cardinal5);
#endif

    keys = generate_random_keys<Key>(DataSize, Cardinal6);
    run_insert_random<std::unordered_map<Key, Value>>(name0, keys, Cardinal6);
    run_insert_random<jstd::Dictionary<Key, Value>>  (name1, keys, Cardinal6);
}

void benchmark_all_hashmaps(std::size_t iters)
{
    benchmark_insert_random<int, int>(iters);

    printf("------------------------------------------------------------------------------------\n\n");

    benchmark_insert_random<std::size_t, std::size_t>(iters);
}

void std_hash_test()
{
    printf("#define HASH_MAP_FUNCTION = %s\n\n", PRINT_MACRO(HASH_MAP_FUNCTION));

    printf("std::hash<std::uint32_t>\n\n");
    for (std::uint32_t i = 0; i < 8; i++) {
        std::size_t hash_code = std::hash<std::uint32_t>()(i);
        printf("key = %3u, hash_code = 0x%08X%08X\n",
               i, UINT64_High(hash_code), UINT64_Low(hash_code));
    }
    printf("\n");

    printf("std::hash<std::uint64_t>\n\n");
    for (std::uint64_t i = 0; i < 8; i++) {
        std::size_t hash_code = std::hash<std::uint64_t>()(i);
        printf("key = %3" PRIu64 ", hash_code = 0x%08X%08X\n",
               i, UINT64_High(hash_code), UINT64_Low(hash_code));
    }
    printf("\n");

#if (HASH_FUNCTION_ID != ID_STD_HASH)
    printf("%s<std::uint32_t>\n\n", PRINT_MACRO(HASH_MAP_FUNCTION));
    for (std::uint32_t i = 0; i < 8; i++) {
        std::size_t hash_code = HASH_MAP_FUNCTION<std::uint32_t>()(i);
        printf("key = %3u, hash_code = 0x%08X%08X\n",
               i, UINT64_High(hash_code), UINT64_Low(hash_code));
    }
    printf("\n");

    printf("%s<std::uint64_t>\n\n", PRINT_MACRO(HASH_MAP_FUNCTION));
    for (std::uint64_t i = 0; i < 8; i++) {
        std::size_t hash_code = HASH_MAP_FUNCTION<std::uint64_t>()(i);
        printf("key = %3" PRIu64 ", hash_code = 0x%08X%08X\n",
               i, UINT64_High(hash_code), UINT64_Low(hash_code));
    }
    printf("\n");
#endif
}

int main(int argc, char * argv[])
{
    jstd::RandomGen   RandomGen(20200831);
    jstd::MtRandomGen mtRandomGen(20200831);

    std::size_t iters = kDefaultIters;
    if (argc > 1) {
        // first arg is # of iterations
        iters = ::atoi(argv[1]);
    }

    jtest::CPU::warm_up(1000);

    if (1) { std_hash_test(); }

    if (1)
    {
        printf("------------------------------ benchmark_all_hashmaps ------------------------------\n\n");
        benchmark_all_hashmaps(iters);
    }

    printf("------------------------------------------------------------------------------------\n\n");

    //jstd::Console::ReadKey();
    return 0;
}
