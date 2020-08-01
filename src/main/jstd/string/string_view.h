
#ifndef JSTD_STRING_VIEW_H
#define JSTD_STRING_VIEW_H

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

#include "jstd/basic/stddef.h"
#include "jstd/basic/stdint.h"
#include "jstd/basic/stdsize.h"

#include <string.h>
#include <assert.h>

#include <cstdint>  // for std::intptr_t
#include <cstddef>  // for std::ptrdiff_t
#include <cstring>
#include <string>
#include <type_traits>
#include <stdexcept>

#include "jstd/string/string_traits.h"
#include "jstd/string/string_iterator.h"
#include "jstd/string/string_libc.h"
#include "jstd/string/string_utils.h"

namespace jstd {

template <typename CharTy, typename Traits = jstd::string_traits<CharTy>>
class basic_string_view {
public:
    typedef CharTy              char_type;
    typedef CharTy              value_type;
    typedef std::size_t         size_type;
    typedef std::ptrdiff_t      difference_type;
    typedef CharTy *            pointer;
    typedef const CharTy *      const_pointer;
    typedef CharTy &            reference;
    typedef const CharTy &      const_reference;

    typedef jstd::string_iterator<basic_string_view<CharTy>>       iterator;
    typedef jstd::const_string_iterator<basic_string_view<CharTy>> const_iterator;

    typedef std::basic_string<char_type>        string_type;
    typedef basic_string_view<char_type>        this_type;

private:
    const char_type * data_;
    size_type         length_;

public:
    basic_string_view() : data_(nullptr), length_(0) {}
    basic_string_view(const char_type * data)
        : data_(data), length_(libc::StrLen(data)) {}
    basic_string_view(const char_type * data, size_type length)
        : data_(data), length_(length) {}
    basic_string_view(const char_type * first, const char_type * last)
        : data_(first), length_(size_type(last - first)) {}
    template <size_type N>
    basic_string_view(const char_type(&data)[N])
        : data_(data), length_(N - 1) {}
    basic_string_view(const string_type & src)
        : data_(src.c_str()), length_(src.size()) {}
    basic_string_view(const this_type & src)
        : data_(src.data()), length_(src.length()) {}
    ~basic_string_view() { /* Do nothing! */ }

    const char_type * data() const { return this->data_; }
    const char_type * c_str() const { return this->data(); }
    char_type * data() { return const_cast<char_type *>(this->data_); }
    char_type * c_str() { return const_cast<char_type *>(this->data()); }

    size_t length() const { return this->length_; }
    size_t size() const { return this->length(); }

    bool empty() const { return (this->size() == 0); }

    iterator begin() const { return iterator(this->data()); }
    iterator end() const { return iterator(this->data() + this->size()); }

    const_iterator cbegin() const { return const_iterator(this->data()); }
    const_iterator cend() const { return const_iterator(this->data() + this->size()); }

    const_reference front() const { return this->data_[0]; }
    const_reference back() const { return this->data_[this->size() - 1]; }

    basic_string_view & operator = (const char_type * data) {
        this->data_ = data;
        this->length_ = libc::StrLen(data);
        return *this;
    }

    basic_string_view & operator = (const string_type & rhs) {
        this->data_ = rhs.c_str();
        this->length_ = rhs.size();
        return *this;
    }

    basic_string_view & operator = (const this_type & rhs) {
        this->data_ = rhs.data();
        this->length_ = rhs.length();
        return *this;
    }

    void attach(const char_type * data, size_t length) {
        this->data_ = data;
        this->length_ = length;
    }

    void attach(const char_type * data) {
        this->attach(data, libc::StrLen(data));
    }

    void attach(const char_type * first, const char_type * last) {
        assert(last >= first);
        this->attach(first, size_type(last - first));
    }

    void attach(const char_type * data, size_type first, size_type last) {
        assert(last >= first);
        this->attach(data + first, size_type(last - first));
    }

    template <size_t N>
    void attach(const char_type(&data)[N]) {
        this->attach(data, N - 1);
    }

    void attach(const string_type & src) {
        this->attach(src.c_str(), src.size());
    }

    void attach(const this_type & src) {
        this->attach(src.data(), src.length());
    }

    void clear() {
        this->data_ = nullptr;
        this->length_ = 0;
    }

    void swap(this_type & right) {
        if (&right != this) {
            std::swap(this->data_, right.data_);
            std::swap(this->length_, right.length_);
        }
    }

    reference at(size_t pos) {
        if (pos < this->size())
            return this->data_[pos];
        else
            throw std::out_of_range("basic_string_view<T>::at(pos): out of range.");
    }

    const_reference at(size_t pos) const {
        if (pos < this->size())
            return this->data_[pos];
        else
            throw std::out_of_range("basic_string_view<T>::at(pos): out of range.");
    }

    reference operator [] (size_t pos) {
        return this->data_[pos];
    }

    const_reference operator [] (size_t pos) const {
        return this->data_[pos];
    }

    bool is_equal(const this_type & rhs) const {
        if (likely(&rhs != this)) {
            if (likely(this->data() != rhs.data() && this->size() != rhs.size())) {
                return jstd::StrUtils::is_equals(*this, rhs);
            }
        }
        return true;
    }

    bool is_equal(const string_type & rhs) const {
        if (likely(&rhs != this)) {
            if (likely(this->data() != rhs.data() && this->size() != rhs.size())) {
                return jstd::StrUtils::is_equals(this->data(), this->size(), rhs.data(), rhs.size());
            }
        }
        return true;
    }

    int compare(const this_type & rhs) const {
        if (likely(&rhs != this)) {
            if (likely(this->data() != rhs.data() && this->size() != rhs.size())) {
                return jstd::StrUtils::compare(*this, rhs);
            }
        }
        return jstd::StrUtils::IsEqual;
    }

    int compare(const string_type & rhs) const {
        if (likely(&rhs != this)) {
            if (likely(this->data() != rhs.data() && this->size() != rhs.size())) {
                return jstd::StrUtils::compare(this->data(), this->size(), rhs.data(), rhs.size());
            }
        }
        return jstd::StrUtils::IsEqual;
    }

    string_type toString() const {
        return std::move(string_type(this->data_, this->length_));
    }
}; // class basic_string_view<CharTy>

template <typename CharTy>
inline
void swap(basic_string_view<CharTy> & lhs, basic_string_view<CharTy> & rhs) {
    lhs.swap(rhs);
}

template <typename CharTy>
inline
bool operator == (const basic_string_view<CharTy> & lhs, const basic_string_view<CharTy> & rhs) {
    return lhs.is_equal(rhs);
}

template <typename CharTy>
inline
bool operator < (const basic_string_view<CharTy> & lhs, const basic_string_view<CharTy> & rhs) {
    return (lhs.compare(rhs) == jstd::StrUtils::IsSmaller);
}

template <typename CharTy>
inline
bool operator > (const basic_string_view<CharTy> & lhs, const basic_string_view<CharTy> & rhs) {
    return (lhs.compare(rhs) == jstd::StrUtils::IsBigger);
}

typedef basic_string_view<char>                         string_view;
typedef basic_string_view<wchar_t>                      wstring_view;

typedef basic_string_view<char>                         StringRef;
typedef basic_string_view<wchar_t>                      StringRefW;

} // namespace jstd

namespace std {

template <typename CharTy>
inline
void swap(jstd::basic_string_view<CharTy> & lhs, jstd::basic_string_view<CharTy> & rhs) {
    lhs.swap(rhs);
}

} // namespace std

#endif // JSTD_STRING_VIEW_H
