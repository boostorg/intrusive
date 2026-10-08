/////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Olaf Krzikalla 2004-2006.
// (C) Copyright Ion Gaztanaga  2006-2013.
//
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/intrusive for documentation.
//
/////////////////////////////////////////////////////////////////////////////
#include <boost/intrusive/list.hpp>
#include <boost/intrusive/pointer_traits.hpp>
#include "itestvalue.hpp"
#include "bptr_value.hpp"
#include "smart_ptr.hpp"
#include "common_functors.hpp"
#include <vector>
#include <boost/core/lightweight_test.hpp>
#include "test_macros.hpp"
#include "test_container.hpp"
#include <typeinfo>

using namespace boost::intrusive;

template<class VoidPointer>
struct hooks
{
   typedef list_base_hook<void_pointer<VoidPointer> >                base_hook_type;
   typedef list_base_hook< link_mode<auto_unlink>
                         , void_pointer<VoidPointer>, tag<void> >    auto_base_hook_type;
   typedef list_member_hook<void_pointer<VoidPointer>, tag<void> >   member_hook_type;
   typedef list_member_hook< link_mode<auto_unlink>
                           , void_pointer<VoidPointer> >             auto_member_hook_type;
   typedef nonhook_node_member< list_node_traits< VoidPointer >,
                                circular_list_algorithms >           nonhook_node_member_type;
};


template < typename ListType, typename ValueContainer >
struct test_list
{
   typedef ListType list_type;
   typedef typename list_type::value_traits value_traits;
   typedef typename value_traits::value_type value_type;
   typedef typename list_type::node_algorithms node_algorithms;

   static void test_all(ValueContainer&);
   static void test_front_back(ValueContainer&);
   static void test_sort(ValueContainer&);
   static void test_merge(ValueContainer&);
   static void test_remove_unique(ValueContainer&);
   static void test_insert(ValueContainer&);
   static void test_shift(ValueContainer&);
   static void test_swap(ValueContainer&);
   static void test_splice(ValueContainer&);
   static void test_clone(ValueContainer&);
   static void test_moved_from(ValueContainer&);
   static void test_container_from_end(ValueContainer&, detail::true_type);
   static void test_container_from_end(ValueContainer&, detail::false_type) {}
};

template < typename ListType, typename ValueContainer >
void test_list< ListType, ValueContainer >::test_all(ValueContainer& values)
{
   {
      list_type list(values.begin(), values.end());
      test::test_container(list);
      list.clear();
      list.insert(list.end(), values.begin(), values.end());
      test::test_sequence_container(list, values);
   }
   {
      list_type list(values.begin(), values.end());
      test::test_iterator_bidirectional(list);
   }

   test_front_back(values);
   test_sort(values);
   test_merge(values);
   test_remove_unique(values);
   test_insert(values);
   test_shift(values);
   test_swap(values);
   test_splice(values);
   test_clone(values);
   test_moved_from(values);
   test_container_from_end(values, detail::bool_< ListType::has_container_from_iterator >());
}

//test: push_front, pop_front, push_back, pop_back, front, back, size, empty:
template < class ListType, typename ValueContainer >
void test_list< ListType, ValueContainer >
   ::test_front_back(ValueContainer& values)
{
   list_type testlist;
   BOOST_TEST (testlist.empty());

   testlist.push_back (values[0]);
   BOOST_TEST (testlist.size() == 1);
   BOOST_TEST (&testlist.front() == &values[0]);
   BOOST_TEST (&testlist.back() == &values[0]);

   testlist.push_front (values[1]);
   BOOST_TEST (testlist.size() == 2);
   BOOST_TEST (&testlist.front() == &values[1]);
   BOOST_TEST (&testlist.back() == &values[0]);

   testlist.pop_back();
   BOOST_TEST (testlist.size() == 1);
   const list_type &const_testlist = testlist;
   BOOST_TEST (&const_testlist.front() == &values[1]);
   BOOST_TEST (&const_testlist.back() == &values[1]);

   testlist.pop_front();
   BOOST_TEST (testlist.empty());
}

//test: constructor, iterator, reverse_iterator, sort, reverse:
template < class ListType, typename ValueContainer >
void test_list< ListType, ValueContainer >
   ::test_sort(ValueContainer& values)
{
   //Lists with less than two elements
   {
      list_type emptylist;
      emptylist.sort();
      emptylist.sort(even_odd());
      BOOST_TEST(emptylist.empty());
      BOOST_TEST(emptylist.begin() == emptylist.end());

      list_type onelist;
      onelist.push_back(values[0]);
      onelist.sort();
      onelist.sort(even_odd());
      BOOST_TEST(onelist.size() == 1u);
      BOOST_TEST(&onelist.front() == &values[0]);
      BOOST_TEST(&onelist.back() == &values[0]);
      //The list is still usable after sorting
      onelist.push_front(values[1]);
      onelist.sort();
      {  int init_values [] = { 1, 2 };
         TEST_INTRUSIVE_SEQUENCE( init_values, onelist.begin() );  }
      onelist.clear();
   }
   //A power of two number of elements (the last merge of the sort
   //algorithm merges an empty list)
   {
      list_type fourlist(values.begin(), values.begin() + 4);
      fourlist.reverse();
      {  int init_values [] = { 4, 3, 2, 1 };
         TEST_INTRUSIVE_SEQUENCE( init_values, fourlist.begin() );  }
      fourlist.sort();
      {  int init_values [] = { 1, 2, 3, 4 };
         TEST_INTRUSIVE_SEQUENCE( init_values, fourlist.begin() );  }
      fourlist.sort(even_odd());
      {  int init_values [] = { 2, 4, 1, 3 };
         TEST_INTRUSIVE_SEQUENCE( init_values, fourlist.begin() );  }
      BOOST_TEST(fourlist.size() == 4u);
      fourlist.clear();
   }

   list_type testlist(values.begin(), values.end());

   {  int init_values [] = { 1, 2, 3, 4, 5 };
      TEST_INTRUSIVE_SEQUENCE( init_values, testlist.begin() );  }

   testlist.sort (even_odd());
   {  int init_values [] = { 5, 3, 1, 4, 2 };
      TEST_INTRUSIVE_SEQUENCE( init_values, testlist.rbegin() );  }

   testlist.reverse();
   {  int init_values [] = { 5, 3, 1, 4, 2 };
      TEST_INTRUSIVE_SEQUENCE( init_values, testlist.begin() );  }
}

//test: merge due to error in merge implementation:
template < class ListType, typename ValueContainer >
void test_list< ListType, ValueContainer >
   ::test_remove_unique (ValueContainer& values)
{
   {
      list_type list(values.begin(), values.end());
      const std::size_t old_size = list.size();
      const std::size_t removed  = list.remove_if(is_even());
      const std::size_t new_size = list.size();
      BOOST_TEST(removed == (old_size - new_size));
      int init_values [] = { 1, 3, 5 };
      TEST_INTRUSIVE_SEQUENCE( init_values, list.begin() );
   }
   {
      list_type list(values.begin(), values.end());
      const std::size_t old_size = list.size();
      const std::size_t removed  = list.remove_if(is_odd());
      const std::size_t new_size = list.size();
      BOOST_TEST(removed == (old_size - new_size));
      int init_values [] = { 2, 4 };
      TEST_INTRUSIVE_SEQUENCE( init_values, list.begin() );
   }
   {
      list_type list(values.begin(), values.end());
      const std::size_t old_size = list.size();
      const std::size_t removed  = list.remove_and_dispose_if(is_even(), test::empty_disposer());
      const std::size_t new_size = list.size();
      BOOST_TEST(removed == (old_size - new_size));
      int init_values [] = { 1, 3, 5 };
      TEST_INTRUSIVE_SEQUENCE( init_values, list.begin() );
   }
   {
      list_type list(values.begin(), values.end());
      const std::size_t old_size = list.size();
      const std::size_t removed  = list.remove_and_dispose_if(is_odd(), test::empty_noexcept_disposer());
      const std::size_t new_size = list.size();
      BOOST_TEST(removed == (old_size - new_size));
      int init_values [] = { 2, 4 };
      TEST_INTRUSIVE_SEQUENCE( init_values, list.begin() );
   }
   {
      ValueContainer values2(values);
      list_type list(values.begin(), values.end());
      list.insert(list.end(), values2.begin(), values2.end());
      list.sort();
      int init_values [] = { 1, 1, 2, 2, 3, 3, 4, 4, 5, 5 };
      TEST_INTRUSIVE_SEQUENCE( init_values, list.begin() );
      const std::size_t old_size = list.size();
      const std::size_t removed  = list.unique();
      const std::size_t new_size = list.size();
      BOOST_TEST(removed == (old_size - new_size));
      int init_values2 [] = { 1, 2, 3, 4, 5 };
      TEST_INTRUSIVE_SEQUENCE( init_values2, list.begin() );
   }
   {
      ValueContainer values2(values);
      list_type list(values.begin(), values.end());
      list.insert(list.end(), values2.begin(), values2.end());
      list.sort();
      int init_values [] = { 1, 1, 2, 2, 3, 3, 4, 4, 5, 5 };
      TEST_INTRUSIVE_SEQUENCE( init_values, list.begin() );
      const std::size_t old_size = list.size();
      const std::size_t removed  = list.unique_and_dispose(test::empty_disposer());
      const std::size_t new_size = list.size();
      BOOST_TEST(removed == (old_size - new_size));
      int init_values2 [] = { 1, 2, 3, 4, 5 };
      TEST_INTRUSIVE_SEQUENCE( init_values2, list.begin() );
   }
}

//test: merge due to error in merge implementation:
template < class ListType, typename ValueContainer >
void test_list< ListType, ValueContainer >
   ::test_merge (ValueContainer& values)
{
   list_type testlist1, testlist2;
   testlist1.push_front (values[0]);
   testlist2.push_front (values[4]);
   testlist2.push_front (values[3]);
   testlist2.push_front (values[2]);
   testlist1.merge (testlist2);

   int init_values [] = { 1, 3, 4, 5 };
   TEST_INTRUSIVE_SEQUENCE( init_values, testlist1.begin() );
   testlist1.clear();

   //Merge with itself has no effects
   {
      list_type l (values.begin(), values.begin() + 5);
      l.merge(l);
      std::size_t count = 0;
      test::counting_less less = { &count };
      l.merge(l, less);
      BOOST_TEST(count == 0u);
      BOOST_TEST(l.size() == 5u);
      int self_values [] = { 1, 2, 3, 4, 5 };
      TEST_INTRUSIVE_SEQUENCE( self_values, l.begin() );
      l.clear();
   }

   //The merge performs at most size() + x.size() - 1 comparisons
   {
      static const int first [4][4] = { {0, 2, 4, -1}, {1, 3, -1, -1}, {0, 1, -1, -1}, {2, 3, 4, -1} };
      static const int second[4][4] = { {1, 3, -1, -1}, {0, 2, 4, -1}, {2, 3, 4, -1}, {0, 1, -1, -1} };
      for(int c = 0; c != 4; ++c){
         list_type l1, l2;
         for(int i = 0; first[c][i] >= 0; ++i)
            l1.insert(l1.end(), values[std::size_t(first[c][i])]);
         for(int i = 0; second[c][i] >= 0; ++i)
            l2.insert(l2.end(), values[std::size_t(second[c][i])]);
         const std::size_t bound = l1.size() + l2.size() - 1u;
         std::size_t count = 0;
         test::counting_less less = { &count };
         l1.merge(l2, less);
         BOOST_TEST(count <= bound);
         BOOST_TEST(l2.empty());
         int merged_values [] = { 1, 2, 3, 4, 5 };
         TEST_INTRUSIVE_SEQUENCE( merged_values, l1.begin() );
         l1.clear();
      }
   }
}

//test: assign, insert, const_iterator, const_reverse_iterator, erase, s_iterator_to:
template < class ListType, typename ValueContainer >
void test_list< ListType, ValueContainer >
   ::test_insert(ValueContainer& values)
{
   list_type testlist;
   testlist.assign (values.begin() + 2, values.begin() + 5);

   const list_type& const_testlist = testlist;
   {  int init_values [] = { 3, 4, 5 };
      TEST_INTRUSIVE_SEQUENCE( init_values, const_testlist.begin() );  }

   testlist.dispose_and_assign (test::empty_disposer(), values.begin(), values.begin() + 2);
   {  int init_values [] = { 1, 2 };
      TEST_INTRUSIVE_SEQUENCE( init_values, const_testlist.begin() );  }
   testlist.dispose_and_assign (test::empty_disposer(), values.begin() + 2, values.begin() + 5);
   {  int init_values [] = { 3, 4, 5 };
      TEST_INTRUSIVE_SEQUENCE( init_values, const_testlist.begin() );  }

   typename list_type::iterator i = ++testlist.begin();
   BOOST_TEST (i->value_ == 4);

   {
   typename list_type::const_iterator ci = typename list_type::iterator();
   (void)ci;
   }

   testlist.insert (i, values[0]);
   {  int init_values [] = { 5, 4, 1, 3 };
      TEST_INTRUSIVE_SEQUENCE( init_values, const_testlist.rbegin() );  }

   i = testlist.iterator_to (values[4]);
   BOOST_TEST (&*i == &values[4]);

   i = list_type::s_iterator_to (values[4]);
   BOOST_TEST (&*i == &values[4]);

   typename list_type::const_iterator ic;
   ic = testlist.iterator_to (static_cast< typename list_type::const_reference >(values[4]));
   BOOST_TEST (&*ic == &values[4]);

   ic = list_type::s_iterator_to (static_cast< typename list_type::const_reference >(values[4]));
   BOOST_TEST (&*ic == &values[4]);

   i = testlist.erase (i);
   BOOST_TEST (i == testlist.end());

   {  int init_values [] = { 3, 1, 4 };
      TEST_INTRUSIVE_SEQUENCE( init_values, const_testlist.begin() );  }
}

template < class ListType, typename ValueContainer >
void test_list< ListType, ValueContainer >
   ::test_shift(ValueContainer& values)
{
   list_type testlist;
   const std::size_t num_values = values.size();
   std::vector<int> expected_values(num_values);

   for(std::size_t s = 1u; s <= num_values; ++s){
      expected_values.resize(s);
      //Shift forward all possible positions 3 times
      for(std::size_t i = 0u; i < s*3u; ++i){
         testlist.insert(testlist.begin(), values.begin(), values.begin() + std::ptrdiff_t(s));
         testlist.shift_forward(i);
         for(std::size_t j = 0u; j < s; ++j){
            expected_values[(j + s - i%s) % s] = int(j + 1u);
         }
         TEST_INTRUSIVE_SEQUENCE_EXPECTED(expected_values, testlist.begin());
         testlist.clear();
      }

      //Shift backwards all possible positions
      for(std::size_t i = 0u; i < s*3u; ++i){
         testlist.insert(testlist.begin(), values.begin(), values.begin() + std::ptrdiff_t(s));
         testlist.shift_backwards(i);
         for(std::size_t j = 0u; j < s; ++j){
            expected_values[(j + i) % s] = int(j + 1);
         }
         TEST_INTRUSIVE_SEQUENCE_EXPECTED(expected_values, testlist.begin());
         testlist.clear();
      }
   }
}

//test: swap, swap_nodes:
template < class ListType, typename ValueContainer >
void test_list< ListType, ValueContainer >
   ::test_swap(ValueContainer& values)
{
   {  //swap
      list_type testlist1 (values.begin(), values.begin() + 2);
      list_type testlist2;
      testlist2.insert (testlist2.end(), values.begin() + 2, values.begin() + 5);
      testlist1.swap (testlist2);

      {  int init_values [] = { 3, 4, 5 };
         TEST_INTRUSIVE_SEQUENCE( init_values, testlist1.begin() );  }
      {  int init_values [] = { 1, 2 };
         TEST_INTRUSIVE_SEQUENCE( init_values, testlist2.begin() );  }
   }
   {  //swap with itself has no effects
      list_type testlist1 (values.begin(), values.begin() + 3);
      testlist1.swap(testlist1);
      BOOST_TEST(testlist1.size() == 3u);
      {  int init_values [] = { 1, 2, 3 };
         TEST_INTRUSIVE_SEQUENCE( init_values, testlist1.begin() );  }
   }
   {
      list_type testlist1 (values.begin(), values.begin() + 2);
      list_type testlist2 (values.begin() + 3, values.begin() + 5);

      swap_nodes< node_algorithms >(values[0], values[2]);
      {  int init_values [] = { 3, 2 };
         TEST_INTRUSIVE_SEQUENCE( init_values, testlist1.begin() );  }

      swap_nodes< node_algorithms >(values[2], values[4]);
      {  int init_values [] = { 5, 2 };
         TEST_INTRUSIVE_SEQUENCE( init_values, testlist1.begin() );  }
      {  int init_values [] = { 4, 3 };
         TEST_INTRUSIVE_SEQUENCE( init_values, testlist2.begin() );  }
   }
   {  //Swap adjacent nodes of the same list, in both orders
      list_type testlist1 (values.begin(), values.begin() + 3);

      //this_node (values[0]) precedes other_node (values[1])
      swap_nodes< node_algorithms >(values[0], values[1]);
      {  int init_values [] = { 2, 1, 3 };
         TEST_INTRUSIVE_SEQUENCE( init_values, testlist1.begin() );  }
      {  int init_values [] = { 3, 1, 2 };
         TEST_INTRUSIVE_SEQUENCE( init_values, testlist1.rbegin() );  }
      BOOST_TEST(testlist1.size() == 3u);

      //other_node (values[1]) now precedes this_node (values[0])
      swap_nodes< node_algorithms >(values[0], values[1]);
      {  int init_values [] = { 1, 2, 3 };
         TEST_INTRUSIVE_SEQUENCE( init_values, testlist1.begin() );  }
      {  int init_values [] = { 3, 2, 1 };
         TEST_INTRUSIVE_SEQUENCE( init_values, testlist1.rbegin() );  }
      BOOST_TEST(testlist1.size() == 3u);

      //Adjacent nodes at the end of the list
      swap_nodes< node_algorithms >(values[1], values[2]);
      {  int init_values [] = { 1, 3, 2 };
         TEST_INTRUSIVE_SEQUENCE( init_values, testlist1.begin() );  }
      {  int init_values [] = { 2, 3, 1 };
         TEST_INTRUSIVE_SEQUENCE( init_values, testlist1.rbegin() );  }
      BOOST_TEST(testlist1.size() == 3u);

      swap_nodes< node_algorithms >(values[2], values[1]);
      {  int init_values [] = { 1, 2, 3 };
         TEST_INTRUSIVE_SEQUENCE( init_values, testlist1.begin() );  }
      {  int init_values [] = { 3, 2, 1 };
         TEST_INTRUSIVE_SEQUENCE( init_values, testlist1.rbegin() );  }
      BOOST_TEST(testlist1.size() == 3u);
   }
   {  //Swap the only two nodes of a list
      list_type testlist1 (values.begin(), values.begin() + 2);

      swap_nodes< node_algorithms >(values[0], values[1]);
      {  int init_values [] = { 2, 1 };
         TEST_INTRUSIVE_SEQUENCE( init_values, testlist1.begin() );  }
      {  int init_values [] = { 1, 2 };
         TEST_INTRUSIVE_SEQUENCE( init_values, testlist1.rbegin() );  }
      BOOST_TEST(testlist1.size() == 2u);

      swap_nodes< node_algorithms >(values[1], values[0]);
      {  int init_values [] = { 1, 2 };
         TEST_INTRUSIVE_SEQUENCE( init_values, testlist1.begin() );  }
      {  int init_values [] = { 2, 1 };
         TEST_INTRUSIVE_SEQUENCE( init_values, testlist1.rbegin() );  }
      BOOST_TEST(testlist1.size() == 2u);
   }
   {
      list_type testlist1 (values.begin(), values.begin() + 1);

      {  int init_values [] = { 1 };
         TEST_INTRUSIVE_SEQUENCE( init_values, testlist1.begin() );  }

      swap_nodes< node_algorithms >(values[1], values[2]);

      {  int init_values [] = { 1 };
         TEST_INTRUSIVE_SEQUENCE( init_values, testlist1.begin() );  }

      swap_nodes< node_algorithms >(values[0], values[2]);

      {  int init_values [] = { 3 };
         TEST_INTRUSIVE_SEQUENCE( init_values, testlist1.begin() );  }

      swap_nodes< node_algorithms >(values[0], values[2]);

      {  int init_values [] = { 1 };
         TEST_INTRUSIVE_SEQUENCE( init_values, testlist1.begin() );  }
   }
}

//test: splice between two lists and in the same list
template < class ListType, typename ValueContainer >
void test_list< ListType, ValueContainer >
   ::test_splice(ValueContainer& values)
{
   {  //splice between two lists, erase (seq-version)
      list_type testlist1 (values.begin() + 2, values.begin() + 5);
      list_type testlist2 (values.begin(), values.begin() + 2);

      testlist2.splice (++testlist2.begin(), testlist1);
      {  int init_values [] = { 1, 3, 4, 5, 2 };
         TEST_INTRUSIVE_SEQUENCE( init_values, testlist2.begin() );  }

      BOOST_TEST (testlist1.empty());

      testlist1.splice (testlist1.end(), testlist2, ++(++testlist2.begin()));
      {  int init_values [] = { 4 };
         TEST_INTRUSIVE_SEQUENCE( init_values, testlist1.begin() );  }

      {  int init_values [] = { 1, 3, 5, 2 };
         TEST_INTRUSIVE_SEQUENCE( init_values, testlist2.begin() );  }

      testlist1.splice (testlist1.end(), testlist2,
                        testlist2.begin(), ----testlist2.end());
      {  int init_values [] = { 4, 1, 3 };
         TEST_INTRUSIVE_SEQUENCE( init_values, testlist1.begin() );  }
      {  int init_values [] = { 5, 2 };
         TEST_INTRUSIVE_SEQUENCE( init_values, testlist2.begin() );  }

      testlist1.erase (testlist1.iterator_to(values[0]), testlist1.end());
      BOOST_TEST (testlist1.size() == 1);
      BOOST_TEST (&testlist1.front() == &values[3]);
   }

   {  //splice in the same list
      list_type testlist1 (values.begin(), values.begin() + 5);

      {  int init_values [] = { 1, 2, 3, 4, 5 };
         TEST_INTRUSIVE_SEQUENCE( init_values, testlist1.begin() );  }

      //nop 1
      testlist1.splice (testlist1.begin(), testlist1, testlist1.begin(), ++testlist1.begin());
      {  int init_values [] = { 1, 2, 3, 4, 5 };
         TEST_INTRUSIVE_SEQUENCE( init_values, testlist1.begin() );  }

      //nop 2
      testlist1.splice (++testlist1.begin(), testlist1, testlist1.begin(), ++testlist1.begin());
      {  int init_values [] = { 1, 2, 3, 4, 5 };
         TEST_INTRUSIVE_SEQUENCE( init_values, testlist1.begin() );  }

      //nop 3
      testlist1.splice (testlist1.begin(), testlist1, ++testlist1.begin(), ++testlist1.begin());
      {  int init_values [] = { 1, 2, 3, 4, 5 };
         TEST_INTRUSIVE_SEQUENCE( init_values, testlist1.begin() );  }

      testlist1.splice (testlist1.begin(), testlist1, ++testlist1.begin(), ++++testlist1.begin());
      {  int init_values [] = { 2, 1, 3, 4, 5 };
         TEST_INTRUSIVE_SEQUENCE( init_values, testlist1.begin() );  }

      testlist1.splice (testlist1.begin(), testlist1, ++testlist1.begin(), ++++++testlist1.begin());
      {  int init_values [] = { 1, 3, 2, 4, 5 };
         TEST_INTRUSIVE_SEQUENCE( init_values, testlist1.begin() );  }

      testlist1.splice (++++++++testlist1.begin(), testlist1, testlist1.begin(), ++++testlist1.begin());
      {  int init_values [] = { 2, 4, 1, 3, 5 };
         TEST_INTRUSIVE_SEQUENCE( init_values, testlist1.begin() );  }
   }

   //splice in the same list of a range that ends in the last element
   {  //range version: move { 3, 4 } before 2
      list_type testlist (values.begin(), values.begin() + 4);
      typename list_type::iterator f = testlist.begin();
      ++f;
      ++f;
      testlist.splice(++testlist.begin(), testlist, f, testlist.end());
      {  int init_values [] = { 1, 3, 4, 2 };
         TEST_INTRUSIVE_SEQUENCE( init_values, testlist.begin() );  }
      {  int init_values [] = { 2, 4, 3, 1 };
         TEST_INTRUSIVE_SEQUENCE( init_values, testlist.rbegin() );  }
      BOOST_TEST (&testlist.back() == &values[1]);
      testlist.push_back(values[4]);
      {  int init_values [] = { 1, 3, 4, 2, 5 };
         TEST_INTRUSIVE_SEQUENCE( init_values, testlist.begin() );  }
      BOOST_TEST (testlist.size() == 5u);
      testlist.check();
   }
   {  //single element version: move 4 before 2
      list_type testlist (values.begin(), values.begin() + 4);
      testlist.splice(++testlist.begin(), testlist, --testlist.end());
      {  int init_values [] = { 1, 4, 2, 3 };
         TEST_INTRUSIVE_SEQUENCE( init_values, testlist.begin() );  }
      {  int init_values [] = { 3, 2, 4, 1 };
         TEST_INTRUSIVE_SEQUENCE( init_values, testlist.rbegin() );  }
      BOOST_TEST (&testlist.back() == &values[2]);
      testlist.push_back(values[4]);
      {  int init_values [] = { 1, 4, 2, 3, 5 };
         TEST_INTRUSIVE_SEQUENCE( init_values, testlist.begin() );  }
      BOOST_TEST (testlist.size() == 5u);
      testlist.check();
   }
   {  //no-op splices of the last element(s) must not change it
      list_type testlist (values.begin(), values.begin() + 2);
      typename list_type::iterator last = --testlist.end();
      //p == e
      testlist.splice(testlist.end(), testlist, last, testlist.end());
      //p == f
      testlist.splice(last, testlist, last, testlist.end());
      //p is the next element of the moved one
      testlist.splice(testlist.end(), testlist, last);
      //p is the moved element
      testlist.splice(last, testlist, last);
      {  int init_values [] = { 1, 2 };
         TEST_INTRUSIVE_SEQUENCE( init_values, testlist.begin() );  }
      BOOST_TEST (&testlist.back() == &values[1]);
      BOOST_TEST (testlist.size() == 2u);
      testlist.push_back(values[4]);
      {  int init_values [] = { 1, 2, 5 };
         TEST_INTRUSIVE_SEQUENCE( init_values, testlist.begin() );  }
      testlist.check();
   }
}

template < class ListType, typename ValueContainer >
void test_list< ListType, ValueContainer >
   ::test_container_from_end(ValueContainer& values, detail::true_type)
{
   list_type testlist1 (values.begin(), values.begin() + std::ptrdiff_t(values.size()));
   BOOST_TEST (testlist1 == list_type::container_from_end_iterator(testlist1.end()));
   BOOST_TEST (testlist1 == list_type::container_from_end_iterator(testlist1.cend()));
}

template < class ListType, typename ValueContainer >
void test_list< ListType, ValueContainer >
   ::test_clone(ValueContainer& values)
{
      list_type testlist1 (values.begin(), values.begin() + std::ptrdiff_t(values.size()));
      list_type testlist2;

      //clone_from itself has no effects
      testlist1.clone_from(testlist1, test::new_cloner<value_type>(), test::delete_disposer<value_type>());
      testlist1.clone_from(boost::move(testlist1), test::new_nonconst_cloner<value_type>(), test::delete_disposer<value_type>());
      BOOST_TEST (testlist1.size() == values.size());
      BOOST_TEST (std::equal(testlist1.begin(), testlist1.end(), values.begin()));

      testlist2.clone_from(testlist1, test::new_cloner<value_type>(), test::delete_disposer<value_type>());
      BOOST_TEST (testlist2 == testlist1);
      testlist2.clear_and_dispose(test::delete_disposer<value_type>());
      BOOST_TEST (testlist2.empty());

      //Empty source, with an empty and a non-empty target
      list_type empty_list;
      testlist2.clone_from(empty_list, test::new_cloner<value_type>(), test::delete_disposer<value_type>());
      BOOST_TEST (testlist2.empty());
      BOOST_TEST (testlist2.begin() == testlist2.end());
      testlist2.clone_from(testlist1, test::new_cloner<value_type>(), test::delete_disposer<value_type>());
      testlist2.clone_from(empty_list, test::new_cloner<value_type>(), test::delete_disposer<value_type>());
      BOOST_TEST (testlist2.empty());
      BOOST_TEST (testlist2.begin() == testlist2.end());
      testlist2.clone_from(testlist1, test::new_cloner<value_type>(), test::delete_disposer<value_type>());
      testlist2.clone_from(boost::move(empty_list), test::new_nonconst_cloner<value_type>(), test::delete_disposer<value_type>());
      BOOST_TEST (testlist2.empty());
      BOOST_TEST (testlist2.begin() == testlist2.end());
      //The target is still usable
      testlist2.clone_from(testlist1, test::new_cloner<value_type>(), test::delete_disposer<value_type>());
      BOOST_TEST (testlist2 == testlist1);
      testlist2.clear_and_dispose(test::delete_disposer<value_type>());
      BOOST_TEST (testlist2.empty());
}

//test: the moved-from container is empty and can be used again
template < class ListType, typename ValueContainer >
void test_list< ListType, ValueContainer >
   ::test_moved_from(ValueContainer& values)
{
   typedef typename list_type::size_type size_type;
   typedef typename list_type::iterator  iterator;

   list_type src (values.begin(), values.end());
   const size_type size = src.size();
   list_type dst (boost::move(src));
   BOOST_TEST (dst.size() == size);

   //Observers, iteration and clear
   BOOST_TEST (src.empty());
   BOOST_TEST (src.size() == 0u);
   BOOST_TEST (src.begin() == src.end());
   src.clear();
   BOOST_TEST (src.empty());

   //clone_from to and from a moved-from container
   src.clone_from(dst, test::new_cloner<value_type>(), test::delete_disposer<value_type>());
   BOOST_TEST (src == dst);
   {
      list_type moved_from (boost::move(src));
      BOOST_TEST (src.empty());
      moved_from.clone_from(src, test::new_cloner<value_type>(), test::delete_disposer<value_type>());
      BOOST_TEST (moved_from.empty());
   }

   //swap
   src.swap(dst);
   BOOST_TEST (src.size() == size);
   BOOST_TEST (dst.empty());
   src.swap(dst);
   BOOST_TEST (dst.size() == size);

   //Insertions in a moved-from container
   dst.clear();
   src.insert(src.end(), values.begin(), values.end());
   BOOST_TEST (src.size() == size);
   {
      std::size_t i = 0;
      for(iterator it = src.begin(), itend = src.end(); it != itend; ++it, ++i){
         BOOST_TEST (&*it == &values[i]);
      }
   }

   //Move assignment to a moved-from container
   list_type moved (boost::move(src));
   BOOST_TEST (src.empty());
   src = boost::move(moved);
   BOOST_TEST (src.size() == size);
   BOOST_TEST (moved.empty());
   src.clear();
}

template < typename ValueTraits, bool ConstantTimeSize, bool Default_Holder, typename ValueContainer >
struct make_and_test_list
   : test_list< list< typename ValueTraits::value_type,
                      value_traits< ValueTraits >,
                      size_type< std::size_t >,
                      constant_time_size< ConstantTimeSize >
                    >,
                ValueContainer
              >
{};

template < typename ValueTraits, bool ConstantTimeSize, typename ValueContainer >
struct make_and_test_list< ValueTraits, ConstantTimeSize, false, ValueContainer >
   : test_list< list< typename ValueTraits::value_type,
                      value_traits< ValueTraits >,
                      size_type< std::size_t >,
                      constant_time_size< ConstantTimeSize >,
                      header_holder_type< heap_node_holder< typename ValueTraits::node_ptr > >
                    >,
                ValueContainer
              >
{};


enum HookType
{
   Base,
   Member,
   NonMember
};

//Each combination tests a single hook type to limit the instantiations per test
template < class VoidPointer, bool ConstantTimeSize, bool Default_Holder, HookType Type >
class test_main_template
{
   typedef testvalue_traits< hooks<VoidPointer> > testval_traits_t;
   typedef typename testval_traits_t::value_type value_type;

   //Auto-unlink hooks are only compatible with non-constant time size
   typedef typename detail::if_c
      < ConstantTimeSize
      , typename testval_traits_t::base_value_traits
      , typename testval_traits_t::auto_base_value_traits
      >::type base_value_traits_t;
   typedef typename detail::if_c
      < ConstantTimeSize
      , typename testval_traits_t::member_value_traits
      , typename testval_traits_t::auto_member_value_traits
      >::type member_value_traits_t;
   typedef typename detail::if_c
      < Type == Base
      , base_value_traits_t
      , typename detail::if_c
         < Type == Member
         , member_value_traits_t
         , typename testval_traits_t::nonhook_value_traits
         >::type
      >::type value_traits_t;

   public:
   int operator()()
   {
      std::vector<value_type> data (5);
      for (std::size_t i = 0u; i < 5u; ++i)
         data[i].value_ = (int)i + 1;

      make_and_test_list < value_traits_t
                         , ConstantTimeSize
                         , Default_Holder
                         , std::vector< value_type >
                         >::test_all(data);
      return 0;
   }
};

template < bool ConstantTimeSize >
struct test_main_template_bptr
{
   int operator()()
   {
      typedef BPtr_Value value_type;
      typedef BPtr_Value_Traits< List_BPtr_Node_Traits > list_value_traits;
      typedef typename list_value_traits::node_ptr node_ptr;
      typedef bounded_allocator< value_type > allocator_type;

      bounded_allocator_scope<allocator_type> bounded_scope; (void)bounded_scope;

      allocator_type allocator;

      {
          bounded_reference_cont< value_type > ref_cont;
          for (std::size_t i = 0; i < 5u; ++i)
          {
              node_ptr tmp = allocator.allocate(1);
              new (tmp.raw()) value_type((int)i + 1);
              ref_cont.push_back(*tmp);
          }

          test_list < list < value_type,
                             value_traits< list_value_traits >,
                             size_type< std::size_t >,
                             constant_time_size< ConstantTimeSize >,
                             header_holder_type< bounded_pointer_holder< value_type > >
                           >,
                      bounded_reference_cont< value_type >
          >::test_all(ref_cont);
      }

      return 0;
   }
};

int main()
{
   //Combinations: VoidPointer x ConstantTimeSize x Default_Holder x HookType
   //Minimize them selecting different combinations for raw and smart pointers

   //void pointer
   test_main_template<void*, false, false, Base>()();
   test_main_template<void*, false, true, Member>()();
   test_main_template<void*, true, false, Base>()();
   test_main_template<void*, true, true, NonMember>()();

   //smart_ptr
   test_main_template<boost::intrusive::smart_ptr<void>, false, true, Base>()();
   test_main_template<boost::intrusive::smart_ptr<void>, true, false, Member>()();

   //bounded_ptr (bool ConstantTimeSize)
   test_main_template_bptr< true >()();
   test_main_template_bptr< false >()();

   return boost::report_errors();
}
