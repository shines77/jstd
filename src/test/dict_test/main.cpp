
// Enable Visual Leak Detector (Visual Studio)
#define JSTD_ENABLE_VLD     1

#ifdef _MSC_VER
#if JSTD_ENABLE_VLD
#include <vld.h>
#endif
#endif

#ifndef __SSE4_2__
#define __SSE4_2__              1
#endif // __SSE4_2__

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

#if __SSE4_2__

// Support SSE 4.2: _mm_crc32_u32(), _mm_crc32_u64().
#define SUPPORT_SSE42_CRC32C    1

// Support Intel SMID SHA module: sha1 & sha256, it's higher than SSE 4.2 .
// _mm_sha1msg1_epu32(), _mm_sha1msg2_epu32() and so on.
#define SUPPORT_SMID_SHA        0

#endif // __SSE4_2__

// String compare mode
#define STRING_UTILS_LIBC     0
#define STRING_UTILS_U64      1
#define STRING_UTILS_SSE42    2

#define STRING_UTILS_MODE     STRING_UTILS_SSE42

// Use in <jstd/support/PowerOf2.h>
#define JSTD_SUPPORT_X86_BITSCAN_INSTRUCTION    1

#include <jstd/basic/stddef.h>
#include <jstd/basic/stdint.h>
#include <jstd/basic/inttypes.h>

#include <jstd/string/StringRef.h>
#include <jstd/hash/hash_table.h>
#include <jstd/hash/dictionary.h>
#include <jstd/support/StopWatch.h>

#include <jstd/all.h>

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

#if defined(_MSC_VER)
#include <io.h>
#include <process.h>
#include <psapi.h>
#else
// defined(__linux__) || defined(__clang__) || defined(__FreeBSD__) || (defined(__GNUC__) && defined(__cygwin__))
#include <unistd.h>
#endif // _MSC_VER

using namespace jstd;

#ifdef NDEBUG
static const std::size_t kIterations = 3000000;
#else
static const std::size_t kIterations = 10000;
#endif

static const std::size_t kInitCapacity = 16;

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

class ListDemo {
public:
    ListDemo() {}
    ~ListDemo() {}
};

template <class T>
class ListIterator : public jstd::iterator<
        jstd::random_access_iterator_tag, ListIterator<T>
    > {
public:
    typedef jstd::iterator<jstd::random_access_iterator_tag, ListIterator<T>>
                                                    base_type;
    typedef ListIterator<T>                         this_type;

    typedef typename base_type::iterator_category   iterator_category;
    typedef typename base_type::value_type          value_type;
    typedef typename base_type::difference_type     difference_type;

    typedef typename base_type::pointer             pointer;
    typedef typename base_type::reference           reference;

private:
    T * ptr_;

public:
    ListIterator(T * ptr = nullptr) : ptr_(ptr) {
    }
    template <class Other>
    ListIterator(const ListIterator<Other> & src) : ptr_(src.ptr()) {
    }
    ~ListIterator() {
    }

    // return wrapped iterator
	T * ptr() {
        return ptr_;
    }

    // return wrapped iterator
	const T * ptr() const {
        return ptr_;
    }

    // assign from compatible base
    template <class Other>
    this_type & operator = (const ListIterator<Other> & right) {
        ptr_ = right.ptr();
        return (*this);
    }

    // return designated value
    reference operator * () const {
        return (*const_cast<this_type *>(this));
    }

    // return pointer to class object
    pointer operator -> () const {
        return (std::pointer_traits<pointer>::pointer_to(**this));
    }

    this_type & operator ++ () {
        ++ptr_;
        return (*this);
    }

    this_type operator ++ (int) {
        this_type tmp = *this;
        ++ptr_;
        return tmp;
    }

    this_type & operator -- () {
        --ptr_;
        return (*this);
    }

    this_type operator -- (int) {
        this_type tmp = *this;
        --ptr_;
        return tmp;
    }

    // increment by integer
    this_type & operator += (difference_type offset) {
        ptr_ += offset;
        return (*this);
    }

    this_type operator + (difference_type offset) {
        return this_type(ptr_ + offset);
    }

    this_type & operator -= (difference_type offset) {
        ptr_ -= offset;
        return (*this);
    }

    this_type operator - (difference_type offset) {
        return this_type(ptr_ - offset);
    }
};

void run_iterator_test()
{
    bool is_iterator = jstd::is_iterator<ListIterator<ListDemo>>::value;

    ListIterator<ListDemo> iter;
    iter++;
    iter--;
    iter += std::ptrdiff_t(10);
    iter -= std::ptrdiff_t(5);

    jstd::reverse_iterator<ListIterator<ListDemo>> riter;
    riter++;
    riter--;
    riter += std::ptrdiff_t(10);
    riter -= std::ptrdiff_t(5);

    ListIterator<ListDemo> random_iter;
    jstd::iterator_utils::advance(random_iter, std::ptrdiff_t(1));

    jstd::Dictionary_crc32c<std::string, std::string> dict;
    dict.insert("1", "100");

    jstd::Dictionary_crc32c<std::wstring, std::wstring> wdict;
    wdict.insert(L"2", L"200");

    printf("dict_test.exe - %d, 0x%p, 0x%p\n\n",
           (int)is_iterator,
           (void *)iter.ptr(),
           (void *)riter.base().ptr());
}

#define USE_STRINGREF_STD_HASH  0

namespace std {

template <>
struct hash<jstd::StringRef> {
    typedef jstd::StringRef         argument_type;
    typedef std::size_t             result_type;
    typedef std::hash<std::string>  hasher_type;

#if USE_STRINGREF_STD_HASH

    std::hash<std::string> hasher_;

    result_type operator()(argument_type const & key) const {
        return hasher_(std::move(std::string(key.c_str(), key.size())));
    }

#else // !USE_STRINGREF_STD_HASH

#if SUPPORT_SSE42_CRC32C
    jstd::hash<jstd::StringRef, HashFunc_CRC32C, std::uint32_t> hasher_;
#else
    jstd::hash<jstd::StringRef, HashFunc_Time31, std::uint32_t> hasher_;
#endif

    result_type operator()(argument_type const & key) const {
        return hasher_(key);
    }

#endif // USE_STRINGREF_STD_HASH
};

} // namespace std

namespace jstd {

template <>
struct hash<jstd::StringRef> {
    typedef jstd::StringRef         argument_type;
    typedef std::size_t             result_type;

#if SUPPORT_SSE42_CRC32C
    jstd::hash<jstd::StringRef, HashFunc_CRC32C, std::uint32_t> hasher_;
#else
    jstd::hash<jstd::StringRef, HashFunc_Time31, std::uint32_t> hasher_;
#endif

    result_type operator()(argument_type const & key) const {
        return hasher_(key);
    }
};

} // namespace jstd

namespace test {

void cpu_warmup(int delayMillsecs)
{
#if defined(NDEBUG)
    double delayTimeLimit = (double)delayMillsecs / 1.0;
    volatile int sum = 0;

    printf("CPU warm-up begin ...\n");
    fflush(stdout);
    std::chrono::high_resolution_clock::time_point startTime, endTime;
    std::chrono::duration<double, std::ratio<1, 1000>> elapsedTime;
    startTime = std::chrono::high_resolution_clock::now();
    do {
        for (int i = 0; i < 500; ++i) {
            sum += i;
            for (int j = 5000; j >= 0; --j) {
                sum -= j;
            }
        }
        endTime = std::chrono::high_resolution_clock::now();
        elapsedTime = endTime - startTime;
    } while (elapsedTime.count() < delayTimeLimit);

    printf("sum = %u, time: %0.3f ms\n", sum, elapsedTime.count());
    printf("CPU warm-up end   ... \n\n");
    fflush(stdout);
#endif // !_DEBUG
}

#if defined(_MSC_VER)

//
// See: https://www.cnblogs.com/talenth/p/9762528.html
// See: https://blog.csdn.net/springontime/article/details/80625850
//

int get_current_process_mem_info(std::string & mem_size)
{
    HANDLE handle = ::GetCurrentProcess();

    PROCESS_MEMORY_COUNTERS_EX pmc = { 0 };
    if (!::GetProcessMemoryInfo(handle, (PROCESS_MEMORY_COUNTERS *)&pmc, sizeof(pmc))) {
        DWORD errCode = ::GetLastError();
        printf("GetProcessMemoryInfo() failed, lastErrorCode: %d\n\n", errCode);
        return -1;
    }

    char buf[256];

    // Physical memory currently occupied
    // WorkingSetSize: %d (KB)
    ::snprintf(buf, sizeof(buf), "%" PRIuPTR " KB\n", pmc.WorkingSetSize / 1024);
    mem_size = buf;

    return 0;
}

#else

//
// See: https://blog.csdn.net/weiyuefei/article/details/52281312
//

int get_current_process_mem_info(std::string & str_mem_size)
{
    pid_t pid = getpid();

    char filename[128];
    ::snprintf(filename, sizeof(filename), "/proc/%d/status", pid);

    std::ifstream ifs;
    ifs.open(filename, std::ios::in);
    if (!ifs.is_open()) {
        std::cout << "open " << filename << " error!" << std::endl << std::endl;
        return (-1);
    }

    char buf[512];
    char mem_size[64];
    char mem_unit[64];

    while (ifs.getline(buf, sizeof(buf) - 1)) {
        if (::strncmp(buf, "VmRSS:", 6) == 0) {
            ::sscanf(buf + 6, "%s%s", mem_size, mem_unit);
            str_mem_size = std::string(mem_size) + " " + std::string(mem_unit);
            break;
        }
    }

    ifs.close();
    return 0;
}

#endif // _MSC_VER

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

    void emplace(const key_type & key, const value_type & value) {
        this->map_.emplace(key, value);
    }

    void emplace(key_type && key, value_type && value) {
        this->map_.emplace(std::forward<key_type>(key),
                           std::forward<value_type>(value));
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

    void emplace(const key_type & key, const value_type & value) {
        std::pair<iterator, bool> result = this->map_.emplace(key, value);
    }

    void emplace(key_type && key, value_type && value) {
        this->map_.emplace(std::forward<key_type>(key),
                           std::forward<value_type>(value));
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

    void insert(key_type && key, value_type && value) {
        this->map_.insert(std::forward<key_type>(key),
                          std::forward<value_type>(value));
    }

    void emplace(const key_type & key, const value_type & value) {
        this->map_.emplace(key, value);
    }

    void emplace(key_type && key, value_type && value) {
        this->map_.emplace(std::forward<key_type>(key),
                           std::forward<value_type>(value));
    }

    size_type erase(const key_type & key) {
        return this->map_.erase(key);
    }
};

} // namespace test

template <typename AlgorithmTy>
void hashtable_find_benchmark()
{
    static const size_t kHeaderFieldSize = sizeof(header_fields) / sizeof(char *);
    static const size_t kRepeatTimes = (kIterations / kHeaderFieldSize);

    std::string field_str[kHeaderFieldSize];
    std::string index_str[kHeaderFieldSize];
    for (size_t i = 0; i < kHeaderFieldSize; ++i) {
        field_str[i].assign(header_fields[i]);
        char buf[16];
#ifdef _MSC_VER
        ::_itoa_s((int)i, buf, 10);
#else
        sprintf(buf, "%d", (int)i);
#endif
        index_str[i] = buf;
    }

    {
        typedef typename AlgorithmTy::iterator iterator;

        size_t checksum = 0;
        AlgorithmTy algorithm;
        for (size_t i = 0; i < kHeaderFieldSize; ++i) {
            algorithm.emplace(field_str[i], index_str[i]);
        }

        StopWatch sw;
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
        printf("sum = %-10" PRIuPTR "  time: %8.3f ms\n", checksum, sw.getMillisec());
    }
}

void run_hashtable_find_benchmark()
{
    std::cout << "-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=" << std::endl;
    std::cout << "  hashtable_find_benchmark()" << std::endl;
    std::cout << "-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=" << std::endl;
    std::cout << std::endl;

    hashtable_find_benchmark<test::std_map<std::string, std::string>>();
    hashtable_find_benchmark<test::std_unordered_map<std::string, std::string>>();

    hashtable_find_benchmark<test::hash_table_impl<jstd::hash_table<std::string, std::string>>>();
    hashtable_find_benchmark<test::hash_table_impl<jstd::hash_table_time31<std::string, std::string>>>();
    hashtable_find_benchmark<test::hash_table_impl<jstd::hash_table_time31_std<std::string, std::string>>>();

#if USE_JSTD_HASH_MAP
    hashtable_find_benchmark<test::hash_table_impl<jstd::hash_map<std::string, std::string>>>();
    hashtable_find_benchmark<test::hash_table_impl<jstd::hash_map_time31<std::string, std::string>>>();
    hashtable_find_benchmark<test::hash_table_impl<jstd::hash_map_time31_std<std::string, std::string>>>();
#endif // USE_JSTD_HASH_MAP

#if USE_JSTD_HASH_MAP_EX
    hashtable_find_benchmark<test::hash_table_impl<jstd::hash_map_ex<std::string, std::string>>>();
    hashtable_find_benchmark<test::hash_table_impl<jstd::hash_map_ex_time31<std::string, std::string>>>();
    hashtable_find_benchmark<test::hash_table_impl<jstd::hash_map_ex_time31_std<std::string, std::string>>>();
#endif // USE_JSTD_HASH_MAP_EX

#if USE_JSTD_DICTIONARY
    hashtable_find_benchmark<test::hash_table_impl<jstd::Dictionary<std::string, std::string>>>();
    hashtable_find_benchmark<test::hash_table_impl<jstd::Dictionary_Time31<std::string, std::string>>>();
    hashtable_find_benchmark<test::hash_table_impl<jstd::Dictionary_Time31Std<std::string, std::string>>>();
#endif // USE_JSTD_DICTIONARY

    printf("---------------------------------------------------------------------------\n");
    printf("\n");
}

template <typename AlgorithmTy>
void hashtable_insert_benchmark_impl()
{
    static const size_t kHeaderFieldSize = sizeof(header_fields) / sizeof(char *);
#ifndef NDEBUG
    static const size_t kRepeatTimes = 100;
#else
    static const size_t kRepeatTimes = (kIterations / kHeaderFieldSize);
#endif

    std::string field_str[kHeaderFieldSize];
    std::string index_str[kHeaderFieldSize];
    for (size_t i = 0; i < kHeaderFieldSize; ++i) {
        field_str[i].assign(header_fields[i]);
        char buf[16];
#ifdef _MSC_VER
        ::_itoa_s((int)i, buf, 10);
#else
        sprintf(buf, "%d", (int)i);
#endif
        index_str[i] = buf;
    }

    {
        size_t checksum = 0;
        double totalTime = 0.0;
        StopWatch sw;

        for (size_t i = 0; i < kRepeatTimes; ++i) {
            AlgorithmTy algorithm(kInitCapacity);
            sw.start();
            for (size_t j = 0; j < kHeaderFieldSize; ++j) {
                algorithm.insert(field_str[j], index_str[j]);
            }
            checksum += algorithm.size();
            sw.stop();

            totalTime += sw.getMillisec();
        }

        AlgorithmTy algorithm;
        printf("---------------------------------------------------------------------------\n");
        printf(" %-36s  ", algorithm.name());
        printf("sum = %-10" PRIuPTR "  time: %8.3f ms\n", checksum, totalTime);
    }
}

void run_hashtable_insert_benchmark()
{
    std::cout << "-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=" << std::endl;
    std::cout << "  hashtable_insert_benchmark()" << std::endl;
    std::cout << "-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=" << std::endl;
    std::cout << std::endl;

    hashtable_insert_benchmark_impl<test::std_map<std::string, std::string>>();
    hashtable_insert_benchmark_impl<test::std_unordered_map<std::string, std::string>>();

    hashtable_insert_benchmark_impl<test::hash_table_impl<jstd::hash_table<std::string, std::string>>>();
    hashtable_insert_benchmark_impl<test::hash_table_impl<jstd::hash_table_time31<std::string, std::string>>>();
    hashtable_insert_benchmark_impl<test::hash_table_impl<jstd::hash_table_time31_std<std::string, std::string>>>();

#if USE_JSTD_HASH_MAP
    hashtable_insert_benchmark_impl<test::hash_table_impl<jstd::hash_map<std::string, std::string>>>();
    hashtable_insert_benchmark_impl<test::hash_table_impl<jstd::hash_map_time31<std::string, std::string>>>();
    hashtable_insert_benchmark_impl<test::hash_table_impl<jstd::hash_map_time31_std<std::string, std::string>>>();
#endif // USE_JSTD_HASH_MAP

#if USE_JSTD_HASH_MAP_EX
    hashtable_insert_benchmark_impl<test::hash_table_impl<jstd::hash_map_ex<std::string, std::string>>>();
    hashtable_insert_benchmark_impl<test::hash_table_impl<jstd::hash_map_ex_time31<std::string, std::string>>>();
    hashtable_insert_benchmark_impl<test::hash_table_impl<jstd::hash_map_ex_time31_std<std::string, std::string>>>();
#endif // USE_JSTD_HASH_MAP_EX

#if USE_JSTD_DICTIONARY
    hashtable_insert_benchmark_impl<test::hash_table_impl<jstd::Dictionary<std::string, std::string>>>();
    hashtable_insert_benchmark_impl<test::hash_table_impl<jstd::Dictionary_Time31<std::string, std::string>>>();
    hashtable_insert_benchmark_impl<test::hash_table_impl<jstd::Dictionary_Time31Std<std::string, std::string>>>();
#endif // USE_JSTD_DICTIONARY

    printf("---------------------------------------------------------------------------\n");
    printf("\n");
}

template <typename AlgorithmTy>
void hashtable_emplace_benchmark_impl()
{
    static const size_t kHeaderFieldSize = sizeof(header_fields) / sizeof(char *);
#ifndef NDEBUG
    static const size_t kRepeatTimes = 100;
#else
    static const size_t kRepeatTimes = (kIterations / kHeaderFieldSize);
#endif

    std::string field_str[kHeaderFieldSize];
    std::string index_str[kHeaderFieldSize];
    for (size_t i = 0; i < kHeaderFieldSize; ++i) {
        field_str[i].assign(header_fields[i]);
        char buf[16];
#ifdef _MSC_VER
        ::_itoa_s((int)i, buf, 10);
#else
        sprintf(buf, "%d", (int)i);
#endif
        index_str[i] = buf;
    }

    {
        size_t checksum = 0;
        double totalTime = 0.0;
        StopWatch sw;

        for (size_t i = 0; i < kRepeatTimes; ++i) {
            AlgorithmTy algorithm(kInitCapacity);
            sw.start();
            for (size_t j = 0; j < kHeaderFieldSize; ++j) {
                algorithm.emplace(field_str[j], index_str[j]);
            }
            checksum += algorithm.size();
            sw.stop();

            totalTime += sw.getMillisec();
        }

        AlgorithmTy algorithm;
        printf("---------------------------------------------------------------------------\n");
        printf(" %-36s  ", algorithm.name());
        printf("sum = %-10" PRIuPTR "  time: %8.3f ms\n", checksum, totalTime);
    }
}

void run_hashtable_emplace_benchmark()
{
    std::cout << "-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=" << std::endl;
    std::cout << "  hashtable_emplace_benchmark()" << std::endl;
    std::cout << "-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=" << std::endl;
    std::cout << std::endl;

    hashtable_emplace_benchmark_impl<test::std_map<std::string, std::string>>();
    hashtable_emplace_benchmark_impl<test::std_unordered_map<std::string, std::string>>();

    hashtable_emplace_benchmark_impl<test::hash_table_impl<jstd::hash_table<std::string, std::string>>>();
    hashtable_emplace_benchmark_impl<test::hash_table_impl<jstd::hash_table_time31<std::string, std::string>>>();
    hashtable_emplace_benchmark_impl<test::hash_table_impl<jstd::hash_table_time31_std<std::string, std::string>>>();

#if USE_JSTD_HASH_MAP
    hashtable_emplace_benchmark_impl<test::hash_table_impl<jstd::hash_map<std::string, std::string>>>();
    hashtable_emplace_benchmark_impl<test::hash_table_impl<jstd::hash_map_time31<std::string, std::string>>>();
    hashtable_emplace_benchmark_impl<test::hash_table_impl<jstd::hash_map_time31_std<std::string, std::string>>>();
#endif // USE_JSTD_HASH_MAP

#if USE_JSTD_HASH_MAP_EX
    hashtable_emplace_benchmark_impl<test::hash_table_impl<jstd::hash_map_ex<std::string, std::string>>>();
    hashtable_emplace_benchmark_impl<test::hash_table_impl<jstd::hash_map_ex_time31<std::string, std::string>>>();
    hashtable_emplace_benchmark_impl<test::hash_table_impl<jstd::hash_map_ex_time31_std<std::string, std::string>>>();
#endif // USE_JSTD_HASH_MAP_EX

#if USE_JSTD_DICTIONARY
    hashtable_emplace_benchmark_impl<test::hash_table_impl<jstd::Dictionary<std::string, std::string>>>();
    hashtable_emplace_benchmark_impl<test::hash_table_impl<jstd::Dictionary_Time31<std::string, std::string>>>();
    hashtable_emplace_benchmark_impl<test::hash_table_impl<jstd::Dictionary_Time31Std<std::string, std::string>>>();
#endif // USE_JSTD_DICTIONARY

    printf("---------------------------------------------------------------------------\n");
    printf("\n");
}

template <typename AlgorithmTy>
void hashtable_erase_benchmark_impl()
{
    static const size_t kHeaderFieldSize = sizeof(header_fields) / sizeof(char *);
#ifndef NDEBUG
    static const size_t kRepeatTimes = 100;
#else
    static const size_t kRepeatTimes = (kIterations / kHeaderFieldSize);
#endif

    std::string field_str[kHeaderFieldSize];
    std::string index_str[kHeaderFieldSize];
    for (size_t i = 0; i < kHeaderFieldSize; ++i) {
        field_str[i].assign(header_fields[i]);
        char buf[16];
#ifdef _MSC_VER
        ::_itoa_s((int)i, buf, 10);
#else
        sprintf(buf, "%d", (int)i);
#endif
        index_str[i] = buf;
    }

    {
        size_t checksum = 0;
        double totalTime = 0.0;
        StopWatch sw;

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

            totalTime += sw.getMillisec();
        }

        AlgorithmTy algorithm;
        printf("---------------------------------------------------------------------------\n");
        printf(" %-36s  ", algorithm.name());
        printf("sum = %-10" PRIuPTR "  time: %8.3f ms\n", checksum, totalTime);
    }
}

void run_hashtable_erase_benchmark()
{
    std::cout << "-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=" << std::endl;
    std::cout << "  hashtable_erase_benchmark()" << std::endl;
    std::cout << "-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=" << std::endl;
    std::cout << std::endl;

    hashtable_erase_benchmark_impl<test::std_map<std::string, std::string>>();
    hashtable_erase_benchmark_impl<test::std_unordered_map<std::string, std::string>>();

    hashtable_erase_benchmark_impl<test::hash_table_impl<jstd::hash_table<std::string, std::string>>>();
    hashtable_erase_benchmark_impl<test::hash_table_impl<jstd::hash_table_time31<std::string, std::string>>>();
    hashtable_erase_benchmark_impl<test::hash_table_impl<jstd::hash_table_time31_std<std::string, std::string>>>();

#if USE_JSTD_HASH_MAP
    hashtable_erase_benchmark_impl<test::hash_table_impl<jstd::hash_map<std::string, std::string>>>();
    hashtable_erase_benchmark_impl<test::hash_table_impl<jstd::hash_map_time31<std::string, std::string>>>();
    hashtable_erase_benchmark_impl<test::hash_table_impl<jstd::hash_map_time31_std<std::string, std::string>>>();
#endif // USE_JSTD_HASH_MAP

#if USE_JSTD_HASH_MAP_EX
    hashtable_erase_benchmark_impl<test::hash_table_impl<jstd::hash_map_ex<std::string, std::string>>>();
    hashtable_erase_benchmark_impl<test::hash_table_impl<jstd::hash_map_ex_time31<std::string, std::string>>>();
    hashtable_erase_benchmark_impl<test::hash_table_impl<jstd::hash_map_ex_time31_std<std::string, std::string>>>();
#endif // USE_JSTD_HASH_MAP_EX

#if USE_JSTD_DICTIONARY
    hashtable_erase_benchmark_impl<test::hash_table_impl<jstd::Dictionary<std::string, std::string>>>();
    hashtable_erase_benchmark_impl<test::hash_table_impl<jstd::Dictionary_Time31<std::string, std::string>>>();
    hashtable_erase_benchmark_impl<test::hash_table_impl<jstd::Dictionary_Time31Std<std::string, std::string>>>();
#endif // USE_JSTD_DICTIONARY

    printf("---------------------------------------------------------------------------\n");
    printf("\n");
}

template <typename AlgorithmTy>
void hashtable_ref_erase_benchmark_impl()
{
    static const size_t kHeaderFieldSize = sizeof(header_fields) / sizeof(char *);
#ifndef NDEBUG
    static const size_t kRepeatTimes = 100;
#else
    static const size_t kRepeatTimes = (kIterations / kHeaderFieldSize);
#endif

    std::string field_str[kHeaderFieldSize];
    std::string index_str[kHeaderFieldSize];
    for (size_t i = 0; i < kHeaderFieldSize; ++i) {
        field_str[i].assign(header_fields[i]);
        char buf[16];
#ifdef _MSC_VER
        ::_itoa_s((int)i, buf, 10);
#else
        sprintf(buf, "%d", (int)i);
#endif
        index_str[i] = buf;
    }

    {
        size_t checksum = 0;
        double totalTime = 0.0;
        StopWatch sw;

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

            totalTime += sw.getMillisec();
        }

        AlgorithmTy algorithm;
        printf("---------------------------------------------------------------------------\n");
        printf(" %-36s  ", algorithm.name());
        printf("sum = %-10" PRIuPTR "  time: %8.3f ms\n", checksum, totalTime);
    }
}

void run_hashtable_ref_erase_benchmark()
{
    std::cout << "-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=" << std::endl;
    std::cout << "  hashtable_ref_erase_benchmark()" << std::endl;
    std::cout << "-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=" << std::endl;
    std::cout << std::endl;

    hashtable_ref_erase_benchmark_impl<test::std_map<StringRef, StringRef>>();
    hashtable_ref_erase_benchmark_impl<test::std_unordered_map<StringRef, StringRef>>();

    hashtable_ref_erase_benchmark_impl<test::hash_table_impl<jstd::hash_table<StringRef, StringRef>>>();
    hashtable_ref_erase_benchmark_impl<test::hash_table_impl<jstd::hash_table_time31<StringRef, StringRef>>>();
    hashtable_ref_erase_benchmark_impl<test::hash_table_impl<jstd::hash_table_time31_std<StringRef, StringRef>>>();

#if USE_JSTD_HASH_MAP
    hashtable_ref_erase_benchmark_impl<test::hash_table_impl<jstd::hash_map<StringRef, StringRef>>>();
    hashtable_ref_erase_benchmark_impl<test::hash_table_impl<jstd::hash_map_time31<StringRef, StringRef>>>();
    hashtable_ref_erase_benchmark_impl<test::hash_table_impl<jstd::hash_map_time31_std<StringRef, StringRef>>>();
#endif // USE_JSTD_HASH_MAP

#if USE_JSTD_HASH_MAP_EX
    hashtable_ref_erase_benchmark_impl<test::hash_table_impl<jstd::hash_map_ex<StringRef, StringRef>>>();
    hashtable_ref_erase_benchmark_impl<test::hash_table_impl<jstd::hash_map_ex_time31<StringRef, StringRef>>>();
    hashtable_ref_erase_benchmark_impl<test::hash_table_impl<jstd::hash_map_ex_time31_std<StringRef, StringRef>>>();
#endif // USE_JSTD_HASH_MAP_EX

#if USE_JSTD_DICTIONARY
    hashtable_ref_erase_benchmark_impl<test::hash_table_impl<jstd::Dictionary<StringRef, StringRef>>>();
    hashtable_ref_erase_benchmark_impl<test::hash_table_impl<jstd::Dictionary_Time31<StringRef, StringRef>>>();
    hashtable_ref_erase_benchmark_impl<test::hash_table_impl<jstd::Dictionary_Time31Std<StringRef, StringRef>>>();
#endif // USE_JSTD_DICTIONARY

    printf("---------------------------------------------------------------------------\n");
    printf("\n");
}

template <typename AlgorithmTy>
void hashtable_insert_erase_benchmark_impl()
{
    static const size_t kHeaderFieldSize = sizeof(header_fields) / sizeof(char *);
#ifndef NDEBUG
    static const size_t kRepeatTimes = 100;
#else
    static const size_t kRepeatTimes = (kIterations / kHeaderFieldSize);
#endif

    std::string field_str[kHeaderFieldSize];
    std::string index_str[kHeaderFieldSize];
    for (size_t i = 0; i < kHeaderFieldSize; ++i) {
        field_str[i].assign(header_fields[i]);
        char buf[16];
#ifdef _MSC_VER
        ::_itoa_s((int)i, buf, 10);
#else
        sprintf(buf, "%d", (int)i);
#endif
        index_str[i] = buf;
    }

    {
        size_t checksum = 0;
        AlgorithmTy algorithm;

        StopWatch sw;

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
        printf("sum = %-10" PRIuPTR "  time: %8.3f ms\n", checksum, sw.getMillisec());
    }
}

void run_hashtable_insert_erase_benchmark()
{
    std::cout << "-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=" << std::endl;
    std::cout << "  hashtable_insert_erase_benchmark()" << std::endl;
    std::cout << "-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=" << std::endl;
    std::cout << std::endl;

    hashtable_insert_erase_benchmark_impl<test::std_map<std::string, std::string>>();
    hashtable_insert_erase_benchmark_impl<test::std_unordered_map<std::string, std::string>>();

    hashtable_insert_erase_benchmark_impl<test::hash_table_impl<jstd::hash_table<std::string, std::string>>>();
    hashtable_insert_erase_benchmark_impl<test::hash_table_impl<jstd::hash_table_time31<std::string, std::string>>>();
    hashtable_insert_erase_benchmark_impl<test::hash_table_impl<jstd::hash_table_time31_std<std::string, std::string>>>();

#if USE_JSTD_HASH_MAP
    hashtable_insert_erase_benchmark_impl<test::hash_table_impl<jstd::hash_map<std::string, std::string>>>();
    hashtable_insert_erase_benchmark_impl<test::hash_table_impl<jstd::hash_map_time31<std::string, std::string>>>();
    hashtable_insert_erase_benchmark_impl<test::hash_table_impl<jstd::hash_map_time31_std<std::string, std::string>>>();
#endif // USE_JSTD_HASH_MAP

#if USE_JSTD_HASH_MAP_EX
    hashtable_insert_erase_benchmark_impl<test::hash_table_impl<jstd::hash_map_ex<std::string, std::string>>>();
    hashtable_insert_erase_benchmark_impl<test::hash_table_impl<jstd::hash_map_ex_time31<std::string, std::string>>>();
    hashtable_insert_erase_benchmark_impl<test::hash_table_impl<jstd::hash_map_ex_time31_std<std::string, std::string>>>();
#endif // USE_JSTD_HASH_MAP_EX

#if USE_JSTD_DICTIONARY
    hashtable_insert_erase_benchmark_impl<test::hash_table_impl<jstd::Dictionary<std::string, std::string>>>();
    hashtable_insert_erase_benchmark_impl<test::hash_table_impl<jstd::Dictionary_Time31<std::string, std::string>>>();
    hashtable_insert_erase_benchmark_impl<test::hash_table_impl<jstd::Dictionary_Time31Std<std::string, std::string>>>();
#endif // USE_JSTD_DICTIONARY

    printf("---------------------------------------------------------------------------\n");
    printf("\n");
}

template <typename AlgorithmTy>
void hashtable_ref_find_benchmark()
{
    static const size_t kHeaderFieldSize = sizeof(header_fields) / sizeof(char *);
    static const size_t kRepeatTimes = (kIterations / kHeaderFieldSize);

    std::string index_buf[kHeaderFieldSize];
    StringRef field_str[kHeaderFieldSize];
    StringRef index_str[kHeaderFieldSize];
    for (size_t i = 0; i < kHeaderFieldSize; ++i) {
        field_str[i].attach(header_fields[i]);
        char buf[16];
#ifdef _MSC_VER
        ::_itoa_s((int)i, buf, 10);
#else
        sprintf(buf, "%d", (int)i);
#endif
        index_buf[i] = buf;
        index_str[i] = index_buf[i];
    }

    {
        typedef typename AlgorithmTy::iterator iterator;

        size_t checksum = 0;
        AlgorithmTy algorithm;
        for (size_t i = 0; i < kHeaderFieldSize; ++i) {
            algorithm.emplace(field_str[i], index_str[i]);
        }

        StopWatch sw;
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
        printf("sum = %-10" PRIuPTR "  time: %8.3f ms\n", checksum, sw.getMillisec());
    }
}

void run_hashtable_ref_find_benchmark()
{
    std::cout << "-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=" << std::endl;
    std::cout << "  hashtable_ref_find_benchmark()" << std::endl;
    std::cout << "-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=" << std::endl;
    std::cout << std::endl;

    hashtable_ref_find_benchmark<test::std_map<StringRef, StringRef>>();
    hashtable_ref_find_benchmark<test::std_unordered_map<StringRef, StringRef>>();

    hashtable_ref_find_benchmark<test::hash_table_impl<jstd::hash_table<StringRef, StringRef>>>();
    hashtable_ref_find_benchmark<test::hash_table_impl<jstd::hash_table_time31<StringRef, StringRef>>>();
    hashtable_ref_find_benchmark<test::hash_table_impl<jstd::hash_table_time31_std<StringRef, StringRef>>>();

#if USE_JSTD_HASH_MAP
    hashtable_ref_find_benchmark<test::hash_table_impl<jstd::hash_map<StringRef, StringRef>>>();
    hashtable_ref_find_benchmark<test::hash_table_impl<jstd::hash_map_time31<StringRef, StringRef>>>();
    hashtable_ref_find_benchmark<test::hash_table_impl<jstd::hash_map_time31_std<StringRef, StringRef>>>();
#endif // USE_JSTD_HASH_MAP

#if USE_JSTD_HASH_MAP_EX
    hashtable_ref_find_benchmark<test::hash_table_impl<jstd::hash_map_ex<StringRef, StringRef>>>();
    hashtable_ref_find_benchmark<test::hash_table_impl<jstd::hash_map_ex_time31<StringRef, StringRef>>>();
    hashtable_ref_find_benchmark<test::hash_table_impl<jstd::hash_map_ex_time31_std<StringRef, StringRef>>>();
#endif // USE_JSTD_HASH_MAP_EX

#if USE_JSTD_DICTIONARY
    hashtable_ref_find_benchmark<test::hash_table_impl<jstd::Dictionary<StringRef, StringRef>>>();
    hashtable_ref_find_benchmark<test::hash_table_impl<jstd::Dictionary_Time31<StringRef, StringRef>>>();
    hashtable_ref_find_benchmark<test::hash_table_impl<jstd::Dictionary_Time31Std<StringRef, StringRef>>>();
#endif // USE_JSTD_DICTIONARY

    printf("---------------------------------------------------------------------------\n");
    printf("\n");
}

template <typename AlgorithmTy>
void hashtable_ref_emplace_benchmark_impl()
{
    static const size_t kHeaderFieldSize = sizeof(header_fields) / sizeof(char *);
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
        char buf[16];
#ifdef _MSC_VER
        ::_itoa_s((int)i, buf, 10);
#else
        sprintf(buf, "%d", (int)i);
#endif
        index_buf[i] = buf;
        index_str[i] = index_buf[i];
    }

    {
        size_t checksum = 0;
        double totalTime = 0.0;
        StopWatch sw;

        for (size_t i = 0; i < kRepeatTimes; ++i) {
            AlgorithmTy algorithm(kInitCapacity);
            sw.start();
            for (size_t j = 0; j < kHeaderFieldSize; ++j) {
                algorithm.emplace(field_str[j], index_str[j]);
            }
            checksum += algorithm.size();
            sw.stop();

            totalTime += sw.getMillisec();
        }

        AlgorithmTy algorithm;
        printf("---------------------------------------------------------------------------\n");
        printf(" %-36s  ", algorithm.name());
        printf("sum = %-10" PRIuPTR "  time: %8.3f ms\n", checksum, totalTime);
    }
}

void run_hashtable_ref_emplace_benchmark()
{
    std::cout << "-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=" << std::endl;
    std::cout << "  hashtable_ref_emplace_benchmark()" << std::endl;
    std::cout << "-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=" << std::endl;
    std::cout << std::endl;

    hashtable_ref_emplace_benchmark_impl<test::std_map<StringRef, StringRef>>();
    hashtable_ref_emplace_benchmark_impl<test::std_unordered_map<StringRef, StringRef>>();

    hashtable_ref_emplace_benchmark_impl<test::hash_table_impl<jstd::hash_table<StringRef, StringRef>>>();
    hashtable_ref_emplace_benchmark_impl<test::hash_table_impl<jstd::hash_table_time31<StringRef, StringRef>>>();
    hashtable_ref_emplace_benchmark_impl<test::hash_table_impl<jstd::hash_table_time31_std<StringRef, StringRef>>>();

#if USE_JSTD_HASH_MAP
    hashtable_ref_emplace_benchmark_impl<test::hash_table_impl<jstd::hash_map<StringRef, StringRef>>>();
    hashtable_ref_emplace_benchmark_impl<test::hash_table_impl<jstd::hash_map_time31<StringRef, StringRef>>>();
    hashtable_ref_emplace_benchmark_impl<test::hash_table_impl<jstd::hash_map_time31_std<StringRef, StringRef>>>();
#endif // USE_JSTD_HASH_MAP

#if USE_JSTD_HASH_MAP_EX
    hashtable_ref_emplace_benchmark_impl<test::hash_table_impl<jstd::hash_map_ex<StringRef, StringRef>>>();
    hashtable_ref_emplace_benchmark_impl<test::hash_table_impl<jstd::hash_map_ex_time31<StringRef, StringRef>>>();
    hashtable_ref_emplace_benchmark_impl<test::hash_table_impl<jstd::hash_map_ex_time31_std<StringRef, StringRef>>>();
#endif // USE_JSTD_HASH_MAP_EX

#if USE_JSTD_DICTIONARY
    hashtable_ref_emplace_benchmark_impl<test::hash_table_impl<jstd::Dictionary<StringRef, StringRef>>>();
    hashtable_ref_emplace_benchmark_impl<test::hash_table_impl<jstd::Dictionary_Time31<StringRef, StringRef>>>();
    hashtable_ref_emplace_benchmark_impl<test::hash_table_impl<jstd::Dictionary_Time31Std<StringRef, StringRef>>>();
#endif // USE_JSTD_DICTIONARY

    printf("---------------------------------------------------------------------------\n");
    printf("\n");
}

template <typename AlgorithmTy>
void hashtable_ref_insert_erase_benchmark_impl()
{
    static const size_t kHeaderFieldSize = sizeof(header_fields) / sizeof(char *);
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
        char buf[16];
#ifdef _MSC_VER
        ::_itoa_s((int)i, buf, 10);
#else
        sprintf(buf, "%d", (int)i);
#endif
        index_buf[i] = buf;
        index_str[i] = index_buf[i];
    }

    {
        size_t checksum = 0;
        AlgorithmTy algorithm;

        StopWatch sw;

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
        printf("sum = %-10" PRIuPTR "  time: %8.3f ms\n", checksum, sw.getMillisec());
    }
}

void run_hashtable_ref_insert_erase_benchmark()
{
    std::cout << "-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=" << std::endl;
    std::cout << "  hashtable_ref_insert_erase_benchmark()" << std::endl;
    std::cout << "-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=" << std::endl;
    std::cout << std::endl;

    hashtable_ref_insert_erase_benchmark_impl<test::std_map<StringRef, StringRef>>();
    hashtable_ref_insert_erase_benchmark_impl<test::std_unordered_map<StringRef, StringRef>>();

    hashtable_ref_insert_erase_benchmark_impl<test::hash_table_impl<jstd::hash_table<StringRef, StringRef>>>();
    hashtable_ref_insert_erase_benchmark_impl<test::hash_table_impl<jstd::hash_table_time31<StringRef, StringRef>>>();
    hashtable_ref_insert_erase_benchmark_impl<test::hash_table_impl<jstd::hash_table_time31_std<StringRef, StringRef>>>();

#if USE_JSTD_HASH_MAP
    hashtable_ref_insert_erase_benchmark_impl<test::hash_table_impl<jstd::hash_map<StringRef, StringRef>>>();
    hashtable_ref_insert_erase_benchmark_impl<test::hash_table_impl<jstd::hash_map_time31<StringRef, StringRef>>>();
    hashtable_ref_insert_erase_benchmark_impl<test::hash_table_impl<jstd::hash_map_time31_std<StringRef, StringRef>>>();
#endif // USE_JSTD_HASH_MAP

#if USE_JSTD_HASH_MAP_EX
    hashtable_ref_insert_erase_benchmark_impl<test::hash_table_impl<jstd::hash_map_ex<StringRef, StringRef>>>();
    hashtable_ref_insert_erase_benchmark_impl<test::hash_table_impl<jstd::hash_map_ex_time31<StringRef, StringRef>>>();
    hashtable_ref_insert_erase_benchmark_impl<test::hash_table_impl<jstd::hash_map_ex_time31_std<StringRef, StringRef>>>();
#endif // USE_JSTD_HASH_MAP_EX

#if USE_JSTD_DICTIONARY
    hashtable_ref_insert_erase_benchmark_impl<test::hash_table_impl<jstd::Dictionary<StringRef, StringRef>>>();
    hashtable_ref_insert_erase_benchmark_impl<test::hash_table_impl<jstd::Dictionary_Time31<StringRef, StringRef>>>();
    hashtable_ref_insert_erase_benchmark_impl<test::hash_table_impl<jstd::Dictionary_Time31Std<StringRef, StringRef>>>();
#endif // USE_JSTD_DICTIONARY

    printf("---------------------------------------------------------------------------\n");
    printf("\n");
}

template <typename AlgorithmTy>
void hashtable_rehash_benchmark_impl()
{
    static const size_t kHeaderFieldSize = sizeof(header_fields) / sizeof(char *);
#ifndef NDEBUG
    static const size_t kRepeatTimes = 2;
#else
    static const size_t kRepeatTimes = (kIterations / kHeaderFieldSize);
#endif

    std::string field_str[kHeaderFieldSize];
    std::string index_str[kHeaderFieldSize];
    for (size_t i = 0; i < kHeaderFieldSize; ++i) {
        field_str[i].assign(header_fields[i]);
        char buf[16];
#ifdef _MSC_VER
        ::_itoa_s((int)i, buf, 10);
#else
        sprintf(buf, "%d", (int)i);
#endif
        index_str[i] = buf;
    }

    {
        size_t checksum = 0;
        size_t buckets = 128;

        AlgorithmTy algorithm(kInitCapacity);
        algorithm.reserve(buckets);

        for (size_t i = 0; i < kHeaderFieldSize; ++i) {
            algorithm.emplace(field_str[i], index_str[i]);
        }

        StopWatch sw;

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
        printf("sum = %-10" PRIuPTR "  time: %8.3f ms\n", checksum, sw.getMillisec());
    }
}

void run_hashtable_rehash_benchmark()
{
    std::cout << "-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=" << std::endl;
    std::cout << "  hashtable_rehash_benchmark()" << std::endl;
    std::cout << "-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=" << std::endl;
    std::cout << std::endl;

    hashtable_rehash_benchmark_impl<test::std_unordered_map<std::string, std::string>>();

    hashtable_rehash_benchmark_impl<test::hash_table_impl<jstd::hash_table<std::string, std::string>>>();
    hashtable_rehash_benchmark_impl<test::hash_table_impl<jstd::hash_table_time31<std::string, std::string>>>();
    hashtable_rehash_benchmark_impl<test::hash_table_impl<jstd::hash_table_time31_std<std::string, std::string>>>();

#if USE_JSTD_HASH_MAP
    hashtable_rehash_benchmark_impl<test::hash_table_impl<jstd::hash_map<std::string, std::string>>>();
    hashtable_rehash_benchmark_impl<test::hash_table_impl<jstd::hash_map_time31<std::string, std::string>>>();
    hashtable_rehash_benchmark_impl<test::hash_table_impl<jstd::hash_map_time31_std<std::string, std::string>>>();
#endif // USE_JSTD_HASH_MAP

#if USE_JSTD_HASH_MAP_EX
    hashtable_rehash_benchmark_impl<test::hash_table_impl<jstd::hash_map_ex<std::string, std::string>>>();
    hashtable_rehash_benchmark_impl<test::hash_table_impl<jstd::hash_map_ex_time31<std::string, std::string>>>();
    hashtable_rehash_benchmark_impl<test::hash_table_impl<jstd::hash_map_ex_time31_std<std::string, std::string>>>();
#endif // USE_JSTD_HASH_MAP_EX

#if USE_JSTD_DICTIONARY
    hashtable_rehash_benchmark_impl<test::hash_table_impl<jstd::Dictionary<std::string, std::string>>>();
    hashtable_rehash_benchmark_impl<test::hash_table_impl<jstd::Dictionary_Time31<std::string, std::string>>>();
    hashtable_rehash_benchmark_impl<test::hash_table_impl<jstd::Dictionary_Time31Std<std::string, std::string>>>();
#endif // USE_JSTD_DICTIONARY

    printf("---------------------------------------------------------------------------\n");
    printf("\n");
}

template <typename AlgorithmTy>
void hashtable_rehash2_benchmark_impl()
{
    static const size_t kHeaderFieldSize = sizeof(header_fields) / sizeof(char *);
#ifndef NDEBUG
    static const size_t kRepeatTimes = 2;
#else
    static const size_t kRepeatTimes = (kIterations / kHeaderFieldSize / 2);
#endif

    std::string field_str[kHeaderFieldSize];
    std::string index_str[kHeaderFieldSize];
    for (size_t i = 0; i < kHeaderFieldSize; ++i) {
        field_str[i].assign(header_fields[i]);
        char buf[16];
#ifdef _MSC_VER
        ::_itoa_s((int)i, buf, 10);
#else
        sprintf(buf, "%d", (int)i);
#endif
        index_str[i] = buf;
    }

    {
        size_t checksum = 0;
        size_t buckets;

        StopWatch sw;

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
        printf("sum = %-10" PRIuPTR "  time: %8.3f ms\n", checksum, sw.getMillisec());
    }
}

void run_hashtable_rehash2_benchmark()
{
    std::cout << "-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=" << std::endl;
    std::cout << "  hashtable_rehash2_benchmark()" << std::endl;
    std::cout << "-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=" << std::endl;
    std::cout << std::endl;

    hashtable_rehash2_benchmark_impl<test::std_unordered_map<std::string, std::string>>();

    hashtable_rehash2_benchmark_impl<test::hash_table_impl<jstd::hash_table<std::string, std::string>>>();
    hashtable_rehash2_benchmark_impl<test::hash_table_impl<jstd::hash_table_time31<std::string, std::string>>>();
    hashtable_rehash2_benchmark_impl<test::hash_table_impl<jstd::hash_table_time31_std<std::string, std::string>>>();

#if USE_JSTD_HASH_MAP
    hashtable_rehash2_benchmark_impl<test::hash_table_impl<jstd::hash_map<std::string, std::string>>>();
    hashtable_rehash2_benchmark_impl<test::hash_table_impl<jstd::hash_map_time31<std::string, std::string>>>();
    hashtable_rehash2_benchmark_impl<test::hash_table_impl<jstd::hash_map_time31_std<std::string, std::string>>>();
#endif // USE_JSTD_HASH_MAP

#if USE_JSTD_HASH_MAP_EX
    hashtable_rehash2_benchmark_impl<test::hash_table_impl<jstd::hash_map_ex<std::string, std::string>>>();
    hashtable_rehash2_benchmark_impl<test::hash_table_impl<jstd::hash_map_ex_time31<std::string, std::string>>>();
    hashtable_rehash2_benchmark_impl<test::hash_table_impl<jstd::hash_map_ex_time31_std<std::string, std::string>>>();
#endif // USE_JSTD_HASH_MAP_EX

#if USE_JSTD_DICTIONARY
    hashtable_rehash2_benchmark_impl<test::hash_table_impl<jstd::Dictionary<std::string, std::string>>>();
    hashtable_rehash2_benchmark_impl<test::hash_table_impl<jstd::Dictionary_Time31<std::string, std::string>>>();
    hashtable_rehash2_benchmark_impl<test::hash_table_impl<jstd::Dictionary_Time31Std<std::string, std::string>>>();
#endif // USE_JSTD_DICTIONARY

    printf("---------------------------------------------------------------------------\n");
    printf("\n");
}

void run_hashtable_benchmark()
{
    run_hashtable_find_benchmark();
    run_hashtable_ref_find_benchmark();

    run_hashtable_insert_benchmark();

    run_hashtable_emplace_benchmark();
    run_hashtable_ref_emplace_benchmark();

    run_hashtable_erase_benchmark();
    run_hashtable_ref_erase_benchmark();

    run_hashtable_insert_erase_benchmark();
    run_hashtable_ref_insert_erase_benchmark();

    run_hashtable_rehash_benchmark();
    run_hashtable_rehash2_benchmark();
}

int main(int argc, char *argv[])
{
    test::cpu_warmup(1000);

    run_iterator_test();

    run_hashtable_benchmark();

#ifdef _WIN32
    ::system("pause");
#endif
    return 0;
}
