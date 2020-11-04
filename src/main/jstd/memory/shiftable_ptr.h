
#ifndef JSTD_MEMORY_SHIFTABLE_PTR_H
#define JSTD_MEMORY_SHIFTABLE_PTR_H

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

#include "jstd/basic/stddef.h"
#include "jstd/basic/stdint.h"

#include <stdint.h>
#include <stddef.h>
#include <malloc.h>
#include <memory.h>
#include <assert.h>

#include <cstddef>
#include <cstdint>
#include <memory>
#include <type_traits>
#include <functional>

#include "jstd/memory/defines.h"
#include "jstd/allocator.h"
#include "jstd/type_traits.h"

namespace jstd {

template <typename T>
class shiftable_ptr {
public:
    typedef T                               element_type;
    typedef element_type *                  value_type;
    typedef element_type *                  pointer;
    typedef const element_type *            const_pointer;
    typedef element_type &                  reference;
    typedef const element_type &            const_reference;
    typedef std::size_t                     size_type;
    typedef shiftable_ptr<T>                this_type;

protected:
    pointer value_;
    bool    shifted_;

public:
    shiftable_ptr(pointer value = nullptr) : value_(value), shifted_(false) {
    }

    template <typename Other>
    shiftable_ptr(Other * value = nullptr) : value_(static_cast<pointer>(value)), shifted_(false) {
        MUST_BE_A_DERIVED_CLASS_OF(T, Other);
    }

    shiftable_ptr(this_type & src) : value_(nullptr), shifted_(false) {
        this->shift_from(src);
    }

    shiftable_ptr(this_type && src) : value_(nullptr), shifted_(false) {
        this->swap(src);
    }

    template <typename Other>
    shiftable_ptr(shiftable_ptr<Other> & src) : value_(nullptr), shifted_(false) {
        this->shift_from(src);
    }

    template <typename Other>
    shiftable_ptr(shiftable_ptr<Other> && src) : value_(nullptr), shifted_(false) {
        this->swap(std::forward<this_type>(src));
    }

    virtual ~shiftable_ptr() {
        this->destroy();
    }

    pointer value() const { return this->value_; }

    bool is_shifted() const { return this->shifted_; }
    void set_shifted(bool shifted = true) { this->shifted_ = shifted; }

protected:
    template <typename ...Args>
    JSTD_FORCE_INLINE
    void create_impl(Args && ... args) {
        this->value_   = reinterpret_cast<pointer>(new T(std::forward<Args>(args)...));
        this->shifted_ = false;
    }

    void destroy() {
        if (!this->shifted_) {
            if (this->value_) {
                delete_helper<T, std::is_array<T>::value>::delete_it(this->value_);
                this->value_ = nullptr;
            }
            this->shifted_ = true;
        }
    }

    JSTD_FORCE_INLINE
    void destroy_value() {
        if (!this->shifted_) {
            if (this->value_) {
                delete_helper<T, std::is_array<T>::value>::delete_it(this->value_);
            }
        }
    }

    void assign(pointer value) {
        this->value_   = value;
        this->shifted_ = false;
    }

    void purge() {
        this->value_ = nullptr;
        this->shifted_ = true;
    }

public:
    template <typename ...Args>
    JSTD_FORCE_INLINE
    void create(Args && ... args) {
        this->destroy_value();
        this->create_impl(std::forward<Args>(args)...);
    }

    this_type & operator = (this_type & rhs) {
        this->shift_from(rhs);
        return *this;
    }

    this_type & operator = (this_type && rhs) {
        this->swap(std::forward<this_type>(rhs));
        return *this;
    }

    this_type & operator = (pointer value) {
        this->set(value);
        return *this;
    }

    template <typename Other>
    this_type & operator = (Other * value) {
        MUST_BE_A_DERIVED_CLASS_OF(T, Other);
        this->set(static_cast<pointer>(value));
        return *this;
    }

    const_reference operator * () const {
        return (*(this->value_));
    }

    pointer operator -> () {
        return std::pointer_traits<pointer>::pointer_to(*(this->value_));
    }

    const_pointer operator -> () const {
        return std::pointer_traits<const_pointer>::pointer_to(*(this->value_));
    }

    pointer get() const { return this->value(); }

    void set(pointer value) {
        if (value != this->value_) {
            this->destroy_value();
            this->assign(value);
        }
    }

    template <typename Other>
    void set(Other * value) {
        MUST_BE_A_DERIVED_CLASS_OF(T, Other);
        this->set(static_cast<pointer>(value));
    }

    void clear() {
        this->destroy_value();
        this->purge();
    }

    void reset(pointer value = nullptr) {
        this->set(value);
    }

    template <typename Other>
    void reset(Other * value = nullptr) {
        this->set(value);
    }

    void copy_from(const this_type & src) {
        if (src.is_shifted()) {
            if (&src != this && src.value() != this->value()) {
                this->destroy_value();
                this->value_   = src.value_;
                this->shifted_ = true;
            }
        }
    }

    void shift_from(this_type & src) {
        if (!src.is_shifted()) {
            if (&src != this) {
                this->destroy_value();
                this->value_   = src.value_;
                this->shifted_ = src.shifted_;
                src.purge();
            }
        }
    }

    template <typename Other>
    void shift_from(shiftable_ptr<Other> & src) {
        MUST_BE_A_DERIVED_CLASS_OF(T, Other);
        if (!src.is_shifted()) {
            if (static_cast<void *>(&src) != static_cast<void *>(this)) {
                this->destroy_value();
                this->value_   = static_cast<pointer>(src.value_);
                this->shifted_ = src.shifted_;
                src.purge();
            }
        }
    }

    pointer shift() {
        this->shifted_ = true;
        return this->value_;
    }

    void swap(this_type & rhs) {
        if (&rhs != this) {
            std::swap(this->value_, rhs.value_);
            std::swap(this->shifted_, rhs.shifted_);
        }
    }

    template <typename Other>
    void swap(shiftable_ptr<Other> & rhs) {
        MUST_BE_A_DERIVED_CLASS_OF(T, Other);
        if (static_cast<void *>(&rhs) != static_cast<void *>(this)) {
            pointer tmp = this->value_;
            pointer rhs_value = static_cast<pointer>(rhs.value_);
            this->value_ = rhs_value;
            rhs.value_ = tmp;
            std::swap(this->shifted_, rhs.shifted_);
        }
    }
};

template < typename T, std::size_t Alignment = align_of<T>::value,
                       typename Allocator = allocator<T, Alignment> >
class custom_shiftable_ptr {
public:
    typedef T                               element_type;
    typedef element_type *                  value_type;
    typedef element_type *                  pointer;
    typedef const element_type *            const_pointer;
    typedef element_type &                  reference;
    typedef const element_type &            const_reference;
    typedef std::size_t                     size_type;
    typedef Allocator                       allocator_type;
    typedef custom_shiftable_ptr<T, Alignment, Allocator>
                                            this_type;

protected:
    pointer         value_;
    bool            shifted_;
    allocator_type  allocator_;

public:
    custom_shiftable_ptr(this_type & src) : value_(nullptr), shifted_(false) {
        this->shift_from(src);
    }

    custom_shiftable_ptr(this_type && src) : value_(nullptr), shifted_(false) {
        this->swap(std::forward<this_type>(src));
    }

    template <typename Other>
    custom_shiftable_ptr(custom_shiftable_ptr<Other, Alignment,
                         typename Allocator::template rebind<Other>::type> & src)
        : value_(nullptr), shifted_(false) {
        this->shift_from(src);
    }

    template <typename Other>
    custom_shiftable_ptr(custom_shiftable_ptr<Other, Alignment,
                         typename Allocator::template rebind<Other>::type> && src)
        : value_(nullptr), shifted_(false) {
        this->swap(std::forward<this_type>(src));
    }

    template <typename ...Args>
    custom_shiftable_ptr(Args && ... args) : value_(nullptr), shifted_(false) {
        this->create_impl(std::forward<Args>(args)...);
    }

    virtual ~custom_shiftable_ptr() {
        this->destroy();
    }

    pointer value() const { return this->value_; }

    bool is_shifted() const { return this->shifted_; }
    void set_shifted(bool shifted = true) { this->shifted_ = shifted; }

protected:
    template <typename ...Args>
    JSTD_FORCE_INLINE
    void create_impl(Args && ... args) {
        this->value_   = this->allocator_.create(std::forward<Args>(args)...);
        this->shifted_ = false;
    }

    void destroy() {
        if (!this->shifted_) {
            if (this->value_) {
                this->allocator_.destroy(this->value_);
                this->value_ = nullptr;
            }
            this->shifted_ = true;
        }
    }

    JSTD_FORCE_INLINE
    void destroy_value() {
        if (!this->shifted_) {
            if (this->value_) {
                this->allocator_.destroy(this->value_);
            }
        }
    }

    void purge() {
        this->value_ = nullptr;
        this->shifted_ = true;
    }

public:
    template <typename ...Args>
    JSTD_FORCE_INLINE
    void create(Args && ... args) {
        this->destroy_value();
        this->create_impl(std::forward<Args>(args)...);
    }

    this_type & operator = (this_type & rhs) {
        this->shift_from(rhs);
        return *this;
    }

    this_type & operator = (this_type && rhs) {
        this->swap(std::forward<this_type>(rhs));
        return *this;
    }

    const_reference operator * () const {
        return (*(this->value_));
    }

    pointer operator -> () {
        return std::pointer_traits<pointer>::pointer_to(*(this->value_));
    }

    const_pointer operator -> () const {
        return std::pointer_traits<const_pointer>::pointer_to(*(this->value_));
    }

    pointer get() const { return this->value(); }

    void set(this_type & src) {
        this->shift_from(src);
    }

    template <typename Other>
    void set(custom_shiftable_ptr<Other, Alignment,
             typename Allocator::template rebind<Other>::type> & src) {
        this->shift_from(src);
    }

    void clear() {
        this->destroy_value();
        this->purge();
    }

    void reset(this_type & src) {
        this->set(src);
    }

    template <typename Other>
    void reset(custom_shiftable_ptr<Other, Alignment,
               typename Allocator::template rebind<Other>::type> & src) {
        this->set(src);
    }

    void reset(this_type && src) {
        this->swap(src);
    }

    template <typename Other>
    void reset(custom_shiftable_ptr<Other, Alignment,
               typename Allocator::template rebind<Other>::type> && src) {
        this->swap(src);
    }

    void copy_from(const this_type & src) {
        if (src.is_shifted()) {
            if (&src != this && src.value() != this->value()) {
                this->destroy_value();
                this->value_   = src.value_;
                this->shifted_ = true;
            }
        }
    }

    void shift_from(this_type & src) {
        if (!src.is_shifted()) {
            if (&src != this) {
                this->destroy_value();
                this->value_   = src.value_;
                this->shifted_ = src.shifted_;
                src.purge();
            }
        }
    }

    template <typename Other>
    void shift_from(custom_shiftable_ptr<Other, Alignment,
                    typename Allocator::template rebind<Other>::type> & src) {
        MUST_BE_A_DERIVED_CLASS_OF(T, Other);
        if (!src.is_shifted()) {
            if (static_cast<void *>(&src) != static_cast<void *>(this)) {
                this->destroy_value();
                this->value_   = static_cast<pointer>(src.value_);
                this->shifted_ = src.shifted_;
                src.purge();
            }
        }
    }

    pointer shift() {
        this->shifted_ = true;
        return this->value_;
    }

    void swap(this_type & rhs) {
        if (&rhs != this) {
            std::swap(this->value_, rhs.value_);
            std::swap(this->shifted_, rhs.shifted_);
        }
    }

    template <typename Other>
    void swap(custom_shiftable_ptr<Other, Alignment,
              typename Allocator::template rebind<Other>::type> & rhs) {
        MUST_BE_A_DERIVED_CLASS_OF(T, Other);
        if (static_cast<void *>(&rhs) != static_cast<void *>(this)) {
            pointer tmp = this->value_;
            this->value_ = static_cast<pointer>(rhs.value_);
            rhs.value_ = tmp;
            std::swap(this->shifted_, rhs.shifted_);
        }
    }
};

} // namespace jstd

#endif // JSTD_MEMORY_SHIFTABLE_PTR_H
