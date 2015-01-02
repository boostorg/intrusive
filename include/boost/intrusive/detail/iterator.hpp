/////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Ion Gaztanaga  2014-2014
//
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/intrusive for documentation.
//
/////////////////////////////////////////////////////////////////////////////

#ifndef BOOST_INTRUSIVE_DETAIL_ITERATOR_HPP
#define BOOST_INTRUSIVE_DETAIL_ITERATOR_HPP

#if defined(_MSC_VER)
#  pragma once
#endif

#include <cstddef>
#include <boost/intrusive/detail/std_fwd.hpp>
#include <boost/move/detail/iterator_traits.hpp>

namespace boost {
namespace intrusive {

using boost::movelib::iterator_traits;

template<class Category, class T, class Distance, class Pointer = T*, class Reference = T&>
struct iterator
{
   typedef Category  iterator_category;
   typedef T         value_type;
   typedef Distance  difference_type;
   typedef Pointer   pointer;
   typedef Reference reference;
};

namespace detail {

template<class InputIt, class Distance> inline
void advance_impl(InputIt& it, Distance n, const std::input_iterator_tag&)
{
   while(n--)
	   ++it;
}

template<class InputIt, class Distance> inline
void advance_impl(InputIt& it, Distance n, std::forward_iterator_tag &)
{
   while(n--)
	   ++it;
}

template<class InputIt, class Distance> inline
void advance_impl(InputIt& it, Distance n, std::bidirectional_iterator_tag &)
{
   for (; 0 < n; --n)
	   ++it;
   for (; n < 0; ++n)
	   --it;
}

template<class InputIt, class Distance>
inline void advance_impl(InputIt& it, Distance n, const std::random_access_iterator_tag &)
{
   it += n;
}

template<class InputIt, class Distance, class Category>
inline void distance_impl(InputIt first, InputIt last, Distance& off, const Category &)
{
   while(first != last){
	   ++off;
      ++first;
   }
}

template<class InputIt, class Distance> inline
void distance_impl(InputIt first, InputIt last, Distance& off, const std::random_access_iterator_tag&)
{
   off += last - first;
}

}  //namespace detail

template<class InputIt, class Distance>
inline void iterator_advance(InputIt& it, Distance n)
{	// increment iterator by offset, arbitrary iterators
   typedef typename boost::intrusive::iterator_traits<InputIt>::iterator_category category_t;
   boost::intrusive::detail::advance_impl(it, n, category_t());
}

template<class InputIt> inline
typename iterator_traits<InputIt>::difference_type iterator_distance(InputIt first, InputIt last)
{
   typename iterator_traits<InputIt>::difference_type off = 0;
   typedef typename boost::intrusive::iterator_traits<InputIt>::iterator_category category_t;
   boost::intrusive::detail::distance_impl(first, last, off, category_t());
   return off;
}

} //namespace intrusive
} //namespace boost

#endif //BOOST_INTRUSIVE_DETAIL_ITERATOR_HPP
