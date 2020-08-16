
#ifndef JSTD_MEMORY_SMART_PTR_H
#define JSTD_MEMORY_SMART_PTR_H

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

#include "jstd/basic/stddef.h"
#include "jstd/basic/stdint.h"
#include "jstd/basic/stdsize.h"

#include <cstdint>
#include <cstddef>
#include <iostream>
#include <memory>
#include <type_traits>

#include "jstd/memory/defines.h"

namespace jstd {

//
// See: http://blog.csdn.net/lollipop_jin/article/details/8499530
//

///////////////////////////////////////////////////
// class smart_ptr<T>
///////////////////////////////////////////////////

template <typename T>
class smart_ptr {
public:
    typedef typename std::remove_all_extents<T>::type
                                    element_type;

    typedef element_type *          pointer;
    typedef const element_type *    const_pointer;
    typedef element_type &          reference;
    typedef const element_type &    const_reference;

    template <typename T>
    class reference_counter {
    private:
        reference_counter(const reference_counter & src);
        reference_counter & operator = (const reference_counter & rhs);

        template <typename Other>
        reference_counter(const reference_counter<Other> & src);

        template <typename Other>
        reference_counter & operator = (const reference_counter<Other> & rhs);

    public:
        typedef T                               element_type;
        typedef element_type *                  value_type;
        typedef element_type *                  pointer;
        typedef const element_type *            const_pointer;
        typedef element_type &                  reference;
        typedef const element_type &            const_reference;
        typedef std::ptrdiff_t                  count_type;

        pointer     ptr;
        count_type  count;

        explicit reference_counter(pointer p = nullptr) : ptr(p), count(1)  {}

        reference_counter(pointer p, count_type cnt) : ptr(p), count(cnt)  {}

        template <typename Other>
        explicit reference_counter(Other * p = nullptr) : ptr(static_cast<pointer>(p)), count(1)  {}

        template <typename Other>
        reference_counter(Other * p, count_type cnt) : ptr(static_cast<pointer>(p)), count(cnt)  {}

        reference_counter(reference_counter && src)
            : ptr(nullptr), count(1) {
            this->swap(src);
        }

        template <typename Other>
        reference_counter(reference_counter<Other> && src)
            : ptr(nullptr), count(1) {
            this->swap(src);
        }

        reference_counter & operator = (reference_counter && rhs) {
            this->swap(src);
            return *this;
        }

        reference_counter & operator = (pointer p) {
            this->ptr = p;
            this->count = 1;
            return *this;
        }

        template <typename Other>
        reference_counter & operator = (Other * p) {
            MUST_BE_A_DERIVED_CLASS_OF(T, Other);
            this->ptr = static_cast<pointer>(p);
            this->count = 1;
            return *this;
        }

        void add_ref() {
            this->count++;
            assert(this->count > 0);
        }

        template <bool is_array>
        bool release() {
            this->count--;
            if (this->count == 0) {
                if (this->ptr) {
                    delete_helper<element_type, is_array>::delete_it(this->ptr);
                    this->ptr = nullptr;
                    return true;
                }
            }
            else if (this->count < 0) {
                // Error: count underflow
                assert(this->count < 0);
            }
            return false;
        }

        void swap(reference_counter & rhs) {
            if (&rhs != this) {
                std::swap(this->ptr, rhs.ptr);
                std::swap(this->count, rhs.count);
            }
        }

        template <typename Other>
        void swap(reference_counter<Other> & rhs) {
            MUST_BE_A_DERIVED_CLASS_OF(T, Other);
            if (&rhs != this) {
                pointer tmp = this->ptr;
                pointer rhs_ptr = static_cast<pointer>(rhs.ptr);
                this->ptr = rhs_ptr;
                rhs.ptr = tmp;
                std::swap(this->count, rhs.count);
            }
        }
    };

    typedef reference_counter<element_type>     counter_type;
    typedef typename counter_type::count_type   count_type;

protected:
    counter_type * counter_;

public:
    smart_ptr(pointer p = nullptr) : counter_((p != nullptr) ? new counter_type(p, 1) : nullptr) {}

    template <typename Other>
    smart_ptr(Other * p = nullptr) : counter_((p != nullptr) ? new counter_type(p, 1) : nullptr) {
        MUST_BE_A_DERIVED_CLASS_OF(T, Other);
    }

    template <size_t N>
    smart_ptr(element_type (&ptrs)[N]) : counter_((ptrs != nullptr) ? new counter_type(ptrs, 1) : nullptr) {}

    template <typename Other, size_t N>
    smart_ptr(Other (&ptrs)[N]) : counter_((ptrs != nullptr) ? new counter_type(ptrs, 1) : nullptr) {
        MUST_BE_A_DERIVED_CLASS_OF(T, Other);
    }

    smart_ptr(const smart_ptr & src) : counter_(nullptr) {
        this->counter_ = src.counter_;
        this->add_ref();
    }

    template <typename Other>
    smart_ptr(const smart_ptr<Other> & src) : counter_(nullptr) {
        MUST_BE_A_DERIVED_CLASS_OF(T, Other);
        this->counter_ = static_cast<counter_type *>(src.counter_);
        this->add_ref();
    }

    smart_ptr(smart_ptr && src) : counter_(nullptr) {
        this->swap(src);
    }

    template <typename Other>
    smart_ptr(smart_ptr<Other> && src) : counter_(nullptr) {
        this->swap(src);
    }

    ~smart_ptr() {
        this->destroy();
    }

    void destroy() {
        if (this->counter_) {
            bool has_released = this->internal_release();
            if (!has_released) {
                delete this->counter_;
                this->counter_ = nullptr;
            }
        }
    }

    void reset(pointer p = nullptr) {
        this->assign(p);
    }

    template <typename Other>
    void reset(Other * p = nullptr) {
        MUST_BE_A_DERIVED_CLASS_OF(T, Other);
        pointer * ptr = static_cast<pointer>(p);
        this->assign(ptr);
    }

    void add_ref() {
        if (this->counter_) {
            this->internal_add_ref();
        }
    }

    bool release() {
        if (this->counter_) {
            return this->internal_release();
        }
        return false;
    }

protected:
    void assign(pointer p = nullptr) {
        MUST_BE_A_DERIVED_CLASS_OF(T, Other);
        if (p != nullptr) {
            // Setting the new data pointer.
            if (this->counter_ != nullptr) {
                if (p != this->counter_->ptr) {
                    this->internal_release();
                    this->counter_ = new counter_type(p);
                }
                else {
                    // If p isn't null and p is same to the data pointer,
                    // needn't change the counter.
                }
            }
            else {
                this->counter_ = new counter_type(p);
            }
        }
        else {
            // The p is null, need destroy old counter.
            this->destroy();
        }
    }

    void internal_add_ref() {
        assert(this->counter_ != nullptr);
        this->counter_->add_ref();
        assert(this->counter_->count > 0);
    }

    template <bool is_array = std::is_array<T>::value>
    bool internal_release() {
        assert(this->counter_ != nullptr);
        bool has_released = this->counter_->release<is_array>();
        if (has_released) {
            delete this->counter_;
            this->counter_ = nullptr;
        }
        return has_released;
    }

public:
    smart_ptr & operator = (const smart_ptr & rhs) {
        this->copy_from(rhs);
        return *this;
    }

    template <typename Other>
    smart_ptr & operator = (const smart_ptr<Other> & rhs) {
        this->copy_from(rhs);
        return *this;
    }

    smart_ptr & operator = (smart_ptr && rhs) {
        this->swap(rhs);
        return *this;
    }

    template <typename Other>
    smart_ptr & operator = (smart_ptr<Other> && rhs) {
        this->swap(rhs);
        return *this;
    }

    smart_ptr & operator = (pointer p) {
        this->assign(p);
        return *this;
    }

    template <typename Other>
    smart_ptr & operator = (Other * p) {
        MUST_BE_A_DERIVED_CLASS_OF(T, Other);
        pointer ptr = static_cast<pointer>(p);
        this->assign(ptr);
        return *this;
    }

    reference operator *() const {
        assert(this->counter_ != nullptr);
        return *(this->counter_->ptr);
    }

    pointer operator->() const {
        assert(this->counter_ != nullptr);
        return this->counter_->ptr;
    }

    bool is_valid() const {
        return (this->counter_ != nullptr);
    }

    pointer get() const {
        return (this->is_valid() ? this->counter_->ptr : nullptr);
    }
    pointer unsafe_get() const {
        assert(this->counter_ != nullptr);
        return this->counter_->ptr;
    }

    pointer get_pointer() const {
        return (this->is_valid() ? this->counter_->ptr : nullptr);
    }

    count_type get_count() const {
        return (this->is_valid() ? this->counter_->count : 0);
    }

    counter_type * get_counter() const {
        return this->counter_;
    }
    void set_counter(counter_type * counter) {
        this->counter_ = counter;
    }

    bool not_nullptr() const {
        return (this->is_valid() && (this->counter_->ptr != nullptr));
    }
    bool is_nullptr() const {
        return !(this->not_nullptr());
    }
    bool is_equal(smart_ptr const & rhs) const {
        assert((this->counter_ == rhs->counter_) ||
               (this->counter_ != nullptr && rhs->counter_ != nullptr &&
                this->counter_->ptr == rhs->counter_->ptr &&
                this->counter_->count == rhs->counter_->count));
        return ((this->counter_ == rhs->counter_) ||
                (this->counter_ != nullptr && rhs->counter_ != nullptr &&
                 this->counter_->ptr == rhs->counter_->ptr));
    }

    // operator []
    pointer operator [] (int index) const {
        assert(this->counter_ != nullptr);
        return &(this->counter_->ptr[index]);
    }

    // operator bool
    inline explicit operator bool() const {
        return this->not_nullptr();
    }

    // operator ==
    inline bool operator == (bool rhs) {
        return (this->not_nullptr() == rhs);
    }
    inline bool operator == (void * rhs) {
        return (this->get_pointer() == static_cast<pointer>(rhs));
    }
    inline bool operator == (std::nullptr_t rhs) {
        return (this->get_pointer() == static_cast<pointer>(rhs));
    }
    template <typename U>
    inline bool operator == (U * rhs) {
        return (this->get_pointer() == static_cast<pointer>(rhs));
    }
    inline bool operator == (smart_ptr const & rhs) {
        return this->is_equal(rhs);
    }

    // operator !=
    inline bool operator != (bool rhs) {
        return (this->is_valid() != rhs);
    }
    inline bool operator != (void * rhs) {
        return (this->get_pointer() != static_cast<pointer>(rhs));
    }
    inline bool operator != (std::nullptr_t rhs) {
        return (this->get_pointer() != static_cast<pointer>(rhs));
    }
    template <typename U>
    inline bool operator != (U * rhs) {
        return (this->get_pointer() != static_cast<pointer>(rhs));
    }
    inline bool operator != (smart_ptr const & rhs) {
        return !this->is_equal(rhs);
    }

    // operator >
    inline bool operator > (void * rhs) {
        return (static_cast<void *>(this->get_pointer()) > rhs);
    }
    inline bool operator > (std::nullptr_t rhs) {
        return (static_cast<void *>(this->get_pointer()) > static_cast<void *>(rhs));
    }
    template <typename Other>
    inline bool operator > (Other * rhs) {
        return (static_cast<void *>(this->get_pointer()) > static_cast<void *>(rhs));
    }
    inline bool operator > (smart_ptr const & rhs) {
        return (static_cast<void *>(this->get_pointer()) >
                static_cast<void *>(  rhs.get_pointer()));
    }

    // operator >=
    inline bool operator >= (void * rhs) {
        return (static_cast<void *>(this->get_pointer()) >= rhs);
    }
    inline bool operator >= (std::nullptr_t rhs) {
        return (static_cast<void *>(this->get_pointer()) >= static_cast<void *>(rhs));
    }
    template <typename Other>
    inline bool operator >= (Other * rhs) {
        return (static_cast<void *>(this->get_pointer()) >= static_cast<void *>(rhs));
    }
    inline bool operator >= (smart_ptr const & rhs) {
        return (static_cast<void *>(this->get_pointer()) >=
                static_cast<void *>(  rhs.get_pointer()));
    }

    // operator <
    inline bool operator < (void * rhs) {
        return (static_cast<void *>(this->get_pointer()) < rhs);
    }
    inline bool operator < (std::nullptr_t rhs) {
        return (static_cast<void *>(this->get_pointer()) < static_cast<void *>(rhs));
    }
    template <typename Other>
    inline bool operator < (Other * rhs) {
        return (static_cast<void *>(this->get_pointer()) < static_cast<void *>(rhs));
    }
    inline bool operator < (smart_ptr const & rhs) {
        return (static_cast<void *>(this->get_pointer()) <
                static_cast<void *>(  rhs.get_pointer()));
    }

    // operator <=
    inline bool operator <= (void * rhs) {
        return (static_cast<void *>(this->get_pointer()) <= rhs);
    }
    inline bool operator <= (std::nullptr_t rhs) {
        return (static_cast<void *>(this->get_pointer()) <= static_cast<void *>(rhs));
    }
    template <typename Other>
    inline bool operator <= (Other * rhs) {
        return (static_cast<void *>(this->get_pointer()) <= static_cast<void *>(rhs));
    }
    inline bool operator <= (smart_ptr const & rhs) {
        return (static_cast<void *>(this->get_pointer()) <=
                static_cast<void *>(  rhs.get_pointer()));
    }

    void copy_from(const smart_ptr & src) {
        if (&src != this) {
            if (this->counter_ != src.counter_) {
                this->release();
                this->counter_ = src.counter_;
                this->add_ref();
            }
        }
    }

    template <typename Other>
    void copy_from(const smart_ptr<Other> & rhs) {
        MUST_BE_A_DERIVED_CLASS_OF(T, Other);
        counter_type * rhs_counter = static_cast<counter_type *>(rhs.counter_);
        if (&src != this) {
            if (this->counter_ != rhs_counter) {
                this->release();
                this->counter_ = rhs_counter;
                this->add_ref();
            }
        }
    }

    void swap(smart_ptr & rhs) {
        std::swap(this->counter_, rhs.counter_);
    }

    template <typename Other>
    void swap(smart_ptr<Other> & rhs) {
        MUST_BE_A_DERIVED_CLASS_OF(T, Other);
        counter_type * rhs_counter = static_cast<counter_type *>(rhs.counter_);
        std::swap(this->counter_, rhs.counter_);
    }
};  // class smart_ptr<T>

template <typename T>
inline
void swap(smart_ptr<T> & lhs, smart_ptr<T> & rhs)
{
    lhs.swap(rhs);
}

//
// global operator smart_ptr<T> == **
//
template <typename T> 
inline bool operator == (smart_ptr<T> const & lhs, bool rhs)
{
    return (lhs.not_nullptr() == rhs);
}

template <typename T> 
inline bool operator == (smart_ptr<T> const & lhs, void * rhs)
{
    return (lhs.get_pointer() == static_cast<typename smart_ptr<T>::pointer>(rhs));
}

template <typename T> 
inline bool operator == (smart_ptr<T> const & lhs, std::nullptr_t rhs)
{
    return (lhs.get_pointer() == static_cast<typename smart_ptr<T>::pointer>(rhs));
}

template <typename T, typename U> 
inline bool operator == (smart_ptr<T> const & lhs, U * rhs)
{
    return (lhs.get_pointer() == static_cast<typename smart_ptr<T>::pointer>(rhs));
}

template <typename T> 
inline bool operator == (smart_ptr<T> const & lhs, smart_ptr<T> const & rhs)
{
    return lhs.is_equal(rhs);
}

//
// global operator ** == smart_ptr<T>
//
template <typename T> 
inline bool operator == (bool lhs, smart_ptr<T> const & rhs)
{
    return (rhs.not_nullptr() == lhs);
}

template <typename T> 
inline bool operator == (void * lhs, smart_ptr<T> const & rhs)
{
    return (rhs.get_pointer() == static_cast<typename smart_ptr<T>::pointer>(lhs));
}

template <typename T> 
inline bool operator == (std::nullptr_t lhs, smart_ptr<T> const & rhs)
{
    return (rhs.get_pointer() == static_cast<typename smart_ptr<T>::pointer>(lhs));
}

template <typename T, typename U> 
inline bool operator == (U * lhs, smart_ptr<T> const & rhs)
{
    return (rhs.get_pointer() == static_cast<typename smart_ptr<T>::pointer>(lhs));
}

//
// global operator smart_ptr<T> != **
//
template <typename T> 
inline bool operator != (smart_ptr<T> const & lhs, bool rhs)
{
    return (lhs.not_nullptr() != rhs);
}

template <typename T> 
inline bool operator != (smart_ptr<T> const & lhs, void * rhs)
{
    return (lhs.get_pointer() != static_cast<typename smart_ptr<T>::pointer>(rhs));
}

template <typename T> 
inline bool operator != (smart_ptr<T> const & lhs, std::nullptr_t rhs)
{
    return (lhs.get_pointer() != static_cast<typename smart_ptr<T>::pointer>(rhs));
}

template <typename T, typename U> 
inline bool operator != (smart_ptr<T> const & lhs, U * rhs)
{
    return (lhs.get_pointer() != static_cast<typename smart_ptr<T>::pointer>(rhs));
}

template <typename T> 
inline bool operator != (smart_ptr<T> const & rhs, smart_ptr<T> const & lhs)
{
    return rhs.is_equal(rhs);
}

//
// global operator ** != smart_ptr<T>
//
template <typename T> 
inline bool operator != (bool lhs, smart_ptr<T> const & rhs)
{
    return (rhs.not_nullptr() != lhs);
}

template <typename T> 
inline bool operator != (void * lhs, smart_ptr<T> const & rhs)
{
    return (rhs.get_pointer() != static_cast<typename smart_ptr<T>::pointer>(lhs));
}

template <typename T> 
inline bool operator != (std::nullptr_t lhs, smart_ptr<T> const & rhs)
{
    return (rhs.get_pointer() != static_cast<typename smart_ptr<T>::pointer>(lhs));
}

template <typename T, typename U> 
inline bool operator != (U * lhs, smart_ptr<T> const & rhs)
{
    return (rhs.get_pointer() != static_cast<typename smart_ptr<T>::pointer>(lhs));
}

//
// global operator ** > smart_ptr<T>
//
template <typename T> 
inline bool operator > (void * lhs, smart_ptr<T> const & rhs)
{
    return (lhs > static_cast<void *>(rhs.get_pointer()));
}

template <typename T> 
inline bool operator > (std::nullptr_t lhs, smart_ptr<T> const & rhs)
{
    return (static_cast<void *>(lhs) > static_cast<void *>(rhs.get_pointer()));
}

template <typename T, typename U> 
inline bool operator > (U * lhs, smart_ptr<T> const & rhs)
{
    return (static_cast<void *>(lhs) > static_cast<void *>(rhs.get_pointer()));
}

template <typename T> 
inline bool operator > (smart_ptr<T> const & lhs, smart_ptr<T> const & rhs)
{
    return (static_cast<void *>(lhs.get_pointer()) >
            static_cast<void *>(rhs.get_pointer()));
}

//
// global operator ** >= smart_ptr<T>
//
template <typename T> 
inline bool operator >= (void * lhs, smart_ptr<T> const & rhs)
{
    return (lhs >= static_cast<void *>(rhs.get_pointer()));
}

template <typename T> 
inline bool operator >= (std::nullptr_t lhs, smart_ptr<T> const & rhs)
{
    return (static_cast<void *>(lhs) >= static_cast<void *>(rhs.get_pointer()));
}

template <typename T, typename U> 
inline bool operator >= (U * lhs, smart_ptr<T> const & rhs)
{
    return (static_cast<void *>(lhs) >= static_cast<void *>(rhs.get_pointer()));
}

template <typename T> 
inline bool operator >= (smart_ptr<T> const & lhs, smart_ptr<T> const & rhs)
{
    return (static_cast<void *>(lhs.get_pointer()) >=
            static_cast<void *>(rhs.get_pointer()));
}

//
// global operator ** < smart_ptr<T>
//
template <typename T> 
inline bool operator < (void * lhs, smart_ptr<T> const & rhs)
{
    return (lhs < static_cast<void *>(rhs.get_pointer()));
}

template <typename T> 
inline bool operator < (std::nullptr_t lhs, smart_ptr<T> const & rhs)
{
    return (static_cast<void *>(lhs) < static_cast<void *>(rhs.get_pointer()));
}

template <typename T, typename U> 
inline bool operator < (U * lhs, smart_ptr<T> const & rhs)
{
    return (static_cast<void *>(lhs) < static_cast<void *>(rhs.get_pointer()));
}

template <typename T> 
inline bool operator < (smart_ptr<T> const & lhs, smart_ptr<T> const & rhs)
{
    return (static_cast<void *>(lhs.get_pointer()) <
            static_cast<void *>(rhs.get_pointer()));
}

//
// global operator ** <= smart_ptr<T>
//
template <typename T> 
inline bool operator <= (void * lhs, smart_ptr<T> const & rhs)
{
    return (lhs <= static_cast<void *>(rhs.get_pointer()));
}

template <typename T> 
inline bool operator <= (std::nullptr_t lhs, smart_ptr<T> const & rhs)
{
    return (static_cast<void *>(lhs) <= static_cast<void *>(rhs.get_pointer()));
}

template <typename T, typename U> 
inline bool operator <= (U * lhs, smart_ptr<T> const & rhs)
{
    return (static_cast<void *>(lhs) <= static_cast<void *>(rhs.get_pointer()));
}

template <typename T> 
inline bool operator <= (smart_ptr<T> const & lhs, smart_ptr<T> const & rhs)
{
    return (static_cast<void *>(lhs.get_pointer()) <=
            static_cast<void *>(rhs.get_pointer()));
}

} // namespace jstd

namespace std {

template <typename T>
inline
void swap(jstd::smart_ptr<T> & lhs, jstd::smart_ptr<T> & rhs)
{
    lhs.swap(rhs);
}

} // namespace std

#endif // JSTD_MEMORY_SMART_PTR_H
