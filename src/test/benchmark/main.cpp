
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
#define STRING_UTILS_LIBC       0
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
#include <jstd/string/string_view_array.h>
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
#include <vector>
#include <map>
#include <unordered_map>
#include <atomic>
#include <thread>
#include <ratio>
#include <chrono>
#include <memory>
#include <utility>

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

//
// An individual benchmark result.
//
struct Result {
    typedef std::size_t size_type;

    size_type   catId;
    std::string name;
    double      elaspedTime1;
    size_type   checksum1;
    double      elaspedTime2;
    size_type   checksum2;

    Result(size_type catId, const std::string & name,
           double elaspedTime1, size_type checksum1,
           double elaspedTime2, size_type checksum2)
        : catId(catId), name(name),
          elaspedTime1(elaspedTime1), checksum1(checksum1),
          elaspedTime2(elaspedTime2), checksum2(checksum2) {
    }
    Result(const Result & src)
        : catId(src.catId), name(src.name),
          elaspedTime1(src.elaspedTime1), checksum1(src.checksum1),
          elaspedTime2(src.elaspedTime2), checksum2(src.checksum2) {
    }
    Result(Result && rhs)
        : catId(size_type(-1)),
          elaspedTime1(0.0), checksum1(0),
          elaspedTime2(0.0), checksum2(0) {
        swap(rhs);
    }

    void swap(Result & rhs) {
        if (&rhs != this) {
            std::swap(this->catId,          rhs.catId);
            std::swap(this->name,           rhs.name);
            std::swap(this->elaspedTime1,   rhs.elaspedTime1);
            std::swap(this->checksum1,      rhs.checksum1);
            std::swap(this->elaspedTime2,   rhs.elaspedTime2);
            std::swap(this->checksum2,      rhs.checksum2);
        }
    }
};

class BenchmarkCategory {
public:
    typedef std::size_t size_type;

private:
    size_type           catId;
    std::string         name_;
    std::vector<Result> result_list_;

public:
    BenchmarkCategory(const std::string & name) : catId(size_type(-1)), name_(name) {}
    ~BenchmarkCategory() {}

    size_type size() const { return result_list_.size(); }

    std::string & name() {
        return this->name_;
    }

    const std::string & name() const {
        return this->name_;
    }

    void setName(const std::string & name) {
        this->name_ = name;
    }

    Result & getResult(size_type index) {
        return result_list_[index];
    }

    const Result & getResult(size_type index) const {
        return result_list_[index];
    }

    void addResult(size_type catId, const std::string & name,
                   double time1, size_type checksum1,
                   double time2, size_type checksum2) {
        Result result(catId, name, time1, checksum1, time2, checksum2);
        result_list_.push_back(std::move(result));
    }
};

class BenchmarkResult {
public:
    typedef std::size_t size_type;

private:
    std::string name1_;
    std::string name2_;
    std::vector<BenchmarkCategory *> category_list_;

    void destroy() {
        for (size_type i = 0; i < category_list_.size(); i++) {
            BenchmarkCategory * category = category_list_[i];
            if (category != nullptr) {
                delete category;
                category_list_[i] = nullptr;
            }
        }
        category_list_.clear();
    }

public:
    BenchmarkResult() {}
    ~BenchmarkResult() {
        destroy();
    }

    size_type category_size() const {
        return category_list_.size();
    }

    std::string & getName1() {
        return this->name1_;
    }

    std::string & getName2() {
        return this->name2_;
    }

    const std::string & getName1() const {
        return this->name1_;
    }

    const std::string & getName2() const {
        return this->name2_;
    }

    void setName(const std::string & name1, const std::string & name2) {
        this->name1_ = name1;
        this->name2_ = name2;
    }

    BenchmarkCategory * getCategory(size_type index) {
        if (index < category_list_.size())
            return category_list_[index];
        else
            return nullptr;
    }

    size_type addCategory(const std::string & name) {
        BenchmarkCategory * category = new BenchmarkCategory(name);
        category_list_.push_back(category);
        return (category_list_.size() - 1);
    }

    bool addResult(size_type catId, const std::string & name,
                   double elaspedTime1, size_type checksum1,
                   double elaspedTime2, size_type checksum2) {
        BenchmarkCategory * category = getCategory(catId);
        if (category != nullptr) {
            category->addResult(catId, name, elaspedTime1, checksum1, elaspedTime2, checksum2);
            return true;
        }
        return false;
    }

    /*******************************************************************************************************
       Test                                          std::unordered_map         jstd::Dictionary     Ratio
      ------------------------------------------------------------------------------------------------------
       hash_map<std::string, std::string>          checksum     time         checksum    time

       hash_map<K, V>/find                    | 98765432109   100.00 ms | 98765432109   30.00 ms |   3.33
       hash_map<K, V>/insert                  | 98765432109   100.00 ms | 98765432109   30.00 ms |   3.33
       hash_map<K, V>/emplace                 | 98765432109   100.00 ms | 98765432109   30.00 ms |   3.33
       hash_map<K, V>/erase                   | 98765432109   100.00 ms | 98765432109   30.00 ms |   3.33
      ------------------------------------------------------------------------------------------------------
    *******************************************************************************************************/
    void printResult() {
        printf(" Test                                    %23s  %23s     Ratio\n",
               this->name1_.c_str(), this->name2_.c_str());
        printf("------------------------------------------------------------------------------------------------------\n");

        for (size_type catId = 0; catId < category_size(); catId++) {
            BenchmarkCategory * category = getCategory(catId);
            if (category != nullptr) {
                if (category->name().size() <= 40)
                    printf(" %-40s    checksum    time         checksum    time\n", category->name().c_str());
                else
                    printf(" %-52s"          "    time         checksum    time\n", category->name().c_str());
                printf("\n");

                size_type result_count = category->size();
                for (size_type i = 0; i < result_count; i++) {
                    const Result & result = category->getResult(i);
                    double ratio;
                    if (result.elaspedTime2 != 0.0)
                        ratio = result.elaspedTime1 / result.elaspedTime2;
                    else
                        ratio = 0.0;
                    printf(" %-38s | %11" PRIuPTR " %7.2f ms | %11" PRIuPTR " %7.2f ms |   %0.2f\n",
                           result.name.c_str(),
                           result.checksum1, result.elaspedTime1,
                           result.checksum2, result.elaspedTime2,
                           ratio);
                }

                if (catId < (category_size() - 1))
                    printf("\n");
            }
        }

        printf("\n");
        printf("------------------------------------------------------------------------------------------------------\n");
        printf("\n");
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

template <typename Container, typename Vector>
void test_hashmap_find(const Vector & test_data,
                       double & elapsedTime, std::size_t & check_sum)
{
    typedef typename Container::const_iterator const_iterator;

    std::size_t data_length = test_data.size();
    std::size_t repeat_times;
    if (data_length != 0)
        repeat_times = (kIterations / data_length);
    else
        repeat_times = 0;
    std::size_t checksum = 0;

    Container container(kInitCapacity);
    for (std::size_t i = 0; i < data_length; i++) {
        container.emplace(test_data[i].first, test_data[i].second);
    }

    jtest::StopWatch sw;
    sw.start();
    for (std::size_t n = 0; n < repeat_times; n++) {
        for (std::size_t i = 0; i < data_length; i++) {
            const_iterator iter = container.find(test_data[i].first);
            if (iter != container.end()) {
                checksum++;
            }
//#ifndef NDEBUG
            else {
                static int err_count = 0;
                err_count++;
                if (err_count < 20) {
                    print_error(i, test_data[i].first, test_data[i].second);
                }
            }
//#endif
        }
    }
    sw.stop();

    elapsedTime = sw.getElapsedMillisec();
    check_sum = checksum;

    printf("---------------------------------------------------------------------------\n");
    if (jstd::has_name<Container>::value)
        printf(" %-36s  ", jstd::call_name<Container>::name().c_str());
    else
        printf(" %-36s  ", "std::unordered_map<K, V>");
    printf("sum = %-10" PRIuPTR "  time: %8.3f ms\n", checksum, elapsedTime);
}

template <typename Container, typename Vector>
void test_hashmap_insert(const Vector & test_data,
                         double & elapsedTime, std::size_t & check_sum)
{
    std::size_t data_length = test_data.size();
    std::size_t repeat_times;
    if (data_length != 0)
        repeat_times = (kIterations / data_length);
    else
        repeat_times = 0;

    std::size_t checksum = 0;
    double totalTime = 0.0;
    jtest::StopWatch sw;
        
    for (std::size_t n = 0; n < repeat_times; n++) {
        Container container(kInitCapacity);
        sw.start();
        for (std::size_t i = 0; i < data_length; i++) {
            container.insert(std::make_pair(test_data[i].first, test_data[i].second));
        }
        sw.stop();

        checksum += container.size();
        totalTime += sw.getElapsedMillisec();
    }

    elapsedTime = totalTime;
    check_sum = checksum;

    printf("---------------------------------------------------------------------------\n");
    if (jstd::has_name<Container>::value)
        printf(" %-36s  ", jstd::call_name<Container>::name().c_str());
    else
        printf(" %-36s  ", "std::unordered_map<K, V>");
    printf("sum = %-10" PRIuPTR "  time: %8.3f ms\n", checksum, totalTime);
}

template <typename Container, typename Vector>
void test_hashmap_emplace(const Vector & test_data,
                          double & elapsedTime, std::size_t & check_sum)
{
    std::size_t data_length = test_data.size();
    std::size_t repeat_times;
    if (data_length != 0)
        repeat_times = (kIterations / data_length);
    else
        repeat_times = 0;

    std::size_t checksum = 0;
    double totalTime = 0.0;
    jtest::StopWatch sw;
        
    for (std::size_t n = 0; n < repeat_times; n++) {
        Container container(kInitCapacity);
        sw.start();
        for (std::size_t i = 0; i < data_length; i++) {
            container.emplace(test_data[i].first, test_data[i].second);
        }
        sw.stop();

        checksum += container.size();
        totalTime += sw.getElapsedMillisec();
    }

    elapsedTime = totalTime;
    check_sum = checksum;

    printf("---------------------------------------------------------------------------\n");
    if (jstd::has_name<Container>::value)
        printf(" %-36s  ", jstd::call_name<Container>::name().c_str());
    else
        printf(" %-36s  ", "std::unordered_map<K, V>");
    printf("sum = %-10" PRIuPTR "  time: %8.3f ms\n", checksum, totalTime);
}

template <typename Container, typename Vector>
void test_hashmap_erase(const Vector & test_data,
                        double & elapsedTime, std::size_t & check_sum)
{
    std::size_t data_length = test_data.size();
    std::size_t repeat_times;
    if (data_length != 0)
        repeat_times = (kIterations / data_length);
    else
        repeat_times = 0;

    std::size_t checksum = 0;
    double totalTime = 0.0;
    jtest::StopWatch sw;
        
    for (std::size_t n = 0; n < repeat_times; n++) {
        Container container(kInitCapacity);
        for (std::size_t i = 0; i < data_length; i++) {
            container.emplace(test_data[i].first, test_data[i].second);
        }
        checksum += container.size();

        sw.start();
        for (std::size_t i = 0; i < data_length; i++) {
            container.erase(test_data[i].first);
        }
        sw.stop();

        assert(container.size() == 0);
//#ifndef NDEBUG
        if (container.size() != 0) {
            static int err_count = 0;
            err_count++;
            if (err_count < 20) {
                printf("container.size() = %" PRIuPTR "\n", container.size());
            }
        }
//#endif
        checksum += container.size();
        totalTime += sw.getElapsedMillisec();
    }

    elapsedTime = totalTime;
    check_sum = checksum;

    printf("---------------------------------------------------------------------------\n");
    if (jstd::has_name<Container>::value)
        printf(" %-36s  ", jstd::call_name<Container>::name().c_str());
    else
        printf(" %-36s  ", "std::unordered_map<K, V>");
    printf("sum = %-10" PRIuPTR "  time: %8.3f ms\n", checksum, totalTime);
}

template <typename Container1, typename Container2, typename Vector>
void hashmap_benchmark_single(const std::string & cat_name,
                              Container1 & container1, Container2 & container2,
                              const Vector & test_data,
                              BenchmarkResult & result)
{
    std::size_t cat_id = result.addCategory(cat_name);

    double elapsedTime1, elapsedTime2;
    std::size_t checksum1, checksum2;

    //
    // test hashmap<K, V>/find
    //
    test_hashmap_find<Container1, Vector>(test_data, elapsedTime1, checksum1);
    test_hashmap_find<Container2, Vector>(test_data, elapsedTime2, checksum2);

    result.addResult(cat_id, "hash_map<K, V>/find", elapsedTime1, checksum1, elapsedTime2, checksum2);

    //
    // test hashmap<K, V>/insert
    //
    test_hashmap_insert<Container1, Vector>(test_data, elapsedTime1, checksum1);
    test_hashmap_insert<Container2, Vector>(test_data, elapsedTime2, checksum2);

    result.addResult(cat_id, "hash_map<K, V>/insert", elapsedTime1, checksum1, elapsedTime2, checksum2);

    //
    // test hashmap<K, V>/emplace
    //
    test_hashmap_emplace<Container1, Vector>(test_data, elapsedTime1, checksum1);
    test_hashmap_emplace<Container2, Vector>(test_data, elapsedTime2, checksum2);

    result.addResult(cat_id, "hash_map<K, V>/emplace", elapsedTime1, checksum1, elapsedTime2, checksum2);

    //
    // test hashmap<K, V>/erase
    //
    test_hashmap_erase<Container1, Vector>(test_data, elapsedTime1, checksum1);
    test_hashmap_erase<Container2, Vector>(test_data, elapsedTime2, checksum2);

    result.addResult(cat_id, "hash_map<K, V>/erase", elapsedTime1, checksum1, elapsedTime2, checksum2);
}

void hashmap_benchmark_all()
{
    BenchmarkResult test_result;
    test_result.setName("std::unordered_map", "jstd::Dictionary");

    //
    // std::unordered_map<std::string, std::string>
    //
    std::vector<std::pair<std::string, std::string>> test_data_ss;

    if (!dict_words_is_ready) {
        std::string field_str[kHeaderFieldSize];
        std::string index_str[kHeaderFieldSize];
        for (std::size_t i = 0; i < kHeaderFieldSize; i++) {
            test_data_ss.push_back(std::make_pair(std::string(header_fields[i]), std::to_string(i)));
        }
    }
    else {
        for (std::size_t i = 0; i < dict_words.size(); i++) {
            test_data_ss.push_back(std::make_pair(dict_words[i], std::to_string(i)));
        }
    }

    std::unordered_map<std::string, std::string> std_map_ss;
    jstd::Dictionary<std::string, std::string>   jstd_dict_ss;

    hashmap_benchmark_single("hash_map<std::string, std::string>",
                                std_map_ss, jstd_dict_ss,
                                test_data_ss, test_result);

    printf("\n\n");

    //
    // Note: Don't remove these braces '{' and '}', 
    //       otherwise may cause some unpredictable bugs.
    //
    {
        {
            //
            // std::unordered_map<jstd::string_view, jstd::string_view>
            //
            typedef string_view_array<jstd::string_view, jstd::string_view> string_view_array_t;
            typedef typename string_view_array_t::element_type              element_type;

            string_view_array_t test_data_svsv;

            for (std::size_t i = 0; i < test_data_ss.size(); i++) {
                test_data_svsv.push_back(new element_type(test_data_ss[i].first, test_data_ss[i].second));
            }

            std::unordered_map<jstd::string_view, jstd::string_view> std_map_svsv;
            jstd::Dictionary<jstd::string_view, jstd::string_view>   jstd_dict_svsv;

            hashmap_benchmark_single("hash_map<jstd::string_view, jstd::string_view>",
                                        std_map_svsv, jstd_dict_svsv,
                                        test_data_svsv, test_result);

            printf("\n\n");
        }

        {
            //
            // std::unordered_map<int, int>
            //
            std::vector<std::pair<int, int>> test_data_ii;

            for (std::size_t i = 0; i < test_data_ss.size(); i++) {
                test_data_ii.push_back(std::make_pair(int(i), int(i)));
            }

            std::unordered_map<int, int> std_map_ii;
            jstd::Dictionary<int, int>   jstd_dict_ii;

            hashmap_benchmark_single("hash_map<int, int>",
                                        std_map_ii, jstd_dict_ii,
                                        test_data_ii, test_result);

            printf("\n\n");
        }

        {
            //
            // std::unordered_map<size_t, size_t>
            //
            std::vector<std::pair<std::size_t, std::size_t>> test_data_uu;

            for (std::size_t i = 0; i < test_data_ss.size(); i++) {
                test_data_uu.push_back(std::make_pair(i, i));
            }

            std::unordered_map<std::size_t, std::size_t> std_map_uu;
            jstd::Dictionary<std::size_t, std::size_t>   jstd_dict_uu;

            hashmap_benchmark_single("hash_map<std::size_t, std::size_t>",
                                        std_map_uu, jstd_dict_uu,
                                        test_data_uu, test_result);

            printf("\n\n");
        }
    }

    test_result.printResult();
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

    jtest::CPU::warmup(1000);

    hashmap_benchmark_all();

    jstd::Console::ReadKey();
    return 0;
}
