/////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Ion Gaztanaga  2026
//
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/intrusive for documentation.
//
/////////////////////////////////////////////////////////////////////////////

//Checks that iterators and comparator holders are trivially copyable so that
//compilers pass and return them in registers instead of by invisible reference.

#include <boost/config.hpp>
#include <boost/move/detail/type_traits.hpp>

//Without compiler intrinsics the Boost.Move traits fall back to is_pod, which is
//false for any class type, so the checks are only meaningful when intrinsics exist.
#if defined(BOOST_MOVE_HAS_TRIVIAL_COPY) && defined(BOOST_MOVE_HAS_TRIVIAL_ASSIGN) && defined(BOOST_MOVE_HAS_TRIVIAL_DESTRUCTOR)

#include <boost/intrusive/list.hpp>
#include <boost/intrusive/slist.hpp>
#include <boost/intrusive/set.hpp>
#include <boost/intrusive/avl_set.hpp>
#include <boost/intrusive/unordered_set.hpp>
#include <boost/intrusive/detail/ebo_functor_holder.hpp>
#include <boost/intrusive/detail/tree_value_compare.hpp>
#include <boost/intrusive/detail/workaround.hpp>

#include <functional>

namespace bi = boost::intrusive;

struct node
   : bi::list_base_hook<>
   , bi::slist_base_hook<>
   , bi::set_base_hook<>
   , bi::avl_set_base_hook<>
   , bi::unordered_set_base_hook<>
{
   int v;
   friend bool operator<(const node &a, const node &b)  { return a.v < b.v; }
   friend bool operator==(const node &a, const node &b) { return a.v == b.v; }
   friend std::size_t hash_value(const node &n) { return std::size_t(n.v); }
};

template<class T>
struct is_tc
{
   static const bool value = ::boost::move_detail::is_trivially_copy_constructible<T>::value
                          && ::boost::move_detail::is_trivially_copy_assignable<T>::value
                          && ::boost::move_detail::is_trivially_destructible<T>::value;
};

typedef bi::list<node>           list_t;
typedef bi::slist<node>          slist_t;
typedef bi::set<node>            set_t;
typedef bi::avl_set<node>        avl_set_t;
typedef bi::unordered_set<node>  uset_t;

BOOST_INTRUSIVE_STATIC_ASSERT(is_tc<list_t::iterator>::value);
BOOST_INTRUSIVE_STATIC_ASSERT(is_tc<list_t::const_iterator>::value);
BOOST_INTRUSIVE_STATIC_ASSERT(is_tc<slist_t::iterator>::value);
BOOST_INTRUSIVE_STATIC_ASSERT(is_tc<slist_t::const_iterator>::value);
BOOST_INTRUSIVE_STATIC_ASSERT(is_tc<set_t::iterator>::value);
BOOST_INTRUSIVE_STATIC_ASSERT(is_tc<set_t::const_iterator>::value);
BOOST_INTRUSIVE_STATIC_ASSERT(is_tc<avl_set_t::iterator>::value);
BOOST_INTRUSIVE_STATIC_ASSERT(is_tc<uset_t::iterator>::value);
BOOST_INTRUSIVE_STATIC_ASSERT(is_tc<uset_t::const_iterator>::value);
BOOST_INTRUSIVE_STATIC_ASSERT(is_tc<uset_t::local_iterator>::value);

BOOST_INTRUSIVE_STATIC_ASSERT(is_tc<set_t::key_compare>::value);
//Comparator holders: defaulted operations need C++11
#if !defined(BOOST_NO_CXX11_DEFAULTED_FUNCTIONS) && !defined(BOOST_NO_CXX11_RVALUE_REFERENCES)
BOOST_INTRUSIVE_STATIC_ASSERT((is_tc<bi::detail::ebo_functor_holder<std::less<int> > >::value));
BOOST_INTRUSIVE_STATIC_ASSERT((is_tc<bi::detail::ebo_functor_holder<int*> >::value));
BOOST_INTRUSIVE_STATIC_ASSERT(is_tc<set_t::value_compare>::value);
#endif

#endif   //BOOST_MOVE_HAS_TRIVIAL_COPY && BOOST_MOVE_HAS_TRIVIAL_ASSIGN && BOOST_MOVE_HAS_TRIVIAL_DESTRUCTOR

int main()
{
   return 0;
}
