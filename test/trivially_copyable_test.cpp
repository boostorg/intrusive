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

//Without compiler intrinsics is_trivially_copyable falls back to is_pod, which is
//false for any class type, so the checks are only meaningful when it is exact.
#if defined(BOOST_MOVE_IS_TRIVIALLY_COPYABLE)

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

namespace bmd = ::boost::move_detail;

typedef bi::list<node>           list_t;
typedef bi::slist<node>          slist_t;
typedef bi::set<node>            set_t;
typedef bi::avl_set<node>        avl_set_t;
typedef bi::unordered_set<node>  uset_t;

BOOST_INTRUSIVE_STATIC_ASSERT(bmd::is_trivially_copyable<list_t::iterator>::value);
BOOST_INTRUSIVE_STATIC_ASSERT(bmd::is_trivially_copyable<list_t::const_iterator>::value);
BOOST_INTRUSIVE_STATIC_ASSERT(bmd::is_trivially_copyable<slist_t::iterator>::value);
BOOST_INTRUSIVE_STATIC_ASSERT(bmd::is_trivially_copyable<slist_t::const_iterator>::value);
BOOST_INTRUSIVE_STATIC_ASSERT(bmd::is_trivially_copyable<set_t::iterator>::value);
BOOST_INTRUSIVE_STATIC_ASSERT(bmd::is_trivially_copyable<set_t::const_iterator>::value);
BOOST_INTRUSIVE_STATIC_ASSERT(bmd::is_trivially_copyable<avl_set_t::iterator>::value);
BOOST_INTRUSIVE_STATIC_ASSERT(bmd::is_trivially_copyable<uset_t::iterator>::value);
BOOST_INTRUSIVE_STATIC_ASSERT(bmd::is_trivially_copyable<uset_t::const_iterator>::value);
BOOST_INTRUSIVE_STATIC_ASSERT(bmd::is_trivially_copyable<uset_t::local_iterator>::value);

BOOST_INTRUSIVE_STATIC_ASSERT(bmd::is_trivially_copyable<set_t::key_compare>::value);
//Comparator holders: defaulted operations need C++11
#if !defined(BOOST_NO_CXX11_DEFAULTED_MOVES)
BOOST_INTRUSIVE_STATIC_ASSERT((bmd::is_trivially_copyable<bi::detail::ebo_functor_holder<std::less<int> > >::value));
BOOST_INTRUSIVE_STATIC_ASSERT((bmd::is_trivially_copyable<bi::detail::ebo_functor_holder<int*> >::value));
BOOST_INTRUSIVE_STATIC_ASSERT(bmd::is_trivially_copyable<set_t::value_compare>::value);
#endif

#endif   //BOOST_MOVE_IS_TRIVIALLY_COPYABLE

int main()
{
   return 0;
}
