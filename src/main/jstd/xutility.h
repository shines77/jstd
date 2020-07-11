
#ifndef JSTD_XUTILITY_H
#define JSTD_XUTILITY_H

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

#include "jstd/basic/stdint.h"
#include "jstd/type_traits.h"

#include <cstdint>
#include <cstddef>
#include <cstdbool>

#include <type_traits>

namespace jstd {

// ITERATOR STUFF (from <iterator>)
// ITERATOR TAGS  (from <iterator>)

// identifying tag for input iterators
struct input_iterator_tag
{
};

// TRANSITION, remove for Dev15

// identifying tag for mutable iterators
struct mutable_iterator_tag
{	
};

// identifying tag for output iterators
struct output_iterator_tag : mutable_iterator_tag
{
};

// identifying tag for forward iterators
struct forward_iterator_tag : input_iterator_tag, mutable_iterator_tag
{
};

// identifying tag for bidirectional iterators
struct bidirectional_iterator_tag : forward_iterator_tag
{
};

// identifying tag for random-access iterators
struct random_access_iterator_tag : bidirectional_iterator_tag
{
};

// base type for iterator classes
template <class Category, class T, class Difference = std::ptrdiff_t,
          class Pointer = T *, class Reference = T &>
struct iterator
{
    typedef Category        iterator_category;
    typedef T               value_type;
    typedef Difference      difference_type;

    typedef Pointer         pointer;
    typedef Reference       reference;
};

// base for output iterators
typedef iterator<output_iterator_tag, void, void, void, void>   OutputIter;

// TEMPLATE CLASS iterator_traits

// empty for non-iterators
template <class T, class = void>
struct iterator_traits_base
{	
};

// defined if Iter::* types exist
template <class Iter>
struct iterator_traits_base< Iter, jstd::void_t<
    typename Iter::iterator_category,
    typename Iter::value_type,
    typename Iter::difference_type,
    typename Iter::pointer,
    typename Iter::reference> >
{
    typedef typename Iter::iterator_category    iterator_category;
    typedef typename Iter::value_type           value_type;
    typedef typename Iter::difference_type      difference_type;

    typedef typename Iter::pointer              pointer;
    typedef typename Iter::reference            reference;
};

// get traits from iterator Iter, if possible
template <class Iter>
struct iterator_traits : iterator_traits_base<Iter>
{
};

// get traits from pointer
template <class T>
struct iterator_traits<T *>
{
    typedef random_access_iterator_tag      iterator_category;
    typedef T                               value_type;
    typedef std::ptrdiff_t                  difference_type;

    typedef T *                             pointer;
    typedef T &                             reference;
};

// get traits from const pointer
template<class T>
struct iterator_traits<const T *>
{
    typedef random_access_iterator_tag  iterator_category;
    typedef T                           value_type;
    typedef std::ptrdiff_t              difference_type;

    typedef const T *                   pointer;
    typedef const T &                   reference;
};

// Alias template iter_value_t
template <class Iter>
using iter_value_t = typename iterator_traits<Iter>::value_type;

// Alias template iter_diff_t
template <class Iter>
using iter_diff_t = typename iterator_traits<Iter>::difference_type;

// Alias template iter_cat_t
template <class Iter>
using iter_cat_t = typename iterator_traits<Iter>::iterator_category;

// Template class is_iterator

template <class T, class = void>
struct is_iterator : std::false_type
{
    // default definition
};

template <class T>
struct is_iterator< T, jstd::void_t<
    typename iterator_traits<T>::iterator_category> > : std::true_type
{
    // defined if iterator_category is provided by iterator_traits<T>
};

} // namespace jstd

#endif // JSTD_XUTILITY_H
