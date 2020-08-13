
// Enable Visual Leak Detector (For Visual Studio)
#define JSTD_ENABLE_VLD     1

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

#include <jstd/hash/hash_table.h>
#include <jstd/hash/dictionary.h>
#include <jstd/hash/hashmap_analyzer.h>
#include <jstd/string/string_view.h>
#include <jstd/system/Console.h>
#include <jstd/test/StopWatch.h>
#include <jstd/test/CPUWarmUp.h>
#include <jstd/test/ProcessMemInfo.h>

//#include <jstd/all.h>

#include <iostream>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <string>
#include <atomic>
#include <thread>
#include <ratio>
#include <chrono>
#include <memory>
#include <utility>
#include <map>
#include <unordered_map>

using namespace jstd;

std::vector<std::string> dict_words;
bool dict_words_is_ready = false;

static const std::size_t kInitCapacity = 16;

#ifdef NDEBUG
static const std::size_t kIterations = 3000000;
#else
static const std::size_t kIterations = 10000;
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
struct hash<jstd::StringRef> {
    typedef std::uint32_t   result_type;

    jstd::hash_helper<jstd::StringRef, std::uint32_t, HashFunc_CRC32C> hash_helper_;

    result_type operator()(const jstd::StringRef & key) const {
        return hash_helper_.getHashCode(key);
    }
};

} // namespace std

namespace jstd {

template <>
struct hash<jstd::StringRef> {
    typedef std::uint32_t   result_type;

    jstd::hash_helper<jstd::StringRef, std::uint32_t, HashFunc_CRC32C> hash_helper_;

    result_type operator()(const jstd::StringRef & key) const {
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

    const char * name() {
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

    const char * name() {
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

    const char * name() {
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
        printf(" %-36s  ", algorithm.name());
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
#ifndef NDEBUG
    static const size_t kRepeatTimes = 100;
#else
    static const size_t kRepeatTimes = (kIterations / kHeaderFieldSize);
#endif

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

        AlgorithmTy algorithm;
        printf("---------------------------------------------------------------------------\n");
        printf(" %-36s  ", algorithm.name());
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
#ifndef NDEBUG
    static const size_t kRepeatTimes = 100;
#else
    static const size_t kRepeatTimes = (kIterations / kHeaderFieldSize);
#endif

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

        AlgorithmTy algorithm;
        printf("---------------------------------------------------------------------------\n");
        printf(" %-36s  ", algorithm.name());
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
#ifndef NDEBUG
    static const size_t kRepeatTimes = 100;
#else
    static const size_t kRepeatTimes = (kIterations / kHeaderFieldSize);
#endif

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

        AlgorithmTy algorithm;
        printf("---------------------------------------------------------------------------\n");
        printf(" %-36s  ", algorithm.name());
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
#ifndef NDEBUG
    static const size_t kRepeatTimes = 100;
#else
    static const size_t kRepeatTimes = (kIterations / kHeaderFieldSize);
#endif

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
        printf(" %-36s  ", algorithm.name());
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
    StringRef field_str[kHeaderFieldSize];
    StringRef index_str[kHeaderFieldSize];
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
        printf(" %-36s  ", algorithm.name());
        printf("sum = %-10" PRIuPTR "  time: %8.3f ms\n", checksum, sw.getElapsedMillisec());
    }
}

void hashtable_ref_find_benchmark()
{
    std::cout << "  hashtable_ref_find_benchmark()" << std::endl;
    std::cout << std::endl;

    //hashtable_ref_find_benchmark<test::std_map<StringRef, StringRef>>();
    hashtable_ref_find_benchmark<test::std_unordered_map<StringRef, StringRef>>();

#if USE_JSTD_HASH_TABLE
    hashtable_ref_find_benchmark<test::hash_table_impl<jstd::hash_table<StringRef, StringRef>>>();
    hashtable_ref_find_benchmark<test::hash_table_impl<jstd::hash_table_time31<StringRef, StringRef>>>();
    hashtable_ref_find_benchmark<test::hash_table_impl<jstd::hash_table_time31_std<StringRef, StringRef>>>();
#endif

#if USE_JSTD_DICTIONARY
    hashtable_ref_find_benchmark<test::hash_table_impl<jstd::Dictionary<StringRef, StringRef>>>();
    hashtable_ref_find_benchmark<test::hash_table_impl<jstd::Dictionary_Time31<StringRef, StringRef>>>();
    hashtable_ref_find_benchmark<test::hash_table_impl<jstd::Dictionary_Time31Std<StringRef, StringRef>>>();
#else
    hashtable_ref_find_benchmark<test::hash_table_impl<jstd::Dictionary<StringRef, StringRef>>>();
#endif

    printf("---------------------------------------------------------------------------\n");
    printf("\n");
}

template <typename AlgorithmTy>
void hashtable_ref_insert_benchmark_impl()
{
#ifndef NDEBUG
    static const size_t kRepeatTimes = 100;
#else
    static const size_t kRepeatTimes = (kIterations / kHeaderFieldSize);
#endif

    std::string index_buf[kHeaderFieldSize];
    StringRef field_str[kHeaderFieldSize];
    StringRef index_str[kHeaderFieldSize];
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

        AlgorithmTy algorithm;
        printf("---------------------------------------------------------------------------\n");
        printf(" %-36s  ", algorithm.name());
        printf("sum = %-10" PRIuPTR "  time: %8.3f ms\n", checksum, totalTime);
    }
}

void hashtable_ref_insert_benchmark()
{
    std::cout << "  hashtable_ref_insert_benchmark()" << std::endl;
    std::cout << std::endl;

    //hashtable_ref_emplace_benchmark_impl<test::std_map<StringRef, StringRef>>();
    hashtable_ref_insert_benchmark_impl<test::std_unordered_map<StringRef, StringRef>>();

#if USE_JSTD_HASH_TABLE
    hashtable_ref_insert_benchmark_impl<test::hash_table_impl<jstd::hash_table<StringRef, StringRef>>>();
    hashtable_ref_insert_benchmark_impl<test::hash_table_impl<jstd::hash_table_time31<StringRef, StringRef>>>();
    hashtable_ref_insert_benchmark_impl<test::hash_table_impl<jstd::hash_table_time31_std<StringRef, StringRef>>>();
#endif

#if USE_JSTD_DICTIONARY
    hashtable_ref_insert_benchmark_impl<test::hash_table_impl<jstd::Dictionary<StringRef, StringRef>>>();
    hashtable_ref_insert_benchmark_impl<test::hash_table_impl<jstd::Dictionary_Time31<StringRef, StringRef>>>();
    hashtable_ref_insert_benchmark_impl<test::hash_table_impl<jstd::Dictionary_Time31Std<StringRef, StringRef>>>();
#else
    hashtable_ref_insert_benchmark_impl<test::hash_table_impl<jstd::Dictionary<StringRef, StringRef>>>();
#endif

    printf("---------------------------------------------------------------------------\n");
    printf("\n");
}

template <typename AlgorithmTy>
void hashtable_ref_emplace_benchmark_impl()
{
#ifndef NDEBUG
    static const size_t kRepeatTimes = 100;
#else
    static const size_t kRepeatTimes = (kIterations / kHeaderFieldSize);
#endif

    std::string index_buf[kHeaderFieldSize];
    StringRef field_str[kHeaderFieldSize];
    StringRef index_str[kHeaderFieldSize];
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

        AlgorithmTy algorithm;
        printf("---------------------------------------------------------------------------\n");
        printf(" %-36s  ", algorithm.name());
        printf("sum = %-10" PRIuPTR "  time: %8.3f ms\n", checksum, totalTime);
    }
}

void hashtable_ref_emplace_benchmark()
{
    std::cout << "  hashtable_ref_emplace_benchmark()" << std::endl;
    std::cout << std::endl;

    //hashtable_ref_emplace_benchmark_impl<test::std_map<StringRef, StringRef>>();
    hashtable_ref_emplace_benchmark_impl<test::std_unordered_map<StringRef, StringRef>>();

#if USE_JSTD_HASH_TABLE
    hashtable_ref_emplace_benchmark_impl<test::hash_table_impl<jstd::hash_table<StringRef, StringRef>>>();
    hashtable_ref_emplace_benchmark_impl<test::hash_table_impl<jstd::hash_table_time31<StringRef, StringRef>>>();
    hashtable_ref_emplace_benchmark_impl<test::hash_table_impl<jstd::hash_table_time31_std<StringRef, StringRef>>>();
#endif

#if USE_JSTD_DICTIONARY
    hashtable_ref_emplace_benchmark_impl<test::hash_table_impl<jstd::Dictionary<StringRef, StringRef>>>();
    hashtable_ref_emplace_benchmark_impl<test::hash_table_impl<jstd::Dictionary_Time31<StringRef, StringRef>>>();
    hashtable_ref_emplace_benchmark_impl<test::hash_table_impl<jstd::Dictionary_Time31Std<StringRef, StringRef>>>();
#else
    hashtable_ref_emplace_benchmark_impl<test::hash_table_impl<jstd::Dictionary<StringRef, StringRef>>>();
#endif

    printf("---------------------------------------------------------------------------\n");
    printf("\n");
}

template <typename AlgorithmTy>
void hashtable_ref_erase_benchmark_impl()
{
#ifndef NDEBUG
    static const size_t kRepeatTimes = 100;
#else
    static const size_t kRepeatTimes = (kIterations / kHeaderFieldSize);
#endif

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

        AlgorithmTy algorithm;
        printf("---------------------------------------------------------------------------\n");
        printf(" %-36s  ", algorithm.name());
        printf("sum = %-10" PRIuPTR "  time: %8.3f ms\n", checksum, totalTime);
    }
}

void hashtable_ref_erase_benchmark()
{
    std::cout << "  hashtable_ref_erase_benchmark()" << std::endl;
    std::cout << std::endl;

    //hashtable_ref_erase_benchmark_impl<test::std_map<StringRef, StringRef>>();
    hashtable_ref_erase_benchmark_impl<test::std_unordered_map<StringRef, StringRef>>();

#if USE_JSTD_HASH_TABLE
    hashtable_ref_erase_benchmark_impl<test::hash_table_impl<jstd::hash_table<StringRef, StringRef>>>();
    hashtable_ref_erase_benchmark_impl<test::hash_table_impl<jstd::hash_table_time31<StringRef, StringRef>>>();
    hashtable_ref_erase_benchmark_impl<test::hash_table_impl<jstd::hash_table_time31_std<StringRef, StringRef>>>();
#endif

#if USE_JSTD_DICTIONARY
    hashtable_ref_erase_benchmark_impl<test::hash_table_impl<jstd::Dictionary<StringRef, StringRef>>>();
    hashtable_ref_erase_benchmark_impl<test::hash_table_impl<jstd::Dictionary_Time31<StringRef, StringRef>>>();
    hashtable_ref_erase_benchmark_impl<test::hash_table_impl<jstd::Dictionary_Time31Std<StringRef, StringRef>>>();
#else
    hashtable_ref_erase_benchmark_impl<test::hash_table_impl<jstd::Dictionary<StringRef, StringRef>>>();
#endif

    printf("---------------------------------------------------------------------------\n");
    printf("\n");
}

template <typename AlgorithmTy>
void hashtable_ref_insert_erase_benchmark_impl()
{
#ifndef NDEBUG
    static const size_t kRepeatTimes = 100;
#else
    static const size_t kRepeatTimes = (kIterations / kHeaderFieldSize);
#endif

    std::string index_buf[kHeaderFieldSize];
    StringRef field_str[kHeaderFieldSize];
    StringRef index_str[kHeaderFieldSize];
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
        printf(" %-36s  ", algorithm.name());
        printf("sum = %-10" PRIuPTR "  time: %8.3f ms\n", checksum, sw.getElapsedMillisec());
    }
}

void hashtable_ref_insert_erase_benchmark()
{
    std::cout << "  hashtable_ref_insert_erase_benchmark()" << std::endl;
    std::cout << std::endl;

    //hashtable_ref_insert_erase_benchmark_impl<test::std_map<StringRef, StringRef>>();
    hashtable_ref_insert_erase_benchmark_impl<test::std_unordered_map<StringRef, StringRef>>();

#if USE_JSTD_HASH_TABLE
    hashtable_ref_insert_erase_benchmark_impl<test::hash_table_impl<jstd::hash_table<StringRef, StringRef>>>();
    hashtable_ref_insert_erase_benchmark_impl<test::hash_table_impl<jstd::hash_table_time31<StringRef, StringRef>>>();
    hashtable_ref_insert_erase_benchmark_impl<test::hash_table_impl<jstd::hash_table_time31_std<StringRef, StringRef>>>();
#endif

#if USE_JSTD_DICTIONARY
    hashtable_ref_insert_erase_benchmark_impl<test::hash_table_impl<jstd::Dictionary<StringRef, StringRef>>>();
    hashtable_ref_insert_erase_benchmark_impl<test::hash_table_impl<jstd::Dictionary_Time31<StringRef, StringRef>>>();
    hashtable_ref_insert_erase_benchmark_impl<test::hash_table_impl<jstd::Dictionary_Time31Std<StringRef, StringRef>>>();
#else
    hashtable_ref_insert_erase_benchmark_impl<test::hash_table_impl<jstd::Dictionary<StringRef, StringRef>>>();
#endif

    printf("---------------------------------------------------------------------------\n");
    printf("\n");
}

template <typename AlgorithmTy>
void hashtable_rehash_benchmark_impl()
{
#ifndef NDEBUG
    static const size_t kRepeatTimes = 2;
#else
    static const size_t kRepeatTimes = (kIterations / kHeaderFieldSize);
#endif

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
        algorithm.reserve(buckets);

        for (size_t i = 0; i < kHeaderFieldSize; ++i) {
            algorithm.emplace(field_str[i], index_str[i]);
        }

        jtest::StopWatch sw;

        sw.start();
        for (size_t i = 0; i < kRepeatTimes; ++i) {
            checksum += algorithm.size();

            buckets = 128;
            algorithm.shrink_to_fit(buckets - 1);
            checksum += algorithm.bucket_count();
#ifndef NDEBUG
            if (algorithm.bucket_count() != buckets) {
                size_t bucket_count = algorithm.bucket_count();
                printf("shrink_to(): size = %" PRIuPTR ", buckets = %" PRIuPTR ", bucket_count = %" PRIuPTR "\n",
                       algorithm.size(), buckets, bucket_count);
            }
#endif
            for (size_t j = 0; j < 7; ++j) {
                buckets *= 2;
                algorithm.rehash(buckets - 1);
                checksum += algorithm.bucket_count();
#ifndef NDEBUG
                if (algorithm.bucket_count() != buckets) {
                    size_t bucket_count = algorithm.bucket_count();
                    printf("rehash(%u):   size = %" PRIuPTR ", buckets = %" PRIuPTR ", bucket_count = %" PRIuPTR "\n",
                           (uint32_t)j, algorithm.size(), buckets, bucket_count);
                }
#endif
            }
        }
        sw.stop();

        printf("---------------------------------------------------------------------------\n");
        printf(" %-36s  ", algorithm.name());
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
#ifndef NDEBUG
    static const size_t kRepeatTimes = 2;
#else
    static const size_t kRepeatTimes = (kIterations / kHeaderFieldSize / 2);
#endif

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
            algorithm.shrink_to_fit(buckets - 1);
#ifndef NDEBUG
            if (algorithm.bucket_count() != buckets) {
                size_t bucket_count = algorithm.bucket_count();
                printf("shrink_to(): size = %" PRIuPTR ", buckets = %" PRIuPTR ", bucket_count = %" PRIuPTR "\n",
                       algorithm.size(), buckets, bucket_count);
            }
#endif
            checksum += algorithm.bucket_count();

            for (size_t j = 0; j < 7; ++j) {
                buckets *= 2;
                algorithm.rehash(buckets - 1);
#ifndef NDEBUG
                if (algorithm.bucket_count() != buckets) {
                    size_t bucket_count = algorithm.bucket_count();
                    printf("rehash(%u):   size = %" PRIuPTR ", buckets = %" PRIuPTR ", bucket_count = %" PRIuPTR "\n",
                           (uint32_t)j, algorithm.size(), buckets, bucket_count);
                }
#endif
                checksum += algorithm.bucket_count();
            }
        }
        sw.stop();

        AlgorithmTy algorithm;
        printf("---------------------------------------------------------------------------\n");
        printf(" %-36s  ", algorithm.name());
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
    for (size_t j = 0; j < kHeaderFieldSize; ++j) {
        container.emplace(field_str[j], index_str[j]);
    }

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

    HashMapAnalyzer<Container> analyzer(container);
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

    HashMapAnalyzer<Container> analyzer(container);
    analyzer.set_name(name);
    analyzer.start_analyse();

    analyzer.display_status();
    analyzer.dump_entries(100);
}

void hashtable_uinttest()
{
    if (dict_words_is_ready && dict_words.size() > 0) {
        hashtable_dict_words_show_status<jstd::Dictionary<std::string, std::string>>("Dictionary<std::string, std::string>");
        hashtable_dict_words_show_status<jstd::Dictionary_Time31<std::string, std::string>>("Dictionary_Time31<std::string, std::string>");

        hashtable_dict_words_show_status<std::unordered_map<std::string, std::string>>("std::unordered_map<std::string, std::string>");
    }
    else {
        hashtable_show_status<jstd::Dictionary<std::string, std::string>>("Dictionary<std::string, std::string>");
        hashtable_show_status<jstd::Dictionary_Time31<std::string, std::string>>("Dictionary_Time31<std::string, std::string>");

        hashtable_show_status<std::unordered_map<std::string, std::string>>("std::unordered_map<std::string, std::string>");
    }
    
    //hashtable_iterator_uinttest<jstd::Dictionary<std::string, std::string>>();
}

void string_view_test()
{
    string_view sv1(header_fields[4]);
    //sv1[2] = 'a';  // Error, readonly

    std::string str1("12345676890");

    printf("str1 = %s\n", str1.c_str());

    string_view sv2("abcdefg");
    std::size_t n2 = sv2.copy((char *)str1.data(), 4, 0);

    printf("str1 = %s\n", str1.c_str());

    string_view sv3(str1);
    sv3[2] = 'q';

    printf("str1 = %s\n", str1.c_str());
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
    }

    if (!dict_words_is_ready) {
        jtest::CPU::warmup(1000);
    }

    //string_view_test();

    hashtable_uinttest();
    hashtable_benchmark();

    jstd::Console::ReadKey();
    return 0;
}
