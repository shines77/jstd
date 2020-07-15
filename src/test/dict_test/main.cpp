
#ifndef __SSE4_2__
#define __SSE4_2__              1
#endif // __SSE4_2__

#include <stdlib.h>
#include <stdio.h>

#if __SSE4_2__

// Support SSE 4.2: _mm_crc32_u32(), _mm_crc32_u64().
#define SUPPORT_SSE42_CRC32C    1

// Support Intel SMID SHA module: sha1 & sha256, it's higher than SSE 4.2 .
// _mm_sha1msg1_epu32(), _mm_sha1msg2_epu32() and so on.
#define SUPPORT_SMID_SHA        0

#endif // __SSE4_2__

// String compare mode
#define STRING_COMPARE_LIBC     0
#define STRING_COMPARE_U64      1
#define STRING_COMPARE_SSE42    2

#define STRING_COMPARE_MODE     STRING_COMPARE_SSE42

#include <jstd/all.h>

#include <memory>
#include <utility>

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

int main(int argc, char *argv[])
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

    return 0;
}
