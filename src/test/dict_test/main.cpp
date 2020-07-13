
#include <stdlib.h>
#include <stdio.h>

#include <jstd/all.h>

#include <xutility>
#include <utility>
#include <memory>

class ListDemo {
public:
    ListDemo() {}
    ~ListDemo() {}
};

template <class T>
class ListIterator : public jstd::iterator<
        jstd::random_access_iterator_tag, ListIterator<T>
    > {
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
    ListIterator & operator = (const ListIterator<Other> & right)
    {
        ptr_ = right.ptr();
        return (*this);
    }

    // return designated value
    reference operator * () const {
        return (*const_cast<ListIterator *>(this));
    }

    // return pointer to class object
    pointer operator -> () const {
        return (std::pointer_traits<pointer>::pointer_to(**this));
    }

    ListIterator & operator ++ () {
        ++ptr_;
        return (*this);
    }

    ListIterator operator ++ (int) {
        ListIterator tmp = *this;
        ++ptr_;
        return tmp;
    }

    ListIterator & operator -- () {
        --ptr_;
        return (*this);
    }

    ListIterator operator -- (int) {
        ListIterator tmp = *this;
        --ptr_;
        return tmp;
    }

    // increment by integer
    ListIterator & operator += (difference_type offset) {
        ptr_ += offset;
        return (*this);
    }

    ListIterator operator + (difference_type offset) {
        return ListIterator(ptr_ + offset);
    }

    ListIterator & operator -= (difference_type offset) {
        ptr_ -= offset;
        return (*this);
    }

    ListIterator operator - (difference_type offset) {
        return ListIterator(ptr_ - offset);
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

    ListIterator<ListDemo> random_iter;
    jstd::iterator_utils::advance(random_iter, std::ptrdiff_t(1));

    printf("dict_test.exe - %d, 0x%p\n\n", (int)is_iterator, (void *)iter.ptr());
    return 0;
}
