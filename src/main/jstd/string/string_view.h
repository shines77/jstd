
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
#include <memory>
#include <type_traits>
#include <stdexcept>

#include "jstd/string/string_traits.h"
#include "jstd/string/string_iterator.h"
#include "jstd/string/string_libc.h"
#include "jstd/string/string_stl.h"
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

    static const size_type npos = size_type(-1);

protected:
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
        : data_(src.c_str()), length_(src.size()) {
    }
    basic_string_view(const this_type & src)
        : data_(src.c_str()), length_(src.size()) {
    }
    ~basic_string_view() {
        /* Do nothing! */
    }

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
        return const_cast<char_type *>(this->data_)[pos];
    }

    const_reference operator [] (size_t pos) const {
        return this->data_[pos];
    }

    // copy(dest, count, pos)

    size_type copy(char_type * dest, size_type count, size_type pos = 0) const {
        if (pos > this->size()) {
            throw std::out_of_range("basic_string_view<T>: "
                    "copy(dest, count, pos): pos out_of_range.");
        }
        size_type rcount = ((pos + count) <= this->size()) ? count : (this->size() - pos);
        return Traits::copy(dest, this->data() + pos, rcount);
    }

    // substr(pos, count)

    this_type substr(size_type pos = 0, size_type count = npos) const {
        if (pos > this->size()) {
            throw std::out_of_range("basic_string_view<T>: "
                    "substr(pos, count): pos out_of_range.");
        }
        size_type rcount;
        if (likely(count != npos))
            rcount = ((pos + count) <= this->size()) ? count : (this->size() - pos);
        else
            rcount = this->size() - pos;
        return this_type(this->data() + pos, rcount);
    }

    // is_equal(rhs)

    bool is_equal(const char_type * str) const {
        return Traits::is_equal(this->data(), str);
    }

    bool is_equal(const char_type * s1, const char_type * s2, size_type count) const noexcept {
        return Traits::is_equal(s1, s2, count);
    }

    bool is_equal(const char_type * s1, size_type len1, const char_type * s2, size_type len2) const noexcept {
        return Traits::is_equal(s1, len1, s2, len2);
    }

    bool is_equal(const this_type & rhs) const noexcept {
        return this->is_equal(this->data(), this->size(), rhs.data(), rhs.size());
    }

    bool is_equal(const string_type & rhs) const noexcept {
        return this->is_equal(this->data(), this->size(), rhs.data(), rhs.size());
    }

    bool is_equal(size_type pos, size_type count, const this_type & sv) const {
        if (pos > this->size()) {
            throw std::out_of_range("basic_string_view<T>: "
                    "is_equal(pos, count, sv): pos out_of_range.");
        }
        const char_type * s1 = this->data() + pos;
        return this->is_equal(s1, sv.data(), count);
    }

    bool is_equal(size_type pos, size_type count,
                          const char_type * str) const {
        if (pos > this->size()) {
            throw std::out_of_range("basic_string_view<T>: "
                    "is_equal(pos, count, str): pos out_of_range.");
        }
        constexpr size_type len2 = libc::StrLen(str);
        size_type rcount = ((pos + count) <= this->size()) ? count : (this->size() - pos);
        const char_type * s1 = this->data() + pos;
        return this->is_equal(s1, rcount, str, len2);
    }

    bool is_equal(size_type pos1, size_type count1, const this_type & sv,
                          size_type pos2, size_type count2) const {
        if (pos1 > this->size()) {
            throw std::out_of_range("basic_string_view<T>: "
                    "is_equal(pos1, count1, sv, pos2, count2): pos1 out_of_range.");
        }
        if (pos2 > sv.size()) {
            throw std::out_of_range("basic_string_view<T>: "
                    "is_equal(pos1, count1, sv, pos2, count2): pos2 out_of_range.");
        }
        size_type rcount1 = ((pos1 + count1) <= this->size()) ? count1 : (this->size() - pos1);
        size_type rcount2 = ((pos2 + count2) <= sv.size()) ? count2 : (sv.size() - pos2);
        const char_type * s1 = this->data() + pos1;
        const char_type * s2 = sv.data() + pos2;
        return this->is_equal(s1, rcount1, s2, rcount2);
    }

    bool is_equal(size_type pos1, size_type count1,
                          const char_type * str, size_type count2) const {
        if (pos1 > this->size()) {
            throw std::out_of_range("basic_string_view<T>: "
                    "is_equal(pos1, count1, sv, pos2, count2): pos1 out_of_range.");
        }
        constexpr size_type pos2 = 0;
        constexpr size_type len2 = libc::StrLen(str);
        size_type rcount1 = ((pos1 + count1) <= this->size()) ? count1 : (this->size() - pos1);
        size_type rcount2 = ((pos2 + count2) <= len2) ? count2 : (len2 - pos2);
        const char_type * s1 = this->data() + pos1;
        const char_type * s2 = str + pos2;
        return this->is_equal(s1, rcount1, s2, rcount2);
    }

    // compare(rhs)

    int compare(const char_type * str) const {
        return Traits::compare(this->data(), str);
    }

    int compare(const char_type * s1,
                          const char_type * s2,
                          size_type count) const noexcept {
        return Traits::compare(s1, s2, count);
    }

    int compare(const char_type * s1, size_type len1,
                          const char_type * s2, size_type len2) const noexcept {
        return Traits::compare(s1, len1, s2, len2);
    }

    int compare(const this_type & rhs) const noexcept {
        return this->compare(this->data(), this->size(), rhs.data(), rhs.size());
    }

    int compare(const string_type & rhs) const noexcept {
        return this->compare(this->data(), this->size(), rhs.data(), rhs.size());
    }

    int compare(size_type pos, size_type count, const this_type & sv) const {
        if (pos > this->size()) {
            throw std::out_of_range("basic_string_view<T>: "
                    "compare(pos, count, sv): pos out_of_range.");
        }
        const char_type * s1 = this->data() + pos;
        return this->compare(s1, sv.data(), count);
    }

    int compare(size_type pos, size_type count, const char_type * str) const {
        if (pos > this->size()) {
            throw std::out_of_range("basic_string_view<T>: "
                    "compare(pos, count, str): pos out_of_range.");
        }
        constexpr size_type len2 = libc::StrLen(str);
        size_type rcount = ((pos + count) <= this->size()) ? count : (this->size() - pos);
        const char_type * s1 = this->data() + pos;
        return this->compare(s1, rcount, str, len2);
    }

    int compare(size_type pos1, size_type count1, const this_type & sv,
                size_type pos2, size_type count2) const {
        if (pos1 > this->size()) {
            throw std::out_of_range("basic_string_view<T>: "
                    "compare(pos1, count1, sv, pos2, count2): pos1 out_of_range.");
        }
        if (pos2 > sv.size()) {
            throw std::out_of_range("basic_string_view<T>: "
                    "compare(pos1, count1, sv, pos2, count2): pos2 out_of_range.");
        }
        size_type rcount1 = ((pos1 + count1) <= this->size()) ? count1 : (this->size() - pos1);
        size_type rcount2 = ((pos2 + count2) <= sv.size()) ? count2 : (sv.size() - pos2);
        const char_type * s1 = this->data() + pos1;
        const char_type * s2 = sv.data() + pos2;
        return this->compare(s1, rcount1, s2, rcount2);
    }

    int compare(size_type pos1, size_type count1,
                const char_type * str, size_type count2) const {
        if (pos1 > this->size()) {
            throw std::out_of_range("basic_string_view<T>: "
                    "compare(pos1, count1, sv, pos2, count2): pos1 out_of_range.");
        }
        constexpr size_type pos2 = 0;
        constexpr size_type len2 = libc::StrLen(str);
        size_type rcount1 = ((pos1 + count1) <= this->size()) ? count1 : (this->size() - pos1);
        size_type rcount2 = ((pos2 + count2) <= len2) ? count2 : (len2 - pos2);
        const char_type * s1 = this->data() + pos1;
        const char_type * s2 = str + pos2;
        return this->compare(s1, rcount1, s2, rcount2);
    }

    string_type to_string() const {
        return std::move(string_type(this->data_, this->length_));
    }
}; // class basic_string_view<CharTy>

template <typename CharTy>
inline
bool operator == (const basic_string_view<CharTy> & lhs, const basic_string_view<CharTy> & rhs) {
    return lhs.is_equal(rhs);
}

template <typename CharTy>
inline
bool operator < (const basic_string_view<CharTy> & lhs, const basic_string_view<CharTy> & rhs) {
    return (lhs.compare(rhs) == jstd::CompareResult::IsSmaller);
}

template <typename CharTy>
inline
bool operator > (const basic_string_view<CharTy> & lhs, const basic_string_view<CharTy> & rhs) {
    return (lhs.compare(rhs) == jstd::CompareResult::IsBigger);
}

template <typename CharTy>
inline
void swap(basic_string_view<CharTy> & lhs, basic_string_view<CharTy> & rhs) {
    lhs.swap(rhs);
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
