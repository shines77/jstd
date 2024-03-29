
// Your can edit 'JSTD_ENABLE_VLD' marco in <jstd/basic/vld_def.h> file
// to switch Visual Leak Detector(vld).
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
#include <map>
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

// Use in <jstd/support/Power2.h>
#define JSTD_SUPPORT_X86_BITSCAN_INSTRUCTION    1

#define USE_JSTD_HASH_TABLE     0
#define USE_JSTD_DICTIONARY     0

#include <jstd/basic/stddef.h>
#include <jstd/basic/stdint.h>
#include <jstd/basic/inttypes.h>

#include <jstd/hash/hash_table.h>
#include <jstd/hash/dictionary.h>
#include <jstd/hash/hashmap_analyzer.h>
#include <jstd/string/string_view.h>
#include <jstd/memory/shiftable_ptr.h>
#include <jstd/system/Console.h>
#include <jstd/system/RandomGen.h>
#include <jstd/test/StopWatch.h>
#include <jstd/test/CPUWarmUp.h>
#include <jstd/test/ProcessMemInfo.h>

#include <jstd/hasher/fnv1a.h>
#include <jstd/string/formatter.h>
#include <jstd/string/snprintf.hpp>

#include <jstd/memory/c_aligned_malloc.h>

static std::vector<std::string> dict_words;

static std::string dict_filename;
static bool dict_words_is_ready = false;

static const std::size_t kInitCapacity = 16;

#ifdef NDEBUG
static const std::size_t kIterations = 3000000;
#else
static const std::size_t kIterations = 1000;
#endif

//
// See: https://blog.csdn.net/janekeyzheng/article/details/42419407
//
static const char * header_fields[] = {
    // Request
    "Accept",
    "Accept-Charset",
    "Accept-Encoding",
    "Accept-Language",
    "Authorization",
    "Cache-Control",
    "Connection",
    "Cookie",
    "Content-Length",
    "Content-MD5",
    "Content-Type",
    "Date",  // <-- repeat
    "DNT",
    "From",
    "Front-End-Https",
    "Host",
    "If-Match",
    "If-Modified-Since",
    "If-None-Match",
    "If-Range",
    "If-Unmodified-Since",
    "Max-Forwards",
    "Pragma",
    "Proxy-Authorization",
    "Range",
    "Referer",
    "User-Agent",
    "Upgrade",
    "Via",  // <-- repeat
    "Warning",
    "X-ATT-DeviceId",
    "X-Content-Type-Options",
    "X-Forwarded-For",
    "X-Forwarded-Proto",
    "X-Powered-By",
    "X-Requested-With",
    "X-XSS-Protection",

    // Response
    "Access-Control-Allow-Origin",
    "Accept-Ranges",
    "Age",
    "Allow",
    //"Cache-Control",
    //"Connection",
    "Content-Encoding",
    "Content-Language",
    //"Content-Length",
    "Content-Disposition",
    //"Content-MD5",
    "Content-Range",
    //"Content-Type",
    "Date",  // <-- repeat
    "ETag",
    "Expires",
    "Last-Modified",
    "Link",
    "Location",
    "P3P",
    "Proxy-Authenticate",
    "Refresh",
    "Retry-After",
    "Server",
    "Set-Cookie",
    "Strict-Transport-Security",
    "Trailer",
    "Transfer-Encoding",
    "Vary",
    "Via",  // <-- repeat
    "WWW-Authenticate",
    //"X-Content-Type-Options",
    //"X-Powered-By",
    //"X-XSS-Protection",

    "Last"
};

static const size_t kHeaderFieldSize = sizeof(header_fields) / sizeof(char *);

namespace std {

template <>
struct hash<jstd::string_view> {
    typedef std::uint32_t   result_type;

    jstd::string_hash_helper<jstd::string_view, std::uint32_t, jstd::HashFunc_CRC32C> hash_helper_;

    result_type operator()(const jstd::string_view & key) const {
        return hash_helper_.getHashCode(key);
    }
};

} // namespace std

namespace jstd {

template <>
struct hash<jstd::string_view, std::uint32_t, jstd::HashFunc_CRC32C> {
    typedef std::uint32_t   result_type;

    jstd::string_hash_helper<jstd::string_view, std::uint32_t, jstd::HashFunc_CRC32C> hash_helper_;

    result_type operator()(const jstd::string_view & key) const {
        return hash_helper_.getHashCode(key);
    }
};

template <>
struct hash<jstd::string_view, std::uint32_t, jstd::HashFunc_Time31> {
    typedef std::uint32_t   result_type;

    jstd::string_hash_helper<jstd::string_view, std::uint32_t, jstd::HashFunc_Time31> hash_helper_;

    result_type operator()(const jstd::string_view & key) const {
        return hash_helper_.getHashCode(key);
    }
};

template <>
struct hash<jstd::string_view, std::uint32_t, jstd::HashFunc_Time31Std> {
    typedef std::uint32_t   result_type;

    jstd::string_hash_helper<jstd::string_view, std::uint32_t, jstd::HashFunc_Time31Std> hash_helper_;

    result_type operator()(const jstd::string_view & key) const {
        return hash_helper_.getHashCode(key);
    }
};

} // namespace jstd

namespace test {

template <typename Key, typename Value>
class std_map {
public:
    typedef std::map<Key, Value>                map_type;
    typedef Key                                 key_type;
    typedef Value                               value_type;
    typedef typename map_type::size_type        size_type;
    typedef typename map_type::iterator         iterator;
    typedef typename map_type::const_iterator   const_iterator;

private:
    map_type map_;

public:
    std_map(size_type capacity = kInitCapacity) {
    }
    ~std_map() {}

    static const char * name() {
        return "std::map<K, V>";
    }

    bool is_hashtable() {
        return false;
    }

    iterator begin() {
        return this->map_.begin();
    }

    iterator end() {
        return this->map_.end();
    }

    const_iterator begin() const {
        return this->map_.begin();
    }

    const_iterator end() const {
        return this->map_.end();
    }

    size_type size() const {
        return this->map_.size();
    }

    bool empty() const {
        return this->map_.empty();
    }

    size_type count(const key_type & key) const {
        return this->map_.count(key);
    }

    void clear() {
        this->map_.clear();
    }

    void reserve(size_type new_capacity) {
        // Not implemented
    }

    void rehash(size_type new_capacity) {
        // Not implemented
    }

    void shrink_to_fit(size_type new_capacity) {
        // Not implemented
    }

    iterator find(const key_type & key) {
        return this->map_.find(key);
    }

    void insert(const key_type & key, const value_type & value) {
        this->map_.insert(std::make_pair(key, value));
    }

    void insert(key_type && key, value_type && value) {
        this->map_.insert(std::make_pair(std::forward<key_type>(key),
                                         std::forward<value_type>(value)));
    }

    template <typename ...Args>
    void emplace(Args && ... args) {
        this->map_.emplace(std::forward<Args>(args)...);
    }

    size_type erase(const key_type & key) {
        return this->map_.erase(key);
    }
};

template <typename Key, typename Value>
class std_unordered_map {
public:
    typedef std::unordered_map<Key, Value>      map_type;
    typedef Key                                 key_type;
    typedef Value                               value_type;
    typedef typename map_type::size_type        size_type;
    typedef typename map_type::iterator         iterator;
    typedef typename map_type::const_iterator   const_iterator;

private:
    map_type map_;

public:
    std_unordered_map(size_type capacity = kInitCapacity) : map_(capacity) {}
    ~std_unordered_map() {}

    static const char * name() {
        return "std::unordered_map<K, V>";
    }

    bool is_hashtable() {
        return true;
    }

    iterator begin() {
        return this->map_.begin();
    }

    iterator end() {
        return this->map_.end();
    }

    const_iterator begin() const {
        return this->map_.begin();
    }

    const_iterator end() const {
        return this->map_.end();
    }

    size_type size() const {
        return this->map_.size();
    }

    bool empty() const {
        return this->map_.empty();
    }

    size_type bucket_count() const {
        return this->map_.bucket_count();
    }

    size_type count(const key_type & key) const {
        return this->map_.count(key);
    }

    void clear() {
        this->map_.clear();
    }

    void reserve(size_type max_count) {
        this->map_.reserve(max_count);
    }

    void rehash(size_type new_buckets) {
        this->map_.rehash(new_buckets);
    }

    void shrink_to_fit(size_type new_buckets) {
        this->map_.rehash(new_buckets);
    }

    iterator find(const key_type & key) {
        return this->map_.find(key);
    }

    void insert(const key_type & key, const value_type & value) {
        this->map_.insert(std::make_pair(key, value));
    }

    void insert(key_type && key, value_type && value) {
        this->map_.insert(std::make_pair(std::forward<key_type>(key),
                                         std::forward<value_type>(value)));
    }

    template <typename ...Args>
    void emplace(Args && ... args) {
        this->map_.emplace(std::forward<Args>(args)...);
    }

    size_type erase(const key_type & key) {
        return this->map_.erase(key);
    }
};

template <typename T>
class hash_table_impl {
public:
    typedef T                                   map_type;
    typedef typename map_type::key_type         key_type;
    typedef typename map_type::mapped_type      value_type;
    typedef typename map_type::size_type        size_type;
    typedef typename map_type::iterator         iterator;
    typedef typename map_type::const_iterator   const_iterator;
    typedef std::pair<iterator, bool>           insert_return_type;

private:
    map_type map_;

public:
    hash_table_impl(size_type capacity = kInitCapacity) : map_(capacity) {}
    ~hash_table_impl() {}

    static const char * name() {
        return map_type::name();
    }

    bool is_hashtable() {
        return true;
    }

    iterator begin() {
        return this->map_.begin();
    }

    iterator end() {
        return this->map_.end();
    }

    const_iterator begin() const {
        return this->map_.begin();
    }

    const_iterator end() const {
        return this->map_.end();
    }

    size_type size() const {
        return this->map_.size();
    }

    bool empty() const {
        return this->map_.empty();
    }

    size_type bucket_mask() const {
        return this->map_.bucket_mask();
    }

    size_type bucket_count() const {
        return this->map_.bucket_count();
    }

    void clear() {
        this->map_.clear();
    }

    void reserve(size_type new_buckets) {
        this->map_.reserve(new_buckets);
    }

    void rehash(size_type new_buckets) {
        this->map_.rehash(new_buckets);
    }

    void shrink_to_fit(size_type new_buckets) {
        this->map_.shrink_to_fit(new_buckets);
    }

    iterator find(const key_type & key) {
        return this->map_.find(key);
    }

    void insert(const key_type & key, const value_type & value) {
        this->map_.insert(key, value);
    }

    void insert(const key_type & key, value_type && value) {
        this->map_.insert(key, std::forward<value_type>(value));
    }

    void insert(key_type && key, value_type && value) {
        this->map_.insert(std::forward<key_type>(key),
                          std::forward<value_type>(value));
    }

    template <typename ...Args>
    void emplace(Args && ... args) {
        this->map_.emplace(std::forward<Args>(args)...);
    }

    size_type erase(const key_type & key) {
        return this->map_.erase(key);
    }
};

} // namespace test

template <typename AlgorithmTy>
void hashtable_find_benchmark()
{
    static const size_t kRepeatTimes = (kIterations / kHeaderFieldSize);

    std::string field_str[kHeaderFieldSize];
    std::string index_str[kHeaderFieldSize];
    for (size_t i = 0; i < kHeaderFieldSize; ++i) {
        field_str[i].assign(header_fields[i]);
        index_str[i] = std::to_string(i);
    }

    {
        typedef typename AlgorithmTy::iterator iterator;

        size_t checksum = 0;
        AlgorithmTy algorithm;
        for (size_t i = 0; i < kHeaderFieldSize; ++i) {
            algorithm.emplace(field_str[i], index_str[i]);
        }

        jtest::StopWatch sw;
        sw.start();
        for (size_t i = 0; i < kRepeatTimes; ++i) {
            for (size_t j = 0; j < kHeaderFieldSize; ++j) {
                iterator iter = algorithm.find(field_str[j]);
                if (iter != algorithm.end()) {
                    checksum++;
                }
            }
        }
        sw.stop();

        printf("---------------------------------------------------------------------------\n");
        printf(" %-36s  ", AlgorithmTy::name());
        printf("sum = %-10" PRIuPTR "  time: %8.3f ms\n", checksum, sw.getElapsedMillisec());
    }
}

void hashtable_find_benchmark()
{
    std::cout << "  hashtable_find_benchmark()" << std::endl;
    std::cout << std::endl;

    //hashtable_find_benchmark<test::std_map<std::string, std::string>>();
    hashtable_find_benchmark<test::std_unordered_map<std::string, std::string>>();

#if USE_JSTD_HASH_TABLE
    hashtable_find_benchmark<test::hash_table_impl<jstd::hash_table<std::string, std::string>>>();
    hashtable_find_benchmark<test::hash_table_impl<jstd::hash_table_time31<std::string, std::string>>>();
    hashtable_find_benchmark<test::hash_table_impl<jstd::hash_table_time31_std<std::string, std::string>>>();
#endif

#if USE_JSTD_DICTIONARY
    hashtable_find_benchmark<test::hash_table_impl<jstd::Dictionary<std::string, std::string>>>();
    hashtable_find_benchmark<test::hash_table_impl<jstd::Dictionary_Time31<std::string, std::string>>>();
    hashtable_find_benchmark<test::hash_table_impl<jstd::Dictionary_Time31Std<std::string, std::string>>>();
#else
    hashtable_find_benchmark<test::hash_table_impl<jstd::Dictionary<std::string, std::string>>>();
#endif

    printf("---------------------------------------------------------------------------\n");
    printf("\n");
}

template <typename AlgorithmTy>
void hashtable_insert_benchmark_impl()
{
    static const size_t kRepeatTimes = (kIterations / kHeaderFieldSize);

    std::string field_str[kHeaderFieldSize];
    std::string index_str[kHeaderFieldSize];
    for (size_t i = 0; i < kHeaderFieldSize; ++i) {
        field_str[i].assign(header_fields[i]);
        index_str[i] = std::to_string(i);
    }

    {
        size_t checksum = 0;
        double totalTime = 0.0;
        jtest::StopWatch sw;

        for (size_t i = 0; i < kRepeatTimes; ++i) {
            AlgorithmTy algorithm(kInitCapacity);
            sw.start();
            for (size_t j = 0; j < kHeaderFieldSize; ++j) {
                algorithm.insert(field_str[j], index_str[j]);
            }
            checksum += algorithm.size();
            sw.stop();

            totalTime += sw.getElapsedMillisec();
        }

        printf("---------------------------------------------------------------------------\n");
        printf(" %-36s  ", AlgorithmTy::name());
        printf("sum = %-10" PRIuPTR "  time: %8.3f ms\n", checksum, totalTime);
    }
}

void hashtable_insert_benchmark()
{
    std::cout << "  hashtable_insert_benchmark()" << std::endl;
    std::cout << std::endl;

    //hashtable_insert_benchmark_impl<test::std_map<std::string, std::string>>();
    hashtable_insert_benchmark_impl<test::std_unordered_map<std::string, std::string>>();

#if USE_JSTD_HASH_TABLE
    hashtable_insert_benchmark_impl<test::hash_table_impl<jstd::hash_table<std::string, std::string>>>();
    hashtable_insert_benchmark_impl<test::hash_table_impl<jstd::hash_table_time31<std::string, std::string>>>();
    hashtable_insert_benchmark_impl<test::hash_table_impl<jstd::hash_table_time31_std<std::string, std::string>>>();
#endif

#if USE_JSTD_DICTIONARY
    hashtable_insert_benchmark_impl<test::hash_table_impl<jstd::Dictionary<std::string, std::string>>>();
    hashtable_insert_benchmark_impl<test::hash_table_impl<jstd::Dictionary_Time31<std::string, std::string>>>();
    hashtable_insert_benchmark_impl<test::hash_table_impl<jstd::Dictionary_Time31Std<std::string, std::string>>>();
#else
    hashtable_insert_benchmark_impl<test::hash_table_impl<jstd::Dictionary<std::string, std::string>>>();
#endif

    printf("---------------------------------------------------------------------------\n");
    printf("\n");
}

template <typename AlgorithmTy>
void hashtable_emplace_benchmark_impl()
{
    static const size_t kRepeatTimes = (kIterations / kHeaderFieldSize);

    std::string field_str[kHeaderFieldSize];
    std::string index_str[kHeaderFieldSize];
    for (size_t i = 0; i < kHeaderFieldSize; ++i) {
        field_str[i].assign(header_fields[i]);
        index_str[i] = std::to_string(i);
    }

    {
        size_t checksum = 0;
        double totalTime = 0.0;
        jtest::StopWatch sw;

        for (size_t i = 0; i < kRepeatTimes; ++i) {
            AlgorithmTy algorithm(kInitCapacity);
            sw.start();
            for (size_t j = 0; j < kHeaderFieldSize; ++j) {
                algorithm.emplace(field_str[j], index_str[j]);
            }
            checksum += algorithm.size();
            sw.stop();

            totalTime += sw.getElapsedMillisec();
        }

        printf("---------------------------------------------------------------------------\n");
        printf(" %-36s  ", AlgorithmTy::name());
        printf("sum = %-10" PRIuPTR "  time: %8.3f ms\n", checksum, totalTime);
    }
}

void hashtable_emplace_benchmark()
{
    std::cout << "  hashtable_emplace_benchmark()" << std::endl;
    std::cout << std::endl;

    //hashtable_emplace_benchmark_impl<test::std_map<std::string, std::string>>();
    hashtable_emplace_benchmark_impl<test::std_unordered_map<std::string, std::string>>();

#if USE_JSTD_HASH_TABLE
    hashtable_emplace_benchmark_impl<test::hash_table_impl<jstd::hash_table<std::string, std::string>>>();
    hashtable_emplace_benchmark_impl<test::hash_table_impl<jstd::hash_table_time31<std::string, std::string>>>();
    hashtable_emplace_benchmark_impl<test::hash_table_impl<jstd::hash_table_time31_std<std::string, std::string>>>();
#endif

#if USE_JSTD_DICTIONARY
    hashtable_emplace_benchmark_impl<test::hash_table_impl<jstd::Dictionary<std::string, std::string>>>();
    hashtable_emplace_benchmark_impl<test::hash_table_impl<jstd::Dictionary_Time31<std::string, std::string>>>();
    hashtable_emplace_benchmark_impl<test::hash_table_impl<jstd::Dictionary_Time31Std<std::string, std::string>>>();
#else
    hashtable_emplace_benchmark_impl<test::hash_table_impl<jstd::Dictionary<std::string, std::string>>>();
#endif

    printf("---------------------------------------------------------------------------\n");
    printf("\n");
}

template <typename AlgorithmTy>
void hashtable_erase_benchmark_impl()
{
    static const size_t kRepeatTimes = (kIterations / kHeaderFieldSize);

    std::string field_str[kHeaderFieldSize];
    std::string index_str[kHeaderFieldSize];
    for (size_t i = 0; i < kHeaderFieldSize; ++i) {
        field_str[i].assign(header_fields[i]);
        index_str[i] = std::to_string(i);
    }

    {
        size_t checksum = 0;
        double totalTime = 0.0;
        jtest::StopWatch sw;

        for (size_t i = 0; i < kRepeatTimes; ++i) {
            AlgorithmTy algorithm;

            for (size_t j = 0; j < kHeaderFieldSize; ++j) {
                algorithm.emplace(field_str[j], index_str[j]);
            }
            checksum += algorithm.size();

            sw.start();
            for (size_t j = 0; j < kHeaderFieldSize; ++j) {
                size_t counts = algorithm.erase(field_str[j]);
            }
            sw.stop();

            assert(algorithm.size() == 0);
            checksum += algorithm.size();

            totalTime += sw.getElapsedMillisec();
        }

        printf("---------------------------------------------------------------------------\n");
        printf(" %-36s  ", AlgorithmTy::name());
        printf("sum = %-10" PRIuPTR "  time: %8.3f ms\n", checksum, totalTime);
    }
}

void hashtable_erase_benchmark()
{
    std::cout << "  hashtable_erase_benchmark()" << std::endl;
    std::cout << std::endl;

    //hashtable_erase_benchmark_impl<test::std_map<std::string, std::string>>();
    hashtable_erase_benchmark_impl<test::std_unordered_map<std::string, std::string>>();

#if USE_JSTD_HASH_TABLE
    hashtable_erase_benchmark_impl<test::hash_table_impl<jstd::hash_table<std::string, std::string>>>();
    hashtable_erase_benchmark_impl<test::hash_table_impl<jstd::hash_table_time31<std::string, std::string>>>();
    hashtable_erase_benchmark_impl<test::hash_table_impl<jstd::hash_table_time31_std<std::string, std::string>>>();
#endif

#if USE_JSTD_DICTIONARY
    hashtable_erase_benchmark_impl<test::hash_table_impl<jstd::Dictionary<std::string, std::string>>>();
    hashtable_erase_benchmark_impl<test::hash_table_impl<jstd::Dictionary_Time31<std::string, std::string>>>();
    hashtable_erase_benchmark_impl<test::hash_table_impl<jstd::Dictionary_Time31Std<std::string, std::string>>>();
#else
    hashtable_erase_benchmark_impl<test::hash_table_impl<jstd::Dictionary<std::string, std::string>>>();
#endif

    printf("---------------------------------------------------------------------------\n");
    printf("\n");
}

template <typename AlgorithmTy>
void hashtable_insert_erase_benchmark_impl()
{
    static const size_t kRepeatTimes = (kIterations / kHeaderFieldSize);

    std::string field_str[kHeaderFieldSize];
    std::string index_str[kHeaderFieldSize];
    for (size_t i = 0; i < kHeaderFieldSize; ++i) {
        field_str[i].assign(header_fields[i]);
        index_str[i] = std::to_string(i);
    }

    {
        size_t checksum = 0;
        AlgorithmTy algorithm;

        jtest::StopWatch sw;

        sw.start();
        for (size_t i = 0; i < kRepeatTimes; ++i) {
            for (size_t j = 0; j < kHeaderFieldSize; ++j) {
                algorithm.emplace(field_str[j], index_str[j]);
            }
            checksum += algorithm.size();

            for (size_t j = 0; j < kHeaderFieldSize; ++j) {
                algorithm.erase(field_str[j]);
            }
            assert(algorithm.size() == 0);
            checksum += algorithm.size();
        }
        sw.stop();

        printf("---------------------------------------------------------------------------\n");
        printf(" %-36s  ", AlgorithmTy::name());
        printf("sum = %-10" PRIuPTR "  time: %8.3f ms\n", checksum, sw.getElapsedMillisec());
    }
}

void hashtable_insert_erase_benchmark()
{
    std::cout << "  hashtable_insert_erase_benchmark()" << std::endl;
    std::cout << std::endl;

    //hashtable_insert_erase_benchmark_impl<test::std_map<std::string, std::string>>();
    hashtable_insert_erase_benchmark_impl<test::std_unordered_map<std::string, std::string>>();

#if USE_JSTD_HASH_TABLE
    hashtable_insert_erase_benchmark_impl<test::hash_table_impl<jstd::hash_table<std::string, std::string>>>();
    hashtable_insert_erase_benchmark_impl<test::hash_table_impl<jstd::hash_table_time31<std::string, std::string>>>();
    hashtable_insert_erase_benchmark_impl<test::hash_table_impl<jstd::hash_table_time31_std<std::string, std::string>>>();
#endif

#if USE_JSTD_DICTIONARY
    hashtable_insert_erase_benchmark_impl<test::hash_table_impl<jstd::Dictionary<std::string, std::string>>>();
    hashtable_insert_erase_benchmark_impl<test::hash_table_impl<jstd::Dictionary_Time31<std::string, std::string>>>();
    hashtable_insert_erase_benchmark_impl<test::hash_table_impl<jstd::Dictionary_Time31Std<std::string, std::string>>>();
#else
    hashtable_insert_erase_benchmark_impl<test::hash_table_impl<jstd::Dictionary<std::string, std::string>>>();
#endif

    printf("---------------------------------------------------------------------------\n");
    printf("\n");
}

template <typename AlgorithmTy>
void hashtable_ref_find_benchmark()
{
    static const size_t kRepeatTimes = (kIterations / kHeaderFieldSize);

    std::string index_buf[kHeaderFieldSize];
    jstd::string_view field_str[kHeaderFieldSize];
    jstd::string_view index_str[kHeaderFieldSize];
    for (size_t i = 0; i < kHeaderFieldSize; ++i) {
        field_str[i].attach(header_fields[i]);
        index_buf[i] = std::to_string(i);
        index_str[i] = index_buf[i];
    }

    {
        typedef typename AlgorithmTy::iterator iterator;

        size_t checksum = 0;
        AlgorithmTy algorithm;
        for (size_t i = 0; i < kHeaderFieldSize; ++i) {
            algorithm.emplace(field_str[i], index_str[i]);
        }

        jtest::StopWatch sw;
        sw.start();
        for (size_t i = 0; i < kRepeatTimes; ++i) {
            for (size_t j = 0; j < kHeaderFieldSize; ++j) {
                iterator iter = algorithm.find(field_str[j]);
                if (iter != algorithm.end()) {
                    checksum++;
                }
            }
        }
        sw.stop();

        printf("---------------------------------------------------------------------------\n");
        printf(" %-36s  ", AlgorithmTy::name());
        printf("sum = %-10" PRIuPTR "  time: %8.3f ms\n", checksum, sw.getElapsedMillisec());
    }
}

void hashtable_ref_find_benchmark()
{
    std::cout << "  hashtable_ref_find_benchmark()" << std::endl;
    std::cout << std::endl;

    //hashtable_ref_find_benchmark<test::std_map<jstd::string_view, jstd::string_view>>();
    hashtable_ref_find_benchmark<test::std_unordered_map<jstd::string_view, jstd::string_view>>();

#if USE_JSTD_HASH_TABLE
    hashtable_ref_find_benchmark<test::hash_table_impl<jstd::hash_table<jstd::string_view, jstd::string_view>>>();
    hashtable_ref_find_benchmark<test::hash_table_impl<jstd::hash_table_time31<jstd::string_view, jstd::string_view>>>();
    hashtable_ref_find_benchmark<test::hash_table_impl<jstd::hash_table_time31_std<jstd::string_view, jstd::string_view>>>();
#endif

#if USE_JSTD_DICTIONARY
    hashtable_ref_find_benchmark<test::hash_table_impl<jstd::Dictionary<jstd::string_view, jstd::string_view>>>();
    hashtable_ref_find_benchmark<test::hash_table_impl<jstd::Dictionary_Time31<jstd::string_view, jstd::string_view>>>();
    hashtable_ref_find_benchmark<test::hash_table_impl<jstd::Dictionary_Time31Std<jstd::string_view, jstd::string_view>>>();
#else
    hashtable_ref_find_benchmark<test::hash_table_impl<jstd::Dictionary<jstd::string_view, jstd::string_view>>>();
#endif

    printf("---------------------------------------------------------------------------\n");
    printf("\n");
}

template <typename AlgorithmTy>
void hashtable_ref_insert_benchmark_impl()
{
    static const size_t kRepeatTimes = (kIterations / kHeaderFieldSize);

    std::string index_buf[kHeaderFieldSize];
    jstd::string_view field_str[kHeaderFieldSize];
    jstd::string_view index_str[kHeaderFieldSize];
    for (size_t i = 0; i < kHeaderFieldSize; ++i) {
        field_str[i].attach(header_fields[i]);
        index_buf[i] = std::to_string(i);
        index_str[i] = index_buf[i];
    }

    {
        size_t checksum = 0;
        double totalTime = 0.0;
        jtest::StopWatch sw;

        for (size_t i = 0; i < kRepeatTimes; ++i) {
            AlgorithmTy algorithm(kInitCapacity);
            sw.start();
            for (size_t j = 0; j < kHeaderFieldSize; ++j) {
                algorithm.insert(field_str[j], index_str[j]);
            }
            checksum += algorithm.size();
            sw.stop();

            totalTime += sw.getElapsedMillisec();
        }

        printf("---------------------------------------------------------------------------\n");
        printf(" %-36s  ", AlgorithmTy::name());
        printf("sum = %-10" PRIuPTR "  time: %8.3f ms\n", checksum, totalTime);
    }
}

void hashtable_ref_insert_benchmark()
{
    std::cout << "  hashtable_ref_insert_benchmark()" << std::endl;
    std::cout << std::endl;

    //hashtable_ref_emplace_benchmark_impl<test::std_map<jstd::string_view, jstd::string_view>>();
    hashtable_ref_insert_benchmark_impl<test::std_unordered_map<jstd::string_view, jstd::string_view>>();

#if USE_JSTD_HASH_TABLE
    hashtable_ref_insert_benchmark_impl<test::hash_table_impl<jstd::hash_table<jstd::string_view, jstd::string_view>>>();
    hashtable_ref_insert_benchmark_impl<test::hash_table_impl<jstd::hash_table_time31<jstd::string_view, jstd::string_view>>>();
    hashtable_ref_insert_benchmark_impl<test::hash_table_impl<jstd::hash_table_time31_std<jstd::string_view, jstd::string_view>>>();
#endif

#if USE_JSTD_DICTIONARY
    hashtable_ref_insert_benchmark_impl<test::hash_table_impl<jstd::Dictionary<jstd::string_view, jstd::string_view>>>();
    hashtable_ref_insert_benchmark_impl<test::hash_table_impl<jstd::Dictionary_Time31<jstd::string_view, jstd::string_view>>>();
    hashtable_ref_insert_benchmark_impl<test::hash_table_impl<jstd::Dictionary_Time31Std<jstd::string_view, jstd::string_view>>>();
#else
    hashtable_ref_insert_benchmark_impl<test::hash_table_impl<jstd::Dictionary<jstd::string_view, jstd::string_view>>>();
#endif

    printf("---------------------------------------------------------------------------\n");
    printf("\n");
}

template <typename AlgorithmTy>
void hashtable_ref_emplace_benchmark_impl()
{
    static const size_t kRepeatTimes = (kIterations / kHeaderFieldSize);

    std::string index_buf[kHeaderFieldSize];
    jstd::string_view field_str[kHeaderFieldSize];
    jstd::string_view index_str[kHeaderFieldSize];
    for (size_t i = 0; i < kHeaderFieldSize; ++i) {
        field_str[i].attach(header_fields[i]);
        index_buf[i] = std::to_string(i);
        index_str[i] = index_buf[i];
    }

    {
        size_t checksum = 0;
        double totalTime = 0.0;
        jtest::StopWatch sw;

        for (size_t i = 0; i < kRepeatTimes; ++i) {
            AlgorithmTy algorithm(kInitCapacity);
            sw.start();
            for (size_t j = 0; j < kHeaderFieldSize; ++j) {
                algorithm.emplace(field_str[j], index_str[j]);
            }
            checksum += algorithm.size();
            sw.stop();

            totalTime += sw.getElapsedMillisec();
        }

        printf("---------------------------------------------------------------------------\n");
        printf(" %-36s  ", AlgorithmTy::name());
        printf("sum = %-10" PRIuPTR "  time: %8.3f ms\n", checksum, totalTime);
    }
}

void hashtable_ref_emplace_benchmark()
{
    std::cout << "  hashtable_ref_emplace_benchmark()" << std::endl;
    std::cout << std::endl;

    //hashtable_ref_emplace_benchmark_impl<test::std_map<jstd::string_view, jstd::string_view>>();
    hashtable_ref_emplace_benchmark_impl<test::std_unordered_map<jstd::string_view, jstd::string_view>>();

#if USE_JSTD_HASH_TABLE
    hashtable_ref_emplace_benchmark_impl<test::hash_table_impl<jstd::hash_table<jstd::string_view, jstd::string_view>>>();
    hashtable_ref_emplace_benchmark_impl<test::hash_table_impl<jstd::hash_table_time31<jstd::string_view, jstd::string_view>>>();
    hashtable_ref_emplace_benchmark_impl<test::hash_table_impl<jstd::hash_table_time31_std<jstd::string_view, jstd::string_view>>>();
#endif

#if USE_JSTD_DICTIONARY
    hashtable_ref_emplace_benchmark_impl<test::hash_table_impl<jstd::Dictionary<jstd::string_view, jstd::string_view>>>();
    hashtable_ref_emplace_benchmark_impl<test::hash_table_impl<jstd::Dictionary_Time31<jstd::string_view, jstd::string_view>>>();
    hashtable_ref_emplace_benchmark_impl<test::hash_table_impl<jstd::Dictionary_Time31Std<jstd::string_view, jstd::string_view>>>();
#else
    hashtable_ref_emplace_benchmark_impl<test::hash_table_impl<jstd::Dictionary<jstd::string_view, jstd::string_view>>>();
#endif

    printf("---------------------------------------------------------------------------\n");
    printf("\n");
}

template <typename AlgorithmTy>
void hashtable_ref_erase_benchmark_impl()
{
    static const size_t kRepeatTimes = (kIterations / kHeaderFieldSize);

    std::string field_str[kHeaderFieldSize];
    std::string index_str[kHeaderFieldSize];
    for (size_t i = 0; i < kHeaderFieldSize; ++i) {
        field_str[i].assign(header_fields[i]);
        index_str[i] = std::to_string(i);
    }

    {
        size_t checksum = 0;
        double totalTime = 0.0;
        jtest::StopWatch sw;

        for (size_t i = 0; i < kRepeatTimes; ++i) {
            AlgorithmTy algorithm;

            for (size_t j = 0; j < kHeaderFieldSize; ++j) {
                algorithm.emplace(field_str[j], index_str[j]);
            }
            checksum += algorithm.size();

            sw.start();
            for (size_t j = 0; j < kHeaderFieldSize; ++j) {
                size_t counts = algorithm.erase(field_str[j]);
            }
            sw.stop();

            assert(algorithm.size() == 0);
            checksum += algorithm.size();

            totalTime += sw.getElapsedMillisec();
        }

        printf("---------------------------------------------------------------------------\n");
        printf(" %-36s  ", AlgorithmTy::name());
        printf("sum = %-10" PRIuPTR "  time: %8.3f ms\n", checksum, totalTime);
    }
}

void hashtable_ref_erase_benchmark()
{
    std::cout << "  hashtable_ref_erase_benchmark()" << std::endl;
    std::cout << std::endl;

    //hashtable_ref_erase_benchmark_impl<test::std_map<jstd::string_view, jstd::string_view>>();
    hashtable_ref_erase_benchmark_impl<test::std_unordered_map<jstd::string_view, jstd::string_view>>();

#if USE_JSTD_HASH_TABLE
    hashtable_ref_erase_benchmark_impl<test::hash_table_impl<jstd::hash_table<jstd::string_view, jstd::string_view>>>();
    hashtable_ref_erase_benchmark_impl<test::hash_table_impl<jstd::hash_table_time31<jstd::string_view, jstd::string_view>>>();
    hashtable_ref_erase_benchmark_impl<test::hash_table_impl<jstd::hash_table_time31_std<jstd::string_view, jstd::string_view>>>();
#endif

#if USE_JSTD_DICTIONARY
    hashtable_ref_erase_benchmark_impl<test::hash_table_impl<jstd::Dictionary<jstd::string_view, jstd::string_view>>>();
    hashtable_ref_erase_benchmark_impl<test::hash_table_impl<jstd::Dictionary_Time31<jstd::string_view, jstd::string_view>>>();
    hashtable_ref_erase_benchmark_impl<test::hash_table_impl<jstd::Dictionary_Time31Std<jstd::string_view, jstd::string_view>>>();
#else
    hashtable_ref_erase_benchmark_impl<test::hash_table_impl<jstd::Dictionary<jstd::string_view, jstd::string_view>>>();
#endif

    printf("---------------------------------------------------------------------------\n");
    printf("\n");
}

template <typename AlgorithmTy>
void hashtable_ref_insert_erase_benchmark_impl()
{
    static const size_t kRepeatTimes = (kIterations / kHeaderFieldSize);

    std::string index_buf[kHeaderFieldSize];
    jstd::string_view field_str[kHeaderFieldSize];
    jstd::string_view index_str[kHeaderFieldSize];
    for (size_t i = 0; i < kHeaderFieldSize; ++i) {
        field_str[i].attach(header_fields[i]);
        index_buf[i] = std::to_string(i);
        index_str[i] = index_buf[i];
    }

    {
        size_t checksum = 0;
        AlgorithmTy algorithm;

        jtest::StopWatch sw;

        sw.start();
        for (size_t i = 0; i < kRepeatTimes; ++i) {
#if 0
            assert(algorithm.size() == 0);
            algorithm.clear();
            assert(algorithm.size() == 0);
            checksum += algorithm.size();
#endif
            for (size_t j = 0; j < kHeaderFieldSize; ++j) {
                algorithm.emplace(field_str[j], index_str[j]);
            }
            checksum += algorithm.size();

            for (size_t j = 0; j < kHeaderFieldSize; ++j) {
                algorithm.erase(field_str[j]);
            }
            assert(algorithm.size() == 0);
            checksum += algorithm.size();
        }
        sw.stop();

        printf("---------------------------------------------------------------------------\n");
        printf(" %-36s  ", AlgorithmTy::name());
        printf("sum = %-10" PRIuPTR "  time: %8.3f ms\n", checksum, sw.getElapsedMillisec());
    }
}

void hashtable_ref_insert_erase_benchmark()
{
    std::cout << "  hashtable_ref_insert_erase_benchmark()" << std::endl;
    std::cout << std::endl;

    //hashtable_ref_insert_erase_benchmark_impl<test::std_map<jstd::string_view, jstd::string_view>>();
    hashtable_ref_insert_erase_benchmark_impl<test::std_unordered_map<jstd::string_view, jstd::string_view>>();

#if USE_JSTD_HASH_TABLE
    hashtable_ref_insert_erase_benchmark_impl<test::hash_table_impl<jstd::hash_table<jstd::string_view, jstd::string_view>>>();
    hashtable_ref_insert_erase_benchmark_impl<test::hash_table_impl<jstd::hash_table_time31<jstd::string_view, jstd::string_view>>>();
    hashtable_ref_insert_erase_benchmark_impl<test::hash_table_impl<jstd::hash_table_time31_std<jstd::string_view, jstd::string_view>>>();
#endif

#if USE_JSTD_DICTIONARY
    hashtable_ref_insert_erase_benchmark_impl<test::hash_table_impl<jstd::Dictionary<jstd::string_view, jstd::string_view>>>();
    hashtable_ref_insert_erase_benchmark_impl<test::hash_table_impl<jstd::Dictionary_Time31<jstd::string_view, jstd::string_view>>>();
    hashtable_ref_insert_erase_benchmark_impl<test::hash_table_impl<jstd::Dictionary_Time31Std<jstd::string_view, jstd::string_view>>>();
#else
    hashtable_ref_insert_erase_benchmark_impl<test::hash_table_impl<jstd::Dictionary<jstd::string_view, jstd::string_view>>>();
#endif

    printf("---------------------------------------------------------------------------\n");
    printf("\n");
}

template <typename AlgorithmTy>
void hashtable_rehash_benchmark_impl()
{
    static const size_t kRepeatTimes = (kIterations / kHeaderFieldSize);

    std::string field_str[kHeaderFieldSize];
    std::string index_str[kHeaderFieldSize];
    for (size_t i = 0; i < kHeaderFieldSize; ++i) {
        field_str[i].assign(header_fields[i]);
        index_str[i] = std::to_string(i);
    }

    {
        size_t checksum = 0;
        size_t buckets = 128;

        AlgorithmTy algorithm(kInitCapacity);

        for (size_t i = 0; i < kHeaderFieldSize; ++i) {
            algorithm.emplace(field_str[i], index_str[i]);
        }

        jtest::StopWatch sw;

        sw.start();
        for (size_t i = 0; i < kRepeatTimes; ++i) {
            checksum += algorithm.size();

            buckets = 128;
            algorithm.rehash(buckets);
            checksum += algorithm.bucket_count();
#ifndef NDEBUG
            static size_t rehash_cnt1 = 0;
            if (rehash_cnt1 < 20) {
                if (algorithm.bucket_count() != buckets) {
                    size_t bucket_count = algorithm.bucket_count();
                    printf("rehash1():   size = %" PRIuPTR ", buckets = %" PRIuPTR ", bucket_count = %" PRIuPTR "\n",
                           algorithm.size(), buckets, bucket_count);
                }
                rehash_cnt1++;
            }
#endif
            for (size_t j = 0; j < 7; ++j) {
                buckets *= 2;
                algorithm.rehash(buckets);
                checksum += algorithm.bucket_count();
#ifndef NDEBUG
                static size_t rehash_cnt2 = 0;
                if (rehash_cnt2 < 20) {
                    if (algorithm.bucket_count() != buckets) {
                        size_t bucket_count = algorithm.bucket_count();
                        printf("rehash2(%u):   size = %" PRIuPTR ", buckets = %" PRIuPTR ", bucket_count = %" PRIuPTR "\n",
                               (uint32_t)j, algorithm.size(), buckets, bucket_count);
                    }
                    rehash_cnt2++;
                }
#endif
            }
        }
        sw.stop();

        printf("---------------------------------------------------------------------------\n");
        printf(" %-36s  ", AlgorithmTy::name());
        printf("sum = %-10" PRIuPTR "  time: %8.3f ms\n", checksum, sw.getElapsedMillisec());
    }
}

void hashtable_rehash_benchmark()
{
    std::cout << "  hashtable_rehash_benchmark()" << std::endl;
    std::cout << std::endl;

    hashtable_rehash_benchmark_impl<test::std_unordered_map<std::string, std::string>>();

#if USE_JSTD_HASH_TABLE
    hashtable_rehash_benchmark_impl<test::hash_table_impl<jstd::hash_table<std::string, std::string>>>();
    hashtable_rehash_benchmark_impl<test::hash_table_impl<jstd::hash_table_time31<std::string, std::string>>>();
    hashtable_rehash_benchmark_impl<test::hash_table_impl<jstd::hash_table_time31_std<std::string, std::string>>>();
#endif

#if USE_JSTD_DICTIONARY
    hashtable_rehash_benchmark_impl<test::hash_table_impl<jstd::Dictionary<std::string, std::string>>>();
    hashtable_rehash_benchmark_impl<test::hash_table_impl<jstd::Dictionary_Time31<std::string, std::string>>>();
    hashtable_rehash_benchmark_impl<test::hash_table_impl<jstd::Dictionary_Time31Std<std::string, std::string>>>();
#else
    hashtable_rehash_benchmark_impl<test::hash_table_impl<jstd::Dictionary<std::string, std::string>>>();
#endif

    printf("---------------------------------------------------------------------------\n");
    printf("\n");
}

template <typename AlgorithmTy>
void hashtable_rehash2_benchmark_impl()
{
    static const size_t kRepeatTimes = (kIterations / kHeaderFieldSize / 2);

    std::string field_str[kHeaderFieldSize];
    std::string index_str[kHeaderFieldSize];
    for (size_t i = 0; i < kHeaderFieldSize; ++i) {
        field_str[i].assign(header_fields[i]);
        index_str[i] = std::to_string(i);
    }

    {
        size_t checksum = 0;
        size_t buckets;

        jtest::StopWatch sw;

        sw.start();
        for (size_t i = 0; i < kRepeatTimes; ++i) {
            AlgorithmTy algorithm(kInitCapacity);
            for (size_t j = 0; j < kHeaderFieldSize; ++j) {
                algorithm.emplace(field_str[j], index_str[j]);
            }

            checksum += algorithm.size();
            checksum += algorithm.bucket_count();

            buckets = 128;
            algorithm.rehash(buckets);
#ifndef NDEBUG
            static size_t rehash_cnt1 = 0;
            if (rehash_cnt1 < 20) {
                if (algorithm.bucket_count() != buckets) {
                    size_t bucket_count = algorithm.bucket_count();
                    printf("rehash1():   size = %" PRIuPTR ", buckets = %" PRIuPTR ", bucket_count = %" PRIuPTR "\n",
                           algorithm.size(), buckets, bucket_count);
                }
                rehash_cnt1++;
            }
#endif
            checksum += algorithm.bucket_count();

            for (size_t j = 0; j < 7; ++j) {
                buckets *= 2;
                algorithm.rehash(buckets);
#ifndef NDEBUG
                static size_t rehash_cnt2 = 0;
                if (rehash_cnt2 < 20) {
                    if (algorithm.bucket_count() != buckets) {
                        size_t bucket_count = algorithm.bucket_count();
                        printf("rehash2(%u):   size = %" PRIuPTR ", buckets = %" PRIuPTR ", bucket_count = %" PRIuPTR "\n",
                               (uint32_t)j, algorithm.size(), buckets, bucket_count);
                    }
                    rehash_cnt2++;
                }
#endif
                checksum += algorithm.bucket_count();
            }
        }
        sw.stop();

        printf("---------------------------------------------------------------------------\n");
        printf(" %-36s  ", AlgorithmTy::name());
        printf("sum = %-10" PRIuPTR "  time: %8.3f ms\n", checksum, sw.getElapsedMillisec());
    }
}

void hashtable_rehash2_benchmark()
{
    std::cout << "  hashtable_rehash2_benchmark()" << std::endl;
    std::cout << std::endl;

    hashtable_rehash2_benchmark_impl<test::std_unordered_map<std::string, std::string>>();

#if USE_JSTD_HASH_TABLE
    hashtable_rehash2_benchmark_impl<test::hash_table_impl<jstd::hash_table<std::string, std::string>>>();
    hashtable_rehash2_benchmark_impl<test::hash_table_impl<jstd::hash_table_time31<std::string, std::string>>>();
    hashtable_rehash2_benchmark_impl<test::hash_table_impl<jstd::hash_table_time31_std<std::string, std::string>>>();
#endif

#if USE_JSTD_DICTIONARY
    hashtable_rehash2_benchmark_impl<test::hash_table_impl<jstd::Dictionary<std::string, std::string>>>();
    hashtable_rehash2_benchmark_impl<test::hash_table_impl<jstd::Dictionary_Time31<std::string, std::string>>>();
    hashtable_rehash2_benchmark_impl<test::hash_table_impl<jstd::Dictionary_Time31Std<std::string, std::string>>>();
#else
    hashtable_rehash2_benchmark_impl<test::hash_table_impl<jstd::Dictionary<std::string, std::string>>>();
#endif

    printf("---------------------------------------------------------------------------\n");
    printf("\n");
}

void hashtable_benchmark()
{
    hashtable_find_benchmark();
    hashtable_insert_benchmark();
    hashtable_emplace_benchmark();
    hashtable_erase_benchmark();
    hashtable_insert_erase_benchmark();

    hashtable_ref_find_benchmark();
    hashtable_ref_insert_benchmark();
    hashtable_ref_emplace_benchmark();
    hashtable_ref_erase_benchmark();
    hashtable_ref_insert_erase_benchmark();

    hashtable_rehash_benchmark();
    hashtable_rehash2_benchmark();
}

template <typename Container>
void hashtable_iterator_uinttest()
{
    std::string field_str[kHeaderFieldSize];
    std::string index_str[kHeaderFieldSize];
    for (size_t i = 0; i < kHeaderFieldSize; ++i) {
        field_str[i].assign(header_fields[i]);
        index_str[i] = std::to_string(i);
    }

    Container container(kInitCapacity);
    Container container2(kInitCapacity);
    for (size_t j = 0; j < kHeaderFieldSize; ++j) {
        container2.emplace(field_str[j], index_str[j]);
    }

    container.swap(container2);

    container.display_status();

    typedef typename Container::iterator                iterator;
    typedef typename Container::const_iterator          const_iterator;
    typedef typename Container::local_iterator          local_iterator;
    typedef typename Container::const_local_iterator    const_local_iterator;

    typedef typename Container::hasher hasher;

    hasher hasher_;

    {
        //     " [  1]: 0x7289F843  3      From:                          13"
        printf("\n");
        printf("   #       hash     index  key                            value\n");
        printf("----------------------------------------------------------------------\n");

        uint32_t index = 0;
        for (iterator iter = container.begin(); iter != container.end(); ++iter) {
            std::uint32_t hash_code = container.get_hash(iter->first);
            std::string key_name = iter->first.c_str() + std::string(":");
            printf(" [%3d]: 0x%08X  %-5u  %-30s %s\n", index + 1,
                   hash_code,
                   uint32_t(hash_code & container.bucket_mask()),
                   key_name.c_str(),
                   iter->second.c_str());
            index++;
        }
        printf("\n\n");
    }

    {
        printf("\n");
        printf("   #       hash     index  key                            value\n");
        printf("----------------------------------------------------------------------\n");

        uint32_t index = 0;
        for (local_iterator iter = container.l_begin(); iter != container.l_end(); ++iter) {
            std::uint32_t hash_code = container.get_hash(iter->value.first);
            std::uint32_t hash_code2 = iter->hash_code;
            assert(hash_code == hash_code2);
            std::string key_name = iter->value.first.c_str() + std::string(":");
            printf(" [%3d]: 0x%08X  %-5u  %-30s %s\n", index + 1,
                   hash_code,
                   uint32_t(hash_code & container.bucket_mask()),
                   key_name.c_str(),
                   iter->value.second.c_str());
            index++;
        }
        printf("\n\n");
    }

    {
        printf("\n");
        printf("   #       hash     index  key                            value\n");
        printf("----------------------------------------------------------------------\n");

        uint32_t index = 0;
        for (const_iterator iter = container.cbegin(); iter != container.cend(); ++iter) {
            std::uint32_t hash_code = container.get_hash(iter->first);
            std::string key_name = iter->first.c_str() + std::string(":");
            printf(" [%3d]: 0x%08X  %-5u  %-30s %s\n", index + 1,
                   hash_code,
                   uint32_t(hash_code & container.bucket_mask()),
                   key_name.c_str(),
                   iter->second.c_str());
            index++;
        }
        printf("\n\n");
    }

    {
        printf("\n");
        printf("   #       hash     index  key                            value\n");
        printf("----------------------------------------------------------------------\n");

        uint32_t index = 0;
        for (const_local_iterator iter = container.l_cbegin(); iter != container.l_cend(); ++iter) {
            std::uint32_t hash_code = container.get_hash(iter->value.first);
            std::uint32_t hash_code2 = iter->hash_code;
            assert(hash_code == hash_code2);
            std::string key_name = iter->value.first.c_str() + std::string(":");
            printf(" [%3d]: 0x%08X  %-5u  %-30s %s\n", index + 1,
                   hash_code,
                   uint32_t(hash_code & container.bucket_mask()),
                   key_name.c_str(),
                   iter->value.second.c_str());
            index++;
        }
        printf("\n\n");
    }
}

template <typename Container>
void hashtable_show_status(const std::string & name)
{
    std::string field_str[kHeaderFieldSize];
    std::string index_str[kHeaderFieldSize];
    for (size_t i = 0; i < kHeaderFieldSize; ++i) {
        field_str[i].assign(header_fields[i]);
        index_str[i] = std::to_string(i);
    }

    Container container(kInitCapacity);
    for (size_t i = 0; i < kHeaderFieldSize; ++i) {
        container.emplace(field_str[i], index_str[i]);
    }

    jstd::HashMapAnalyzer<Container> analyzer(container);
    analyzer.set_name(name);
    analyzer.start_analyse();

    analyzer.display_status();
    analyzer.dump_entries();
}

template <typename Container>
void hashtable_i_show_status(const std::string & name)
{
    Container container(kInitCapacity);
    for (size_t i = 0; i < kHeaderFieldSize; ++i) {
        container.emplace(i, i + 1);
    }

    jstd::HashMapAnalyzer<Container> analyzer(container);
    analyzer.set_name(name);
    analyzer.start_analyse();

    analyzer.display_status();
    analyzer.dump_entries();
}

template <typename Container>
void hashtable_dict_words_show_status(const std::string & name)
{
    Container container(kInitCapacity);
    for (size_t i = 0; i < dict_words.size(); i++) {
        container.emplace(dict_words[i], std::to_string(i));
    }

    jstd::HashMapAnalyzer<Container> analyzer(container);
    analyzer.set_name(name);
    analyzer.start_analyse();

    analyzer.display_status();
    analyzer.dump_entries(100);
}

template <typename Container>
void hashtable_dict_words_i_show_status(const std::string & name)
{
    Container container(kInitCapacity);
    for (size_t i = 0; i < dict_words.size(); i++) {
        container.emplace(i, i + 1);
    }

    jstd::HashMapAnalyzer<Container> analyzer(container);
    analyzer.set_name(name);
    analyzer.start_analyse();

    analyzer.display_status();
    analyzer.dump_entries(100);
}

void hashtable_uinttest()
{
    if (dict_words_is_ready && dict_words.size() > 0) {
        hashtable_dict_words_show_status<jstd::Dictionary<std::string, std::string>>("Dictionary<std::string, std::string>");
        hashtable_dict_words_show_status<jstd::Dictionary<jstd::string_view, jstd::string_view>>("Dictionary<jstd::string_view, jstd::string_view>");

        hashtable_dict_words_show_status<jstd::Dictionary_Time31<std::string, std::string>>("Dictionary_Time31<std::string, std::string>");
        hashtable_dict_words_show_status<jstd::Dictionary_Time31<jstd::string_view, jstd::string_view>>("Dictionary_Time31<jstd::string_view, jstd::string_view>");

        //hashtable_dict_words_show_status<std::unordered_map<std::string, std::string>>("std::unordered_map<std::string, std::string>");
        //hashtable_dict_words_show_status<std::unordered_map<jstd::string_view, jstd::string_view>>("std::unordered_map<jstd::string_view, jstd::string_view>");

        hashtable_dict_words_i_show_status<jstd::Dictionary<std::size_t, std::size_t>>("Dictionary<std::size_t, std::size_t>");
    }
    else {
        hashtable_show_status<jstd::Dictionary<std::string, std::string>>("Dictionary<std::string, std::string>");
        hashtable_show_status<jstd::Dictionary<jstd::string_view, jstd::string_view>>("Dictionary<jstd::string_view, jstd::string_view>");

        hashtable_show_status<jstd::Dictionary_Time31<std::string, std::string>>("Dictionary_Time31<std::string, std::string>");
        hashtable_show_status<jstd::Dictionary_Time31<jstd::string_view, jstd::string_view>>("Dictionary_Time31<jstd::string_view, jstd::string_view>");

        //hashtable_show_status<std::unordered_map<std::string, std::string>>("std::unordered_map<std::string, std::string>");        
        //hashtable_show_status<std::unordered_map<jstd::string_view, jstd::string_view>>("std::unordered_map<jstd::string_view, jstd::string_view>");

        hashtable_i_show_status<jstd::Dictionary<std::size_t, std::size_t>>("Dictionary<std::size_t, std::size_t>");
    }
    
    //hashtable_iterator_uinttest<jstd::Dictionary<std::string, std::string>>();
}

void formatter_benchmark_sprintf_Integer_1()
{
#ifdef NDEBUG
    static const std::size_t iters = 999999;
#else
    static const std::size_t iters = 9999;
#endif

    std::size_t i;
    double time, time_base;
    jtest::StopWatch sw;

    char fmtbuf1[512] = { 0 };
    char fmtbuf2[512] = { 0 };
    std::string str1;
    std::size_t fmt_len;

    printf("==========================================================================\n\n");
    printf("  formatter_benchmark_sprintf_Integer_1()\n\n");

    printf("==========================================================================\n\n");
    printf("  for (i = 0; i < %" PRIuPTR "; ++i) {\n", iters);
    printf("      len = snprintf(buf, bufsize, count,\n"
           "                     \"%%d, %%d, %%d, %%d, %%d,\\n\"\n"
           "                     \"%%d, %%d, %%d, %%d, %%d.\",\n"
           "                      12,  1234,  123456,  12345678,  123456789,\n"
           "                     -12, -1234, -123456, -12345678, -123456789);\n");
    printf("  }\n\n");
    //printf("==========================================================================\n\n");

    {
        sw.start();
        for (i = 0; i < iters; ++i) {
            int fmt_size;
#ifdef _MSC_VER
            fmt_size = sprintf_s(fmtbuf1, sizeof(fmtbuf1),
#else
            fmt_size =   sprintf(fmtbuf1,
#endif
                                 "%d, %d, %d, %d, %d,\n"
                                 "%d, %d, %d, %d, %d.",
                                  12,  1234,  123456,  12345678,  123456789,
                                 -12, -1234, -123456, -12345678, -123456789);
            JSTD_UNUSED_VAR(fmt_size);
        }
        sw.stop();
        time = sw.getElapsedMillisec();
        time_base = time;

        printf("==========================================================================\n\n");
        printf(">>> %-20s <<<\n\n", "sprintf()");
        printf("result =\n%s\n\n", fmtbuf1);
        printf("strlen       = %" PRIuPTR " bytes\n", ::strlen(fmtbuf1));
        printf("elapsed time = %0.3f ms\n", time);
        printf("\n");
    }

    {
        sw.start();
        for (i = 0; i < iters; ++i) {
            int fmt_size;
#ifdef _MSC_VER
            fmt_size = _snprintf_s(fmtbuf2, sizeof(fmtbuf2),
#else
            fmt_size =    snprintf(fmtbuf2, sizeof(fmtbuf2),
#endif
                                 "%d, %d, %d, %d, %d,\n"
                                 "%d, %d, %d, %d, %d.",
                                  12,  1234,  123456,  12345678,  123456789,
                                 -12, -1234, -123456, -12345678, -123456789);
            JSTD_UNUSED_VAR(fmt_size);
        }
        sw.stop();
        time = sw.getElapsedMillisec();
        time_base = time;

        printf("==========================================================================\n\n");
        printf(">>> %-20s <<<\n\n", "snprintf()");
        printf("result =\n%s\n\n", fmtbuf2);
        printf("strlen       = %" PRIuPTR " bytes\n", ::strlen(fmtbuf2));
        printf("elapsed time = %0.3f ms\n", time);
        printf("\n");
    }

    jstd::formatter fmt;

    {
        sw.start();
        for (i = 0; i < iters; ++i) {
            const char * fmt_buf = nullptr;
            fmt_len = fmt.output(fmt_buf,
                                 "%d, %d, %d, %d, %d,\n"
                                 "%d, %d, %d, %d, %d.",
                                  12,  1234,  123456,  12345678,  123456789,
                                 -12, -1234, -123456, -12345678, -123456789);
            if (fmt_buf != nullptr) {
                ::free((void *)fmt_buf);
            }
        }
        sw.stop();
        time = sw.getElapsedMillisec();

        const char * fmt_buf = nullptr;
        fmt_len = fmt.output(fmt_buf,
                             "%d, %d, %d, %d, %d,\n"
                             "%d, %d, %d, %d, %d.",
                              12,  1234,  123456,  12345678,  123456789,
                             -12, -1234, -123456, -12345678, -123456789);

        printf("==========================================================================\n\n");
        printf(">>> %-20s <<<\n\n", "fmt.output()");
        printf("result =\n%s\n\n", fmt_buf);
        printf("strlen       = %" PRIuPTR " bytes\n", ::strlen(fmt_buf));
        printf("elapsed time = %0.3f ms\n", time);
        printf("\n");

        if (fmt_buf != nullptr) {
            ::free((void *)fmt_buf);
        }
    }

    {  
        sw.restart();
        for (i = 0; i < iters; ++i) {
            std::string str;
            fmt_len = fmt.sprintf(str,
                                 "%d, %d, %d, %d, %d,\n"
                                 "%d, %d, %d, %d, %d.",
                                  12,  1234,  123456,  12345678,  123456789,
                                 -12, -1234, -123456, -12345678, -123456789);
        }
        sw.stop();
        time = sw.getElapsedMillisec();

        str1.clear();
        fmt_len = fmt.sprintf(str1,
                              "%d, %d, %d, %d, %d,\n"
                              "%d, %d, %d, %d, %d.",
                               12,  1234,  123456,  12345678,  123456789,
                              -12, -1234, -123456, -12345678, -123456789);

        printf("==========================================================================\n\n");
        printf(">>> %-20s <<<\n\n", "fmt.sprintf() *");
        printf("result = \n%s\n\n", str1.c_str());
        printf("strlen       = %" PRIuPTR " bytes\n", str1.size());

        printf("elapsed time = %0.3f ms\n\n", time);
        printf("fmt.sprintf() * vs snprintf(): %0.3f x times.\n", time_base / time);
        printf("\n");
    }

    {
        std::string str;
  
        sw.restart();
        for (i = 0; i < iters; ++i) {
            str.clear();
            fmt_len = fmt.sprintf(str,
                                 "%d, %d, %d, %d, %d,\n"
                                 "%d, %d, %d, %d, %d.",
                                  12,  1234,  123456,  12345678,  123456789,
                                 -12, -1234, -123456, -12345678, -123456789);
        }
        sw.stop();
        time = sw.getElapsedMillisec();

        printf("==========================================================================\n\n");
        printf(">>> %-20s <<<\n\n", "fmt.sprintf()");
        printf("result = \n%s\n\n", str.c_str());
        printf("strlen       = %" PRIuPTR " bytes\n", str.size());

        printf("elapsed time = %0.3f ms\n\n", time);
        printf("fmt.sprintf() vs snprintf(): %0.3f x times.\n", time_base / time);
        printf("\n");
    }

    {  
        sw.restart();
        for (i = 0; i < iters; ++i) {
            std::string str;
            fmt_len = fmt.sprintf_direct(str,
                                 "%d, %d, %d, %d, %d,\n"
                                 "%d, %d, %d, %d, %d.",
                                  12,  1234,  123456,  12345678,  123456789,
                                 -12, -1234, -123456, -12345678, -123456789);
        }
        sw.stop();
        time = sw.getElapsedMillisec();

        str1.clear();
        fmt_len = fmt.sprintf_direct(str1,
                              "%d, %d, %d, %d, %d,\n"
                              "%d, %d, %d, %d, %d.",
                               12,  1234,  123456,  12345678,  123456789,
                              -12, -1234, -123456, -12345678, -123456789);

        printf("==========================================================================\n\n");
        printf(">>> %-20s <<<\n\n", "fmt.sprintf_direct() *");
        printf("result = \n%s\n\n", str1.c_str());
        printf("strlen       = %" PRIuPTR " bytes\n", str1.size());

        printf("elapsed time = %0.3f ms\n\n", time);
        printf("fmt.sprintf_direct() * vs snprintf(): %0.3f x times.\n", time_base / time);
        printf("\n");
    }

    {
        std::string str;
  
        sw.restart();
        for (i = 0; i < iters; ++i) {
            str.clear();
            fmt_len = fmt.sprintf_direct(str,
                                 "%d, %d, %d, %d, %d,\n"
                                 "%d, %d, %d, %d, %d.",
                                  12,  1234,  123456,  12345678,  123456789,
                                 -12, -1234, -123456, -12345678, -123456789);
        }
        sw.stop();
        time = sw.getElapsedMillisec();

        printf("==========================================================================\n\n");
        printf(">>> %-20s <<<\n\n", "fmt.sprintf_direct()");
        printf("result = \n%s\n\n", str.c_str());
        printf("strlen       = %" PRIuPTR " bytes\n", str.size());

        printf("elapsed time = %0.3f ms\n\n", time);
        printf("fmt.sprintf_direct() vs snprintf(): %0.3f x times.\n", time_base / time);
        printf("\n");
    }

    {
        sw.restart();
        for (i = 0; i < iters; ++i) {
            std::string str;
            fmt_len = fmt.sprintf_no_prepare(str,
                                 "%d, %d, %d, %d, %d,\n"
                                 "%d, %d, %d, %d, %d.",
                                  12,  1234,  123456,  12345678,  123456789,
                                 -12, -1234, -123456, -12345678, -123456789);
        }
        sw.stop();
        time = sw.getElapsedMillisec();

        str1.clear();
        fmt_len = fmt.sprintf_no_prepare(str1,
                                "%d, %d, %d, %d, %d,\n"
                                "%d, %d, %d, %d, %d.",
                                12,  1234,  123456,  12345678,  123456789,
                                -12, -1234, -123456, -12345678, -123456789);

        printf("==========================================================================\n\n");
        printf(">>> %-20s <<<\n\n", "fmt.sprintf_no_prepare() *");
        printf("result = \n%s\n\n", str1.c_str());
        printf("strlen       = %" PRIuPTR " bytes\n", str1.size());

        printf("elapsed time = %0.3f ms\n\n", time);
        printf("fmt.sprintf_no_prepare() * vs snprintf(): %0.3f x times.\n", time_base / time);
        printf("\n");
    }

    {
        std::string str;
  
        sw.restart();
        for (i = 0; i < iters; ++i) {
            str.clear();
            fmt_len = fmt.sprintf_no_prepare(str,
                                 "%d, %d, %d, %d, %d,\n"
                                 "%d, %d, %d, %d, %d.",
                                  12,  1234,  123456,  12345678,  123456789,
                                 -12, -1234, -123456, -12345678, -123456789);
        }
        sw.stop();
        time = sw.getElapsedMillisec();

        printf("==========================================================================\n\n");
        printf(">>> %-20s <<<\n\n", "fmt.sprintf_no_prepare()");
        printf("result = \n%s\n\n", str.c_str());
        printf("strlen       = %" PRIuPTR " bytes\n", str.size());

        printf("elapsed time = %0.3f ms\n\n", time);
        printf("fmt.sprintf_no_prepare() vs snprintf(): %0.3f x times.\n", time_base / time);
        printf("\n");
    }

    {
        sw.restart();
        for (i = 0; i < iters; ++i) {
            std::string str;
            fmt_len = format::snprintf(str,
                                 "%d, %d, %d, %d, %d,\n"
                                 "%d, %d, %d, %d, %d.",
                                  12,  1234,  123456,  12345678,  123456789,
                                 -12, -1234, -123456, -12345678, -123456789);
        }
        sw.stop();
        time = sw.getElapsedMillisec();

        fmt_len = format::snprintf(str1,
                                "%d, %d, %d, %d, %d,\n"
                                "%d, %d, %d, %d, %d.",
                                12,  1234,  123456,  12345678,  123456789,
                                -12, -1234, -123456, -12345678, -123456789);

        printf("==========================================================================\n\n");
        printf(">>> %-20s <<<\n\n", "format::snprintf() *");
        printf("result = \n%s\n\n", str1.c_str());
        printf("strlen       = %" PRIuPTR " bytes\n", str1.size());

        printf("elapsed time = %0.3f ms\n\n", time);
        printf("format::snprintf() * vs snprintf(): %0.3f x times.\n", time_base / time);
        printf("\n");
    }

    {
        std::string str;
  
        sw.restart();
        for (i = 0; i < iters; ++i) {
            str.clear();
            fmt_len = format::snprintf(str,
                                 "%d, %d, %d, %d, %d,\n"
                                 "%d, %d, %d, %d, %d.",
                                  12,  1234,  123456,  12345678,  123456789,
                                 -12, -1234, -123456, -12345678, -123456789);
        }
        sw.stop();
        time = sw.getElapsedMillisec();

        printf("==========================================================================\n\n");
        printf(">>> %-20s <<<\n\n", "format::snprintf()");
        printf("result = \n%s\n\n", str.c_str());
        printf("strlen       = %" PRIuPTR " bytes\n", str.size());

        printf("elapsed time = %0.3f ms\n\n", time);
        printf("format::snprintf() vs snprintf(): %0.3f x times.\n", time_base / time);
        printf("\n");
    }

    JSTD_UNUSED_VAR(fmt_len);
    printf("==========================================================================\n");
    printf("\n");
}

void formatter_benchmark()
{
    formatter_benchmark_sprintf_Integer_1();
}

void string_view_test()
{
    jstd::string_view sv1(header_fields[4]);
    try {
        //sv1[2] = 'a';  // Run-time error, readonly
    } catch (...) {
        std::cout << "jstd::string_view<CharT> operator [] read only" << std::endl;
    }

    std::string str1("12345676890");

    printf("str1 = %s\n", str1.c_str());
    std::cout << "sv1  = " << sv1 << "\n\n";

    jstd::string_view sv2("abcdefg");
    std::size_t n2 = sv2.copy((char *)str1.data(), 4, 0);

    printf("str1 = %s\n", str1.c_str());
    std::cout << "sv2  = " << sv2 << "\n\n";

    jstd::string_view sv3(str1);
    sv3[2] = 'q';

    printf("str1 = %s\n", str1.c_str());
    std::cout << "sv3  = " << sv3 << "\n\n";

    using std::swap;
    std::swap(sv2, sv3);
    std::cout << "sv2  = " << sv2 << '\n';
    std::cout << "sv3  = " << sv3 << "\n\n";

    jstd::Console::ReadKey();
}

void shiftable_ptr_test()
{
    {
        {
            jstd::shiftable_ptr<int> i = new int {100};
            jstd::shiftable_ptr<int> j = i;
            printf("i.value() = 0x%p, j.value() = 0x%p, j = %d\n\n",
                   i.get(), j.get(), *j);
        }

        {
            jstd::shiftable_ptr<int> m = new int {99};
            jstd::shiftable_ptr<int> n = m;
            m = new int {0};
            printf("m.value() = 0x%p, n.value() = 0x%p, m = %d, n = %d\n\n",
                   m.get(), n.get(), *m, *n);
        }

        {
            jstd::custom_shiftable_ptr<int> i {100};
            jstd::custom_shiftable_ptr<int> j = i;
            printf("custom_shiftable_ptr<T>: i.value() = 0x%p, j.value() = 0x%p, j = %d\n\n",
                   i.get(), j.get(), *j);
        }

        {
            jstd::custom_shiftable_ptr<int> m {99};
            jstd::custom_shiftable_ptr<int> n = m;
            m.reset(jstd::custom_shiftable_ptr<int>{88});
            printf("custom_shiftable_ptr<T>: m.value() = 0x%p, n.value() = 0x%p, m = %d, n = %d\n\n",
                   m.get(), n.get(), *m, *n);
        }
    }

    {
        {
            jstd::shiftable_ptr<jstd::string_view> i = new jstd::string_view("ijk");
            jstd::shiftable_ptr<jstd::string_view> j = i;
            printf("i.value() = 0x%p, j.value() = 0x%p, j = %s\n\n",
                   i.get(), j.get(), j->c_str());
        }

        {
            jstd::shiftable_ptr<jstd::string_view> m = new jstd::string_view("mn");
            jstd::shiftable_ptr<jstd::string_view> n = m;
            m = new jstd::string_view("m123");
            printf("m.value() = 0x%p, n.value() = 0x%p, m = %s, n = %s\n\n",
                   m.get(), n.get(), m->c_str(), n->c_str());
        }

        {
            jstd::custom_shiftable_ptr<jstd::string_view> i("ijk");
            jstd::custom_shiftable_ptr<jstd::string_view> j = i;
            printf("custom_shiftable_ptr<T>: i.value() = 0x%p, j.value() = 0x%p, j = %s\n\n",
                   i.get(), j.get(), j->c_str());
        }

        {
            jstd::custom_shiftable_ptr<jstd::string_view> m("mn");
            jstd::custom_shiftable_ptr<jstd::string_view> n = m;
            m.reset(jstd::custom_shiftable_ptr<jstd::string_view>("mmmmmm"));
            printf("custom_shiftable_ptr<T>: m.value() = 0x%p, n.value() = 0x%p, m = %s, n = %s\n\n",
                   m.get(), n.get(), m->c_str(), n->c_str());
        }
    }
}

void formatter_test()
{
    std::string str1, str2, str3;
    int v1 = 100, v2 = 220;
    int v3 = 500, v4 = 1024;
    unsigned int v5 = 2048, v6 = 4096;
    unsigned int v7 = 16384, v8 = 65536;
    jstd::formatter fmt;
    fmt.sprintf(str1, "num1 = %d, num2 = %d\n\n", v1, v2);
    fmt.sprintf_no_prepare(str2, "num1 = %d, num2 = %d\n\n", v1, v2);
    fmt.sprintf_direct(str3, "num1 = %d, num2 = %d\n\n", v1, v2);

    printf("\n");
    printf("fmt.sprintf(str1) = \"%s\"\n", str1.c_str());
    printf("fmt.sprintf_no_prepare(str2) = \"%s\"\n", str2.c_str());
    printf("fmt.sprintf_direct(str3) = \"%s\"\n", str3.c_str());

    str1.clear();
    fmt.sprintf(str1,
                "num1 = %d, num2 = %d\n"
                "num3 = %u, num4 = %d\n"
                "num4 = %u, num5 = %d\n"
                "num7 = %u, num8 = %d\n\n",
                v1, v2, v3, v4, v5, v6, v7, v8);

    str2.clear();
    fmt.sprintf_no_prepare(str2,
                "num1 = %d, num2 = %d\n"
                "num3 = %u, num4 = %d\n"
                "num4 = %u, num5 = %d\n"
                "num7 = %u, num8 = %d\n\n",
                v1, v2, v3, v4, v5, v6, v7, v8);

    str3.clear();
    fmt.sprintf_direct(str3,
                "num1 = %d, num2 = %d\n"
                "num3 = %u, num4 = %d\n"
                "num4 = %u, num5 = %d\n"
                "num7 = %u, num8 = %d\n\n",
                v1, v2, v3, v4, v5, v6, v7, v8);

    printf("\n");
    printf("fmt.sprintf(str1) = \"%s\"\n", str1.c_str());
    printf("fmt.sprintf_no_prepare(str2) = \"%s\"\n", str2.c_str());
    printf("fmt.sprintf_direct(str3) = \"%s\"\n", str3.c_str());
    printf("\n");
}

void fnv1a_hash_test()
{
    const char * test_str = "abcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyz"
        "abcdefghijklmnopqrstuvwxyz";
    uint32_t fnv1a_1 = jstd::hashes::FNV1A_Yoshimura(test_str, jstd::libc::StrLen(test_str));
    uint32_t fnv1a_2 = jstd::hashes::FNV1A_Yoshimitsu_TRIADii_xmm(test_str, jstd::libc::StrLen(test_str));
    uint32_t fnv1a_3 = jstd::hashes::FNV1A_penumbra(test_str, jstd::libc::StrLen(test_str));
}

#define PTR2HEX16(ptr) (uint32_t)((uint64_t)(ptr) >> 32), (uint32_t)((uint64_t)(ptr) & 0xFFFFFFFFULL)

void realloc_test()
{
    void * mem_ptr, * new_ptr;
    size_t old_size = 4, new_size;
    mem_ptr = ::malloc(old_size);
    if (mem_ptr == nullptr)
        return;
    printf("realloc_test()\n");
    printf("\n");
    for (size_t i = 0; i < 25; i++) {
        new_size = old_size * 2;
        new_ptr = ::realloc(mem_ptr, new_size);
        if (new_ptr != nullptr) {
            if (mem_ptr != new_ptr) {
                printf("    old-ptr: 0x%08X%08X, new-ptr: 0x%08X%08X, size: [%8u -> %-8u] bytes\n",
                        PTR2HEX16(mem_ptr), PTR2HEX16(new_ptr), (uint32_t)old_size, (uint32_t)new_size);
            }
            else {
                printf("(*) old-ptr: 0x%08X%08X, new-ptr: 0x%08X%08X, size: [%8u -> %-8u] bytes\n",
                        PTR2HEX16(mem_ptr), PTR2HEX16(new_ptr), (uint32_t)old_size, (uint32_t)new_size);
            }
            mem_ptr = new_ptr;
        }
        else {
            printf("realloc() error.\n\n");
            goto realloc_exit;
        }
        old_size *= 2;
    }
    printf("\n");
    for (size_t i = 0; i < 25; i++) {
        new_size = old_size / 2;
        new_ptr = ::realloc(mem_ptr, new_size);
        if (new_ptr != nullptr) {
            if (mem_ptr != new_ptr) {
                printf("    old-ptr: 0x%08X%08X, new-ptr: 0x%08X%08X, size: [%8u -> %-8u] bytes\n",
                        PTR2HEX16(mem_ptr), PTR2HEX16(new_ptr), (uint32_t)old_size, (uint32_t)new_size);
            }
            else {
                printf("(*) old-ptr: 0x%08X%08X, new-ptr: 0x%08X%08X, size: [%8u -> %-8u] bytes\n",
                        PTR2HEX16(mem_ptr), PTR2HEX16(new_ptr), (uint32_t)old_size, (uint32_t)new_size);
            }
            mem_ptr = new_ptr;
        }
        else {
            printf("realloc() error.\n\n");
            goto realloc_exit;
        }
        old_size /= 2;
    }
    printf("\n");
    for (size_t i = 0; i < 25; i++) {
        new_size = old_size * 2;
        new_ptr = ::realloc(mem_ptr, new_size);
        if (new_ptr != nullptr) {
            if (mem_ptr != new_ptr) {
                printf("    old-ptr: 0x%08X%08X, new-ptr: 0x%08X%08X, size: [%8u -> %-8u] bytes\n",
                        PTR2HEX16(mem_ptr), PTR2HEX16(new_ptr), (uint32_t)old_size, (uint32_t)new_size);
            }
            else {
                printf("(*) old-ptr: 0x%08X%08X, new-ptr: 0x%08X%08X, size: [%8u -> %-8u] bytes\n",
                        PTR2HEX16(mem_ptr), PTR2HEX16(new_ptr), (uint32_t)old_size, (uint32_t)new_size);
            }
            mem_ptr = new_ptr;
        }
        else {
            printf("realloc() error.\n\n");
            goto realloc_exit;
        }
        old_size *= 2;
    }
realloc_exit:
    printf("\n");
    if (mem_ptr != nullptr)
        ::free(mem_ptr);
}

#ifdef _MSC_VER

//
// MSVC: _expand(void * ptr, size_t size);
// https://docs.microsoft.com/en-us/cpp/c-runtime-library/reference/expand?view=msvc-160
//

void expand_test()
{
    void * mem_ptr, * new_ptr;
    size_t old_size = 4, new_size;
    mem_ptr = ::malloc(old_size);
    if (mem_ptr == nullptr)
        return;
    printf("expand_test()\n");
    printf("\n");
    for (size_t i = 0; i < 25; i++) {
        new_size = old_size * 2;
        new_ptr = ::_expand(mem_ptr, new_size);
        if (new_ptr != nullptr) {
            if (mem_ptr != new_ptr) {
                printf("    old-ptr: 0x%08X%08X, new-ptr: 0x%08X%08X, size: [%8u -> %-8u] bytes\n",
                        PTR2HEX16(mem_ptr), PTR2HEX16(new_ptr), (uint32_t)old_size, (uint32_t)new_size);
            }
            else {
                printf("(*) old-ptr: 0x%08X%08X, new-ptr: 0x%08X%08X, size: [%8u -> %-8u] bytes\n",
                        PTR2HEX16(mem_ptr), PTR2HEX16(new_ptr), (uint32_t)old_size, (uint32_t)new_size);
            }
            mem_ptr = new_ptr;
        }
        else {
            printf("_expand() error.\n\n");
            return;
        }
        old_size *= 2;
    }
    printf("\n");
    for (size_t i = 0; i < 25; i++) {
        new_size = old_size / 2;
        new_ptr = ::_expand(mem_ptr, new_size);
        if (new_ptr != nullptr) {
            if (mem_ptr != new_ptr) {
                printf("    old-ptr: 0x%08X%08X, new-ptr: 0x%08X%08X, size: [%8u -> %-8u] bytes\n",
                        PTR2HEX16(mem_ptr), PTR2HEX16(new_ptr), (uint32_t)old_size, (uint32_t)new_size);
            }
            else {
                printf("(*) old-ptr: 0x%08X%08X, new-ptr: 0x%08X%08X, size: [%8u -> %-8u] bytes\n",
                        PTR2HEX16(mem_ptr), PTR2HEX16(new_ptr), (uint32_t)old_size, (uint32_t)new_size);
            }
            mem_ptr = new_ptr;
        }
        else {
            printf("_expand() error.\n\n");
            goto expand_exit;
        }
        old_size /= 2;
    }
    printf("\n");
    for (size_t i = 0; i < 25; i++) {
        new_size = old_size * 2;
        new_ptr = ::_expand(mem_ptr, new_size);
        if (new_ptr != nullptr) {
            if (mem_ptr != new_ptr) {
                printf("    old-ptr: 0x%08X%08X, new-ptr: 0x%08X%08X, size: [%8u -> %-8u] bytes\n",
                        PTR2HEX16(mem_ptr), PTR2HEX16(new_ptr), (uint32_t)old_size, (uint32_t)new_size);
            }
            else {
                printf("(*) old-ptr: 0x%08X%08X, new-ptr: 0x%08X%08X, size: [%8u -> %-8u] bytes\n",
                        PTR2HEX16(mem_ptr), PTR2HEX16(new_ptr), (uint32_t)old_size, (uint32_t)new_size);
            }
            mem_ptr = new_ptr;
        }
        else {
            printf("_expand() error.\n\n");
            goto expand_exit;
        }
        old_size *= 2;
    }
expand_exit:
    printf("\n");
    if (mem_ptr != nullptr)
        ::free(mem_ptr);
}

#endif // _MSC_VER

void jm_aligned_realloc_test()
{
    void * mem_ptr, * new_ptr;
    size_t old_size = 4, new_size;
    size_t alignment = jstd::allocator<int>::kMallocDefaultAlignment;
    mem_ptr = jm_aligned_malloc(old_size, alignment);
    if (mem_ptr == nullptr)
        return;
    printf("jm_aligned_realloc_test(): alignment = %u\n", (uint32_t)alignment);
    printf("\n");
    for (size_t i = 0; i < 25; i++) {
        new_size = old_size * 2;
        new_ptr = jm_aligned_realloc(mem_ptr, new_size, alignment);
        if (new_ptr != nullptr) {
            if (mem_ptr != new_ptr) {
                printf("    old-ptr: 0x%08X%08X, new-ptr: 0x%08X%08X, size: [%8u -> %-8u] bytes\n",
                        PTR2HEX16(mem_ptr), PTR2HEX16(new_ptr), (uint32_t)old_size, (uint32_t)new_size);
            }
            else {
                printf("(*) old-ptr: 0x%08X%08X, new-ptr: 0x%08X%08X, size: [%8u -> %-8u] bytes\n",
                        PTR2HEX16(mem_ptr), PTR2HEX16(new_ptr), (uint32_t)old_size, (uint32_t)new_size);
            }
            mem_ptr = new_ptr;
        }
        else {
            printf("jm_aligned_realloc() error.\n\n");
            goto realloc_exit;
        }
        old_size *= 2;
    }
    printf("\n");
    for (size_t i = 0; i < 25; i++) {
        new_size = old_size / 2;
        new_ptr = jm_aligned_realloc(mem_ptr, new_size, alignment);
        if (new_ptr != nullptr) {
            if (mem_ptr != new_ptr) {
                printf("    old-ptr: 0x%08X%08X, new-ptr: 0x%08X%08X, size: [%8u -> %-8u] bytes\n",
                        PTR2HEX16(mem_ptr), PTR2HEX16(new_ptr), (uint32_t)old_size, (uint32_t)new_size);
            }
            else {
                printf("(*) old-ptr: 0x%08X%08X, new-ptr: 0x%08X%08X, size: [%8u -> %-8u] bytes\n",
                        PTR2HEX16(mem_ptr), PTR2HEX16(new_ptr), (uint32_t)old_size, (uint32_t)new_size);
            }
            mem_ptr = new_ptr;
        }
        else {
            printf("jm_aligned_realloc() error.\n\n");
            goto realloc_exit;
        }
        old_size /= 2;
    }
    printf("\n");
    for (size_t i = 0; i < 25; i++) {
        new_size = old_size * 2;
        new_ptr = jm_aligned_realloc(mem_ptr, new_size, alignment);
        if (new_ptr != nullptr) {
            if (mem_ptr != new_ptr) {
                printf("    old-ptr: 0x%08X%08X, new-ptr: 0x%08X%08X, size: [%8u -> %-8u] bytes\n",
                        PTR2HEX16(mem_ptr), PTR2HEX16(new_ptr), (uint32_t)old_size, (uint32_t)new_size);
            }
            else {
                printf("(*) old-ptr: 0x%08X%08X, new-ptr: 0x%08X%08X, size: [%8u -> %-8u] bytes\n",
                        PTR2HEX16(mem_ptr), PTR2HEX16(new_ptr), (uint32_t)old_size, (uint32_t)new_size);
            }
            mem_ptr = new_ptr;
        }
        else {
            printf("jm_aligned_realloc() error.\n\n");
            goto realloc_exit;
        }
        old_size *= 2;
    }
realloc_exit:
    printf("\n");
    if (mem_ptr != nullptr)
        jm_aligned_free(mem_ptr, alignment);
}

bool read_dict_file(const std::string & filename)
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
    if (argc == 2) {
        std::string filename = argv[1];
        bool read_ok = read_dict_file(filename);
        dict_words_is_ready = read_ok;
        dict_filename = filename;
    }

    if (!dict_words_is_ready) {
        jtest::CPU::warm_up(1000);
    }

    if (1) string_view_test();
    if (0) shiftable_ptr_test();
    if (0) formatter_test();
    if (1) fnv1a_hash_test();
    if (1) realloc_test();
#ifdef _MSC_VER
    if (0) expand_test();
#endif
    if (1) jm_aligned_realloc_test();

    if (0) formatter_benchmark();
    if (1) hashtable_uinttest();
    if (1) hashtable_benchmark();

    printf("sizeof(long double) = %u\n\n", (uint32_t)sizeof(long double));
    printf("sizeof(std::max_align_t) = %u\n\n", (uint32_t)sizeof(std::max_align_t));

    jstd::Console::ReadKey();
    return 0;
}
