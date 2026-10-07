/////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Ion Gaztanaga  2014-2026
//
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/intrusive for documentation.
//
/////////////////////////////////////////////////////////////////////////////

//Compile-time checks of the size of containers, hooks and iterators
#include <boost/intrusive/list.hpp>
#include <boost/intrusive/slist.hpp>
#include <boost/intrusive/set.hpp>
#include <boost/intrusive/avl_set.hpp>
#include <boost/intrusive/bs_set.hpp>
#include <boost/intrusive/sg_set.hpp>
#include <boost/intrusive/splay_set.hpp>
#include <boost/intrusive/treap_set.hpp>
#include <boost/intrusive/unordered_set.hpp>
#include "itestvalue.hpp"   //heap_node_holder
#include <cstddef>

using namespace boost::intrusive;

template<class Hook>
struct node : Hook
{
   friend bool operator<(const node &, const node &) { return false; }
   friend bool operator==(const node &, const node &) { return true; }
   friend std::size_t hash_value(const node &) { return 0u; }
   friend bool priority_order(const node &, const node &) { return false; }
};

template<class Hook>
struct member_node
{
   Hook hook;
   friend bool operator<(const member_node &, const member_node &) { return false; }
   friend bool operator==(const member_node &, const member_node &) { return true; }
   friend std::size_t hash_value(const member_node &) { return 0u; }
};

//Checks that sizeof(TYPE) is WORDS words. The check depends on W
//so that it's only evaluated for the supported architectures.
#define BOOST_INTRUSIVE_TEST_WORDS(TYPE, WORDS) \
   BOOST_INTRUSIVE_STATIC_ASSERT((sizeof(TYPE) == W*(WORDS)))

//Checks the size of a container with forward iterators (in words) and the size of its iterators.
//Each check is in its own scope, as the C++03 emulation of static_assert
//declares a typedef whose name depends on the line.
#define BOOST_INTRUSIVE_TEST_FORWARD(TYPE, WORDS, IT_WORDS) \
   { BOOST_INTRUSIVE_TEST_WORDS(TYPE, WORDS); }\
   { BOOST_INTRUSIVE_TEST_WORDS(TYPE::iterator, IT_WORDS); }\
   { BOOST_INTRUSIVE_TEST_WORDS(TYPE::const_iterator, IT_WORDS); }\
//

//Checks the size of a container with reverse iterators (in words) and the size of its iterators.
#define BOOST_INTRUSIVE_TEST_CONTAINER(TYPE, WORDS, IT_WORDS) \
   BOOST_INTRUSIVE_TEST_FORWARD(TYPE, WORDS, IT_WORDS)\
   { BOOST_INTRUSIVE_TEST_WORDS(TYPE::reverse_iterator, IT_WORDS); }\
   { BOOST_INTRUSIVE_TEST_WORDS(TYPE::const_reverse_iterator, IT_WORDS); }\
//

//W is the word size. 0 means an uncommon architecture whose sizes are not tested
template<std::size_t W>
struct test_sizes
{
   static void list_sizes()
   {
      typedef node< list_base_hook<> >                                    value_t;
      typedef member_node< list_member_hook<> >                           mvalue_t;
      typedef node< list_base_hook< link_mode<auto_unlink> > >            avalue_t;
      typedef heap_node_holder< list_node<void*>* >                       holder_t;

      BOOST_INTRUSIVE_TEST_WORDS(list_base_hook<>, 2);
      BOOST_INTRUSIVE_TEST_WORDS(list_member_hook<>, 2);

      typedef list<value_t>                                                          c1;
      typedef list<value_t, constant_time_size<false> >                              c2;
      typedef list<mvalue_t, member_hook<mvalue_t, list_member_hook<>, &mvalue_t::hook> > c3;
      typedef list<mvalue_t, member_hook<mvalue_t, list_member_hook<>, &mvalue_t::hook>
                  , constant_time_size<false> >                                      c4;
      typedef list<avalue_t, constant_time_size<false> >                             c5;
      typedef list<value_t, header_holder_type<holder_t> >                           c6;
      typedef list<value_t, constant_time_size<false>, header_holder_type<holder_t> > c7;
      BOOST_INTRUSIVE_TEST_CONTAINER(c1, 3, 1);
      BOOST_INTRUSIVE_TEST_CONTAINER(c2, 2, 1);
      BOOST_INTRUSIVE_TEST_CONTAINER(c3, 3, 1);
      BOOST_INTRUSIVE_TEST_CONTAINER(c4, 2, 1);
      BOOST_INTRUSIVE_TEST_CONTAINER(c5, 2, 1);
      BOOST_INTRUSIVE_TEST_CONTAINER(c6, 2, 1);
      BOOST_INTRUSIVE_TEST_CONTAINER(c7, 1, 1);
   }

   static void slist_sizes()
   {
      typedef node< slist_base_hook<> >                                   value_t;
      typedef member_node< slist_member_hook<> >                          mvalue_t;
      typedef heap_node_holder< slist_node<void*>* >                      holder_t;

      BOOST_INTRUSIVE_TEST_WORDS(slist_base_hook<>, 1);
      BOOST_INTRUSIVE_TEST_WORDS(slist_member_hook<>, 1);

      typedef slist<value_t>                                                          c1;
      typedef slist<value_t, constant_time_size<false> >                              c2;
      typedef slist<value_t, cache_last<true> >                                       c3;
      typedef slist<value_t, constant_time_size<false>, cache_last<true> >            c4;
      typedef slist<value_t, linear<true> >                                           c5;
      typedef slist<value_t, constant_time_size<false>, linear<true> >                c6;
      typedef slist<value_t, constant_time_size<false>, linear<true>, cache_last<true> > c7;
      typedef slist<mvalue_t, member_hook<mvalue_t, slist_member_hook<>, &mvalue_t::hook>
                   , constant_time_size<false> >                                      c8;
      typedef slist<value_t, constant_time_size<false>, header_holder_type<holder_t> > c9;
      BOOST_INTRUSIVE_TEST_FORWARD(c1, 2, 1);
      BOOST_INTRUSIVE_TEST_FORWARD(c2, 1, 1);
      BOOST_INTRUSIVE_TEST_FORWARD(c3, 3, 1);
      BOOST_INTRUSIVE_TEST_FORWARD(c4, 2, 1);
      BOOST_INTRUSIVE_TEST_FORWARD(c5, 2, 1);
      BOOST_INTRUSIVE_TEST_FORWARD(c6, 1, 1);
      BOOST_INTRUSIVE_TEST_FORWARD(c7, 2, 1);
      BOOST_INTRUSIVE_TEST_FORWARD(c8, 1, 1);
      BOOST_INTRUSIVE_TEST_FORWARD(c9, 1, 1);
   }

   static void rbtree_sizes()
   {
      typedef node< set_base_hook<> >                                     value_t;
      typedef node< set_base_hook< optimize_size<true> > >                ovalue_t;
      typedef member_node< set_member_hook<> >                            mvalue_t;
      typedef heap_node_holder< rbtree_node<void*>* >                     holder_t;

      BOOST_INTRUSIVE_TEST_WORDS(set_base_hook<>, 4);
      BOOST_INTRUSIVE_TEST_WORDS(set_base_hook< optimize_size<true> >, 3);

      typedef set<value_t>                                                            c1;
      typedef set<value_t, constant_time_size<false> >                                c2;
      typedef multiset<value_t>                                                       c3;
      typedef multiset<value_t, constant_time_size<false> >                           c4;
      typedef set<ovalue_t>                                                           c5;
      typedef set<ovalue_t, constant_time_size<false> >                               c6;
      typedef set<mvalue_t, member_hook<mvalue_t, set_member_hook<>, &mvalue_t::hook> > c7;
      typedef multiset<mvalue_t, member_hook<mvalue_t, set_member_hook<>, &mvalue_t::hook>
                      , constant_time_size<false> >                                   c8;
      typedef set<value_t, header_holder_type<holder_t> >                             c9;
      typedef set<value_t, constant_time_size<false>, header_holder_type<holder_t> >  c10;
      BOOST_INTRUSIVE_TEST_CONTAINER(c1,  5, 1);
      BOOST_INTRUSIVE_TEST_CONTAINER(c2,  4, 1);
      BOOST_INTRUSIVE_TEST_CONTAINER(c3,  5, 1);
      BOOST_INTRUSIVE_TEST_CONTAINER(c4,  4, 1);
      BOOST_INTRUSIVE_TEST_CONTAINER(c5,  4, 1);
      BOOST_INTRUSIVE_TEST_CONTAINER(c6,  3, 1);
      BOOST_INTRUSIVE_TEST_CONTAINER(c7,  5, 1);
      BOOST_INTRUSIVE_TEST_CONTAINER(c8,  4, 1);
      BOOST_INTRUSIVE_TEST_CONTAINER(c9,  2, 1);
      BOOST_INTRUSIVE_TEST_CONTAINER(c10, 1, 1);
   }

   static void avltree_sizes()
   {
      typedef node< avl_set_base_hook<> >                                 value_t;
      typedef node< avl_set_base_hook< optimize_size<true> > >            ovalue_t;
      typedef heap_node_holder< avltree_node<void*>* >                    holder_t;

      BOOST_INTRUSIVE_TEST_WORDS(avl_set_base_hook<>, 4);
      BOOST_INTRUSIVE_TEST_WORDS(avl_set_base_hook< optimize_size<true> >, 3);

      typedef avl_set<value_t>                                                        c1;
      typedef avl_set<value_t, constant_time_size<false> >                            c2;
      typedef avl_multiset<ovalue_t, constant_time_size<false> >                      c3;
      typedef avl_set<value_t, header_holder_type<holder_t> >                         c4;
      typedef avl_set<value_t, constant_time_size<false>, header_holder_type<holder_t> > c5;
      BOOST_INTRUSIVE_TEST_CONTAINER(c1, 5, 1);
      BOOST_INTRUSIVE_TEST_CONTAINER(c2, 4, 1);
      BOOST_INTRUSIVE_TEST_CONTAINER(c3, 3, 1);
      BOOST_INTRUSIVE_TEST_CONTAINER(c4, 2, 1);
      BOOST_INTRUSIVE_TEST_CONTAINER(c5, 1, 1);
   }

   static void bstree_sizes()
   {
      typedef node< bs_set_base_hook<> >                                  value_t;
      typedef member_node< bs_set_member_hook<> >                         mvalue_t;

      BOOST_INTRUSIVE_TEST_WORDS(bs_set_base_hook<>, 3);

      typedef bs_set<value_t>                                                         c1;
      typedef bs_set<value_t, constant_time_size<false> >                             c2;
      typedef bs_multiset<mvalue_t, member_hook<mvalue_t, bs_set_member_hook<>, &mvalue_t::hook>
                         , constant_time_size<false> >                                c3;
      typedef splay_set<value_t>                                                      c4;
      typedef splay_set<value_t, constant_time_size<false> >                          c5;
      typedef treap_set<value_t>                                                      c6;
      typedef treap_set<value_t, constant_time_size<false> >                          c7;
      typedef treap_multiset<value_t, constant_time_size<false> >                     c8;
      typedef sg_set<value_t, floating_point<false> >                                 c9;
      typedef sg_multiset<value_t, floating_point<false> >                            c10;
      BOOST_INTRUSIVE_TEST_CONTAINER(c1,  4, 1);
      BOOST_INTRUSIVE_TEST_CONTAINER(c2,  3, 1);
      BOOST_INTRUSIVE_TEST_CONTAINER(c3,  3, 1);
      BOOST_INTRUSIVE_TEST_CONTAINER(c4,  4, 1);
      BOOST_INTRUSIVE_TEST_CONTAINER(c5,  3, 1);
      BOOST_INTRUSIVE_TEST_CONTAINER(c6,  4, 1);
      BOOST_INTRUSIVE_TEST_CONTAINER(c7,  3, 1);
      BOOST_INTRUSIVE_TEST_CONTAINER(c8,  3, 1);
      BOOST_INTRUSIVE_TEST_CONTAINER(c9,  5, 1);
      BOOST_INTRUSIVE_TEST_CONTAINER(c10, 5, 1);
      //Scapegoat trees with floating point store two float values
      typedef sg_set<value_t>                                                         c11;
      BOOST_INTRUSIVE_STATIC_ASSERT((sizeof(c11) == W*5 + sizeof(float)*2));
   }

   static void unordered_sizes()
   {
      typedef node< unordered_set_base_hook<> >                                                value_t;
      typedef node< unordered_set_base_hook< store_hash<true> > >                              hvalue_t;
      typedef node< unordered_set_base_hook< optimize_multikey<true> > >                       mvalue_t;
      typedef member_node< unordered_set_member_hook< store_hash<true>, optimize_multikey<true> > > mmvalue_t;

      BOOST_INTRUSIVE_TEST_WORDS(unordered_set_base_hook<>, 1);
      BOOST_INTRUSIVE_TEST_WORDS(unordered_set_base_hook< store_hash<true> >, 2);
      BOOST_INTRUSIVE_TEST_WORDS(unordered_set_base_hook< optimize_multikey<true> >, 2);
      typedef unordered_set_base_hook< store_hash<true>, optimize_multikey<true> > hmhook_t;
      BOOST_INTRUSIVE_TEST_WORDS(hmhook_t, 3);

      //Bucket traits (bucket pointer + bucket count) + optional size, cached begin and split count
      typedef unordered_set<value_t>                                                  c1;
      typedef unordered_set<value_t, constant_time_size<false> >                      c2;
      typedef unordered_set<value_t, constant_time_size<false>, power_2_buckets<true> > c3;
      typedef unordered_set<value_t, constant_time_size<false>, cache_begin<true> >   c4;
      typedef unordered_set<value_t, cache_begin<true> >                              c5;
      typedef unordered_set<value_t, constant_time_size<false>, linear_buckets<true> > c6;
      typedef unordered_set<value_t, constant_time_size<false>, fastmod_buckets<true> > c7;
      typedef unordered_set<hvalue_t, constant_time_size<false> >                     c8;
      typedef unordered_multiset<mvalue_t, constant_time_size<false> >                c9;
      typedef unordered_set<mvalue_t, incremental<true> >                             c10;
      typedef unordered_multiset< mmvalue_t
         , member_hook<mmvalue_t, unordered_set_member_hook< store_hash<true>, optimize_multikey<true> >, &mmvalue_t::hook>
         , constant_time_size<false> >                                                c11;
      typedef unordered_set<value_t, power_2_buckets<true> >                          c12;
      BOOST_INTRUSIVE_TEST_FORWARD(c1,  3, 2);
      BOOST_INTRUSIVE_TEST_FORWARD(c2,  2, 2);
      BOOST_INTRUSIVE_TEST_FORWARD(c3,  2, 2);
      BOOST_INTRUSIVE_TEST_FORWARD(c4,  3, 2);
      BOOST_INTRUSIVE_TEST_FORWARD(c5,  4, 2);
      BOOST_INTRUSIVE_TEST_FORWARD(c6,  2, 2);
      BOOST_INTRUSIVE_TEST_FORWARD(c7,  3, 2);   //fastmod_buckets also stores the index of the prime bucket count
      BOOST_INTRUSIVE_TEST_FORWARD(c8,  2, 2);
      BOOST_INTRUSIVE_TEST_FORWARD(c9,  2, 2);
      BOOST_INTRUSIVE_TEST_FORWARD(c10, 4, 2);
      BOOST_INTRUSIVE_TEST_FORWARD(c11, 2, 2);
      BOOST_INTRUSIVE_TEST_FORWARD(c12, 3, 2);
   }

   static void run()
   {
      list_sizes();
      slist_sizes();
      rbtree_sizes();
      avltree_sizes();
      bstree_sizes();
      unordered_sizes();
   }
};

//Uncommon architectures are not tested
template<>
struct test_sizes<0>
{
   static void run()
   {}
};

//Only test architectures where pointers and std::size_t have the same size (4 or 8 bytes)
static const std::size_t word_size =
   (sizeof(void*) == sizeof(std::size_t) && (sizeof(void*) == 4u || sizeof(void*) == 8u))
      ? sizeof(void*) : 0u;

int main()
{
   test_sizes<word_size>::run();
   return 0;
}
