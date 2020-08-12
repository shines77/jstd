
#ifndef JSTD_STRING_STRINF_VIEW_ARRAY_H
#define JSTD_STRING_STRINF_VIEW_ARRAY_H

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

#include "jstd/basic/stdint.h"
#include "jstd/basic/stdsize.h"
#include "jstd/string/string_view.h"

#include <cstdint>
#include <cstddef>
#include <vector>
#include <exception>

namespace jstd {

template <typename First, typename Second>
struct string_view_pair {
    typedef First   first_type;
    typedef Second  second_type;

    first_type  first;
    second_type second;

    string_view_pair() {}
    string_view_pair(const first_type & first, const second_type & second)
        : first(first), second(second) {
    }
    ~string_view_pair() {}
};

template <typename First, typename Second>
class string_view_array {
public:
    typedef First                                   first_type;
    typedef Second                                  second_type;
    typedef string_view_pair<First, Second>         element_type;
    typedef element_type *                          value_type;

    typedef std::vector<value_type>                 vector_type;
    typedef typename vector_type::iterator          iterator;
    typedef typename vector_type::const_iterator    const_iterator;
    typedef typename vector_type::pointer           pointer;
    typedef typename vector_type::const_pointer     const_pointer;
    typedef typename vector_type::reference         reference;
    typedef typename vector_type::const_reference   const_reference;
    typedef typename vector_type::size_type         size_type;

    typedef typename std::make_signed<size_type>::type  ssize_type;    

private:
    vector_type array_;

    void destroy() {
        for (size_type i = 0; i < array_.size(); i++) {
            value_type value = array_[i];
            if (value != nullptr) {
                delete value;
                array_[i] = nullptr;
            }
        }
    }

public:
    string_view_array() {}
    ~string_view_array() {
        destroy();
    }

    size_type size() const       { return array_.size();   }
    size_type capacity() const   { return array_.capacity(); }

    iterator begin()             { return array_.begin();  }
    iterator end()               { return array_.end();    }
    const_pointer begin() const  { return array_.begin();  }
    const_pointer end() const    { return array_.end();    }

    iterator cbegin()            { return array_.cbegin(); }
    iterator cend()              { return array_.cend();   }
    const_pointer cbegin() const { return array_.cbegin(); }
    const_pointer cend() const   { return array_.cend();   }

    reference front() { return array_.front(); }
    reference back()  { return array_.back();  }

    const_reference front() const { return array_.front(); }
    const_reference back() const  { return array_.back();  }

    void reserve(size_type count) {
        array_.reserve(count);
    }

    void resize(size_type new_size) {
        array_.resize(new_size);
    }

    void push_back(const value_type & value) {
        array_.push_back(value);
    }

    void push_back(value_type && value) {
        array_.push_back(value);
    }

    void pop_back() {
        array_.pop_back();
    }

    element_type & operator [] (size_type pos) {
        return *(array_[pos]);
    }

    const element_type & operator [] (size_type pos) const {
        return *(const_cast<const element_type *>(array_[pos]));
    }

    element_type & at(size_type pos) {
        if (pos < size())
            return *(array_[pos]);
        else
            throw std::out_of_range("string_view_array<F, S> outof of range.");
    }

    const element_type & at(size_type pos) const {
        if (pos < size())
            return *(const_cast<const element_type *>(array_[pos]));
        else
            throw std::out_of_range("string_view_array<F, S> outof of range.");
    }
};

} // namespace jstd

#endif // JSTD_STRING_STRINF_VIEW_ARRAY_H
