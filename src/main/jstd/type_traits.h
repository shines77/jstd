
#ifndef JSTD_TYPE_TRAITS_H
#define JSTD_TYPE_TRAITS_H

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

#include "jstd/basic/stddef.h"
#include "jstd/basic/stdint.h"

#include <string>
#include <type_traits>
#include <utility>      // For std::pair<T1, T2>

namespace jstd {

struct false_type {
    typedef false_type type;
    static const bool value = false;
};

struct true_type {
    typedef true_type type;
    static const bool value = true;
};

template <bool condition, class T, class U>
struct condition_if {
    typedef U type;
};

template <class T, class U>
struct condition_if<true, T, U> {
    typedef T type;
};

template <bool condition>
struct boolean_if {
    typedef typename condition_if<condition, true_type, false_type>::type type;
    enum { value = type::value };
};

template <typename T>
struct integral_traits {
    typedef typename std::make_signed<T>::type      signed_type;
    typedef typename std::make_unsigned<T>::type    unsigned_type;

    static_assert(std::is_integral<T>::value,
        "Error: integral_traits<T> -- T must be a integral type.");

    // max bits
    static const size_t bits = sizeof(T) * 8;
    static const size_t max_shift = bits - 1;
    // 0x80000000UL;
    static const unsigned_type max_power2 = static_cast<unsigned_type>(1UL) << max_shift;
    // 0xFFFFFFFFUL;
    static const unsigned_type max_num = static_cast<unsigned_type>(-1);
};

template <typename T>
constexpr
const T & max(const T & a, const T & b) {
  return ((a > b) ? a : b);
}

template <typename T>
constexpr
const T & min(const T & a, const T & b) {
  return ((a < b) ? a : b);
}

//
// is_relocatable<T>
//
// Trait which can be added to user types to enable use of memcpy.
//
// Example:
// template <>
// struct is_relocatable<MyType> : std::true_type {};
//

template <typename T>
struct is_relocatable
    : std::integral_constant<bool,
                             (std::is_trivially_copy_constructible<T>::value &&
                              std::is_trivially_destructible<T>::value)> {};

template <typename T, typename U>
struct is_relocatable<std::pair<T, U>>
    : std::integral_constant<bool, (is_relocatable<T>::value &&
                                    is_relocatable<U>::value)> {};

template <typename T>
struct is_relocatable<const T> : is_relocatable<T> {};

// Template struct param_tester

struct void_warpper {
    void_warpper() {}

    template <typename... Args>
    void operator () (Args && ... args) {
        return void();
    }
};

// test if parameters are valid
template <class ...>
struct param_tester
{
    typedef void type;
};

// Alias template void_t
template <class... Types>
using void_t = typename param_tester<Types...>::type;

template <typename T>
struct remove_cv_rp {
    typedef typename std::remove_cv<
                typename std::remove_reference<
                    typename std::remove_pointer<T>::type
                >::type
            >::type type;
};

template <typename T>
struct remove_cv_rp_ext {
    typedef typename std::remove_extent<
                typename std::remove_cv<
                    typename std::remove_reference<
                        typename std::remove_pointer<T>::type
                    >::type
                >::type
            >::type type;
};

template <typename T>
struct remove_all {
    typedef typename std::remove_all_extents<
                typename std::remove_cv<
                    typename std::remove_reference<
                        typename std::remove_pointer<T>::type
                    >::type
                >::type
            >::type type;
};

template <typename Caller, typename Function, typename = void>
struct is_call_possible : public std::false_type {};

template <typename Caller, typename ReturnType, typename ... Args>
struct is_call_possible<Caller, ReturnType(Args...),
    typename std::enable_if<
        std::is_same<ReturnType, void>::value ||
        std::is_convertible<decltype(
            std::declval<Caller>().operator()(std::declval<Args>()...)
            //          ^^^^^^^^^^ replace this with the member you need.
        ), ReturnType>::value
    >::type
> : public std::true_type {};

//
// See: https://stackoverflow.com/questions/17201329/c11-ways-of-finding-if-a-type-has-member-function-or-supports-operator
//

#define TYPE_SUPPORTS(ClassName, Expr)                          \
template <typename U>                                           \
struct ClassName {                                              \
private:                                                        \
    template <typename>                                         \
    static constexpr std::false_type test(...);                 \
                                                                \
    template <typename T = U>                                   \
    static decltype((Expr), std::true_type{}) test(int);        \
                                                                \
public:                                                         \
    static constexpr bool value = decltype(test<U>(0))::value;  \
};

namespace detail {
    TYPE_SUPPORTS(SupportsBegin, std::begin(std::declval<T>()));
}

//
// See: https://stackoverflow.com/questions/18570285/using-sfinae-to-detect-a-member-function
//

//
// has_size
//
template <typename T>
class has_size {
public:
    typedef typename T::size_type size_type;

private:
    typedef char Yes;
    typedef char No[2];

    template <typename U, U>
    struct really_has;

    template <typename C>
    static Yes & Check(really_has<size_type (C::*)() const, &C::size> *);

    // EDIT: and you can detect one of several overloads... by overloading :)
    template <typename C>
    static Yes & Check(really_has<size_type (C::*)(), &C::size> *);

    template <typename>
    static No & Check(...);

public:
    static bool const value = (sizeof(Check<T>(0)) == sizeof(Yes));
};

template <typename T>
class has_size_cxx11 {
public:
    typedef typename T::size_type size_type;

private:
    typedef char Yes;
    typedef char No[2];

    template <typename C>
    static auto Check(void *)
        -> decltype(size_type{ std::declval<C const>().size() }, Yes{ });

    template <typename>
    static No & Check(...);

public:
    static bool const value = (sizeof(Check<T>(0)) == sizeof(Yes));
};

//
// has_entry_count
//
template <typename T>
class has_entry_count {
public:
    typedef typename T::size_type size_type;

private:
    typedef char Yes;
    typedef char No[2];

    template <typename C>
    static auto Check(void *)
        -> decltype(size_type{ std::declval<C const>().entry_count() }, Yes{ });

    template <typename>
    static No & Check(...);

public:
    static bool const value = (sizeof(Check<T>(0)) == sizeof(Yes));
};

template <typename T>
class call_entry_count {
public:
    typedef typename T::size_type size_type;

private:
    typedef char Yes;
    struct No {
        char data[2];
    };   

    static No s_No;

    template <typename C>
    static auto Call_entry_count(const C & t, size_type & count, void *)
        -> decltype(size_type{ std::declval<C const>().entry_count() }, Yes{ }) {
        count = t.entry_count();
        return Yes{ };
    };

    template <typename>
    static No & Call_entry_count(...) {
        return s_No;
    }

public:
    static size_type entry_count(const T & t) {
        size_type count = 0;
        Call_entry_count<T>(t, count, 0);
        return count;
    }
};

template <typename T>
typename call_entry_count<T>::No call_entry_count<T>::s_No;

//
// has_name
//
template <typename T>
class has_name {
private:
    typedef char Yes;
    typedef char No[2];

    template <typename C>
    static auto Check(void *)
        -> decltype(std::string{ std::declval<C const>().name() }, Yes{ });

    template <typename>
    static No & Check(...);

public:
    static bool const value = (sizeof(Check<T>(0)) == sizeof(Yes));
};

template <typename T>
class call_name {
private:
    typedef char Yes;
    struct No {
        char data[2];
    };   

    static No s_No;

    template <typename C>
    static auto Call_name(const C * t, std::string * sname, void *)
        -> decltype(std::string{ std::declval<C const>().name() }, Yes{ }) {
        *sname = C::name();
        return Yes{ };
    };

    template <typename>
    static No & Call_name(...) {
        return s_No;
    }

public:
    static std::string name() {
        std::string sname;
        T t;
        Call_name<T>(&t, &sname, 0);
        return sname;
    }
};

template <typename T>
typename call_name<T>::No call_name<T>::s_No;

//
// has_c_str
//

template <typename T, typename CharTy>
class has_c_str {
private:
    typedef char Yes;
    typedef char No[2];

    template <typename U, U>
    struct really_has;

    template <typename C>
    static Yes & Check(really_has<const CharTy * (C::*)() const, &C::c_str> *);

    template <typename>
    static No & Check(...);

public:
    static const bool value = (sizeof(Check<T>(0)) == sizeof(Yes));
};

template <typename T, typename CharTy>
class call_c_str {
private:
    typedef char Yes;
    struct No {
        char data[2];
    };    

    static No s_No;

    template <typename U, U>
    struct really_has;

    template <typename C>
    static const CharTy * Call_c_str(const C * s, really_has<const CharTy * (C::*)() const, &C::c_str> *) {
        return s->c_str();
    }

    template <typename C>
    static CharTy * Call_c_str(C * s, really_has<CharTy * (C::*)(), &C::c_str> *) {
        return s->c_str();
    }

    template <typename>
    static const CharTy * Call_c_str(...) {
        return nullptr;
    }

public:
    static const CharTy * c_str(const T & s) {
        const CharTy * data = Call_c_str<T>(&s, 0);
        return data;
    }
};

} // namespace jstd

#endif // JSTD_TYPE_TRAITS_H
