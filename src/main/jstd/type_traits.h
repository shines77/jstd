
#ifndef JSTD_TYPE_TRAITS_H
#define JSTD_TYPE_TRAITS_H

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

#include "jstd/basic/stddef.h"
#include "jstd/basic/stdint.h"

#include <type_traits>
#include <utility>

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

struct void_warpper {
    void_warpper() {}

    template <typename... Args>
    void operator () (Args && ... args) {
        return void();
    }
};

// Template struct param_tester

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

template <typename T>
class has_size {
public:
    typedef typename T::size_type size_type;

private:
  typedef char Yes;
  typedef Yes  No[2];

  template <typename U, U>
  struct really_has;

  template <typename C>
  static Yes & Test(really_has<size_type (C::*)() const, &C::size> *);

  // EDIT: and you can detect one of several overloads... by overloading :)
  template <typename C>
  static Yes & Test(really_has<size_type (C::*)(), &C::size> *);

  template <typename>
  static No & Test(...);

public:
    static bool const value = (sizeof(Test<T>(0)) == sizeof(Yes));
};

template <typename T>
class has_size_cxx11 {
public:
    typedef typename T::size_type size_type;

private:
    typedef char Yes;
    typedef Yes  No[2];

    template <typename C>
    static auto Test(void *)
        -> decltype(size_type{ std::declval<C const>().size() }, Yes{ });

    template <typename>
    static No & Test(...);

public:
    static bool const value = (sizeof(Test<T>(0)) == sizeof(Yes));
};

template <typename T>
class has_entry_count {
public:
    typedef typename T::size_type size_type;

private:
    typedef char Yes;
    typedef Yes  No[2];

    template <typename C>
    static auto Test(void *)
        -> decltype(size_type{ std::declval<C const>().entry_count() }, Yes{ });

    template <typename>
    static No & Test(...);

public:
    static bool const value = (sizeof(Test<T>(0)) == sizeof(Yes));
};

template <typename T>
class call_entry_count {
public:
    typedef typename T::size_type size_type;

    struct No {
        char data[2];
    };

private:
    typedef char Yes;

    static No s_No;

    template <typename C>
    static auto Test(const C & t, size_type & count, void *)
        -> decltype(size_type{ std::declval<C const>().entry_count() }, Yes{ }) {
        count = t.entry_count();
        return Yes{ };
    };

    template <typename C>
    static No & Test(const C & t, size_type & count, ...) {
        return s_No;
    }

public:
    static size_type entry_count(const T & t) {
        size_type count = 0;
        Test(t, count, 0);
        return count;
    }
};

template <typename T>
typename call_entry_count<T>::No call_entry_count<T>::s_No;

} // namespace jstd

#endif // JSTD_TYPE_TRAITS_H
