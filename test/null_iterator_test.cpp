//////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Ion Gaztanaga 2014-2014. Distributed under the Boost
// Software License, Version 1.0. (See accompanying file
// LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/container for documentation.
//
//////////////////////////////////////////////////////////////////////////////

#include <boost/intrusive/list.hpp>
#include <boost/intrusive/slist.hpp>
#include <boost/intrusive/bs_set.hpp>
#include <boost/intrusive/set.hpp>
#include <boost/intrusive/avl_set.hpp>
#include <boost/intrusive/sg_set.hpp>
#include <boost/intrusive/treap_set.hpp>
#include <boost/intrusive/splay_set.hpp>
#include <boost/intrusive/unordered_set.hpp>
#include <boost/intrusive/detail/mpl.hpp>

#include <boost/core/lightweight_test.hpp>
#include <boost/aligned_storage.hpp>
#include <cstring>
#include <new>

using namespace boost::intrusive;

struct Type
   : list_base_hook<>
   , slist_base_hook<>
   , set_base_hook<>
   , avl_set_base_hook<>
   , bs_set_base_hook<>
   , unordered_set_base_hook<>
{
   friend bool operator==(const Type &, const Type &)
   {  return true;  }

   friend std::size_t hash_value(const Type &)
   {  return 0u;  }
};

typedef boost::aligned_storage<sizeof(void*)*4>::type buffer_t;

static buffer_t buffer_0x00;
static buffer_t buffer_0xFF;

template<class Iterator>
const Iterator &on_0x00_buffer()
{
   BOOST_INTRUSIVE_STATIC_ASSERT(sizeof(buffer_t) >= sizeof(Iterator));
   return * ::new(std::memset(&buffer_0x00, 0x00, sizeof(buffer_0x00))) Iterator();
}

template<class Iterator>
const Iterator &on_0xFF_buffer()
{
   BOOST_INTRUSIVE_STATIC_ASSERT(sizeof(buffer_t) >= sizeof(Iterator));
   return * ::new(std::memset(&buffer_0xFF, 0xFF, sizeof(buffer_0xFF))) Iterator();
}

BOOST_INTRUSIVE_INSTANTIATE_DEFAULT_TYPE_TMPLT(reverse_iterator)
BOOST_INTRUSIVE_INSTANTIATE_DEFAULT_TYPE_TMPLT(const_reverse_iterator)


template<class Container>
void check_null_iterators()
{
   typedef typename Container::iterator               iterator;
   typedef typename Container::const_iterator         const_iterator;
   typedef BOOST_INTRUSIVE_OBTAIN_TYPE_WITH_DEFAULT
      (::, Container
      ,reverse_iterator, iterator)                    reverse_iterator;
   typedef BOOST_INTRUSIVE_OBTAIN_TYPE_WITH_DEFAULT
      (::, Container
      ,const_reverse_iterator, const_iterator)        const_reverse_iterator;

   BOOST_TEST(on_0xFF_buffer<iterator>()               == on_0x00_buffer<iterator>());
   BOOST_TEST(on_0xFF_buffer<const_iterator>()         == on_0x00_buffer<const_iterator>());
   BOOST_TEST(on_0xFF_buffer<reverse_iterator>()       == on_0x00_buffer<reverse_iterator>());
   BOOST_TEST(on_0xFF_buffer<const_reverse_iterator>() == on_0x00_buffer<const_reverse_iterator>());
}

#ifndef BOOST_NO_CXX11_NOEXCEPT

//Iterator operations of all containers are noexcept
template<class Iterator>
void check_iterator_noexcept()
{
   Iterator i;
   BOOST_INTRUSIVE_STATIC_ASSERT(noexcept(Iterator()));
   BOOST_INTRUSIVE_STATIC_ASSERT(noexcept(Iterator(i)));
   BOOST_INTRUSIVE_STATIC_ASSERT(noexcept(*i));
   BOOST_INTRUSIVE_STATIC_ASSERT(noexcept(i.operator->()));
   BOOST_INTRUSIVE_STATIC_ASSERT(noexcept(++i));
   BOOST_INTRUSIVE_STATIC_ASSERT(noexcept(i++));
   BOOST_INTRUSIVE_STATIC_ASSERT(noexcept(i == i));
   BOOST_INTRUSIVE_STATIC_ASSERT(noexcept(i != i));
   (void)i;
}

template<class Iterator>
void check_bidirectional_iterator_noexcept()
{
   check_iterator_noexcept<Iterator>();
   Iterator i;
   BOOST_INTRUSIVE_STATIC_ASSERT(noexcept(--i));
   BOOST_INTRUSIVE_STATIC_ASSERT(noexcept(i--));
   (void)i;
}

template<class Container>
void check_container_iterators_noexcept(detail::true_)
{
   check_bidirectional_iterator_noexcept<typename Container::iterator>();
   check_bidirectional_iterator_noexcept<typename Container::const_iterator>();
}

template<class Container>
void check_container_iterators_noexcept(detail::false_)
{
   check_iterator_noexcept<typename Container::iterator>();
   check_iterator_noexcept<typename Container::const_iterator>();
}

template<class Container>
void check_container_iterators_noexcept()
{
   typedef typename Container::iterator iterator;
   typedef typename iterator::iterator_category category;
   check_container_iterators_noexcept<Container>
      (detail::bool_<detail::is_same<category, std::bidirectional_iterator_tag>::value>());
}

void check_iterators_noexcept()
{
   check_container_iterators_noexcept< list<Type> >();
   check_container_iterators_noexcept< slist<Type> >();
   check_container_iterators_noexcept< bs_set<Type> >();
   check_container_iterators_noexcept< set<Type> >();
   check_container_iterators_noexcept< avl_set<Type> >();
   check_container_iterators_noexcept< sg_set<Type> >();
   check_container_iterators_noexcept< treap_set<Type> >();
   check_container_iterators_noexcept< splay_set<Type> >();
   check_container_iterators_noexcept< unordered_set<Type> >();
   check_container_iterators_noexcept< unordered_set<Type, linear_buckets<true> > >();
}

#endif   //#ifndef BOOST_NO_CXX11_NOEXCEPT

int main()
{
   check_null_iterators< list<Type> >();
   check_null_iterators< slist<Type> >();
   check_null_iterators< bs_set<Type> >();
   check_null_iterators< set<Type> >();
   check_null_iterators< multiset<Type> >();
   check_null_iterators< avl_set<Type> >();
   check_null_iterators< avl_multiset<Type> >();
   check_null_iterators< sg_set<Type> >();
   check_null_iterators< sg_multiset<Type> >();
   check_null_iterators< treap_set<Type> >();
   check_null_iterators< treap_multiset<Type> >();
   check_null_iterators< splay_set<Type> >();
   check_null_iterators< splay_multiset<Type> >();
#ifndef BOOST_NO_CXX11_NOEXCEPT
   check_iterators_noexcept();
#endif

   return boost::report_errors();
}
