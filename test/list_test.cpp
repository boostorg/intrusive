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
#include <boost/intrusive/detail/config_begin.hpp>
#include <boost/intrusive/list.hpp>
#include <boost/intrusive/pointer_traits.hpp>
#include "itestvalue.hpp"
#include "bptr_value.hpp"
#include "smart_ptr.hpp"
#include "common_functors.hpp"
#include <vector>
#include <boost/detail/lightweight_test.hpp>
#include "test_macros.hpp"
#include "test_container.hpp"
#include <boost/tti/tti.hpp>
#include <typeinfo>

using namespace boost::intrusive;

class my_tag;

template<class VoidPointer>
struct hooks
{
   typedef list_base_hook<void_pointer<VoidPointer> >                base_hook_type;
   typedef list_base_hook< link_mode<auto_unlink>
                         , void_pointer<VoidPointer>, tag<my_tag> >  auto_base_hook_type;
   typedef list_member_hook<void_pointer<VoidPointer>, tag<my_tag> > member_hook_type;
   typedef list_member_hook< link_mode<auto_unlink>
                           , void_pointer<VoidPointer> >             auto_member_hook_type;
   typedef nonhook_node_member< list_node_traits< VoidPointer >,
                                circular_list_algorithms
                              > nonhook_node_member_type;
};


template < typename List_Type, typename Value_Container >
struct test_list
{
   typedef List_Type list_type;
   typedef typename list_type::value_traits value_traits;
   typedef typename list_type::node_allocator_type node_allocator_type;
   typedef typename value_traits::value_type value_type;

   static void test_all(Value_Container& values);
   static void test_front_back(Value_Container& values);
   static void test_sort(Value_Container& values);
   static void test_merge(Value_Container& values);
   static void test_remove_unique(Value_Container& values);
   static void test_insert(Value_Container& values);
   static void test_shift(Value_Container& values);
   static void test_swap(Value_Container& values);
   static void test_clone(Value_Container& values);
   static void test_container_from_end(Value_Container& values);
};

template < typename List_Type, typename Value_Container >
void test_list< List_Type, Value_Container >::test_all(Value_Container& values)
{
   std::clog << "testing list with:\n"
             << "  value_type = " << typeid(value_type).name() << "\n"
             << "  sizeof(value_type) = " << sizeof(value_type) << "\n"
             << "  link_mode = " << value_traits::link_mode << "\n"
             << "  node = " << typeid(typename list_type::node).name() << "\n"
             << "  sizeof(node) = " << sizeof(typename list_type::node) << "\n"
             << "  node_ptr = " << typeid(typename list_type::node_ptr).name() << "\n"
             << "  sizeof(node_ptr) = " << sizeof(typename list_type::node_ptr) << "\n"
             << "  constant_time_size = " << list_type::constant_time_size << "\n"
             << "  external_header = " << list_type::external_header << "\n"
             << "  has_node_allocator = " << list_type::has_node_allocator << "\n"
             << "  has_value_allocator = " << list_type::has_value_allocator << "\n"
             << "  sizeof(list_type) = " << sizeof(list_type) << "\n";

   {
      list_type list(values.begin(), values.end());
      test::test_container(list);
      list.clear();
      list.insert(list.end(), values.begin(), values.end());
      test::test_sequence_container(list, values);
   }

   test_front_back(values);
   test_sort(values);
   test_merge(values);
   test_remove_unique(values);
   test_insert(values);
   test_shift(values);
   test_swap(values);
   test_clone(values);
   test_container_from_end(values);
}

//test: push_front, pop_front, push_back, pop_back, front, back, size, empty:
template < class List_Type, typename Value_Container >
void test_list< List_Type, Value_Container >
   ::test_front_back(Value_Container& values)
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
template < class List_Type, typename Value_Container >
void test_list< List_Type, Value_Container >
   ::test_sort(Value_Container& values)
{
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
template < class List_Type, typename Value_Container >
void test_list< List_Type, Value_Container >
   ::test_remove_unique (Value_Container& values)
{
   {
      list_type list(values.begin(), values.end());
      list.remove_if(is_even());
      int init_values [] = { 1, 3, 5 };
      TEST_INTRUSIVE_SEQUENCE( init_values, list.begin() );
   }
   {
      Value_Container values2(values); // NOTE: problematic copy of value container
      list_type list(values.begin(), values.end());
      list.insert(list.end(), values2.begin(), values2.end());
      list.sort();
      int init_values [] = { 1, 1, 2, 2, 3, 3, 4, 4, 5, 5 };
      TEST_INTRUSIVE_SEQUENCE( init_values, list.begin() );
      list.unique();
      int init_values2 [] = { 1, 2, 3, 4, 5 };
      TEST_INTRUSIVE_SEQUENCE( init_values2, list.begin() );
   }
}

//test: merge due to error in merge implementation:
template < class List_Type, typename Value_Container >
void test_list< List_Type, Value_Container >
   ::test_merge (Value_Container& values)
{
   list_type testlist1, testlist2;
   testlist1.push_front (values[0]);
   testlist2.push_front (values[4]);
   testlist2.push_front (values[3]);
   testlist2.push_front (values[2]);
   testlist1.merge (testlist2);

   int init_values [] = { 1, 3, 4, 5 };
   TEST_INTRUSIVE_SEQUENCE( init_values, testlist1.begin() );
}

//test: assign, insert, const_iterator, const_reverse_iterator, erase, s_iterator_to:
template < class List_Type, typename Value_Container >
void test_list< List_Type, Value_Container >
   ::test_insert(Value_Container& values)
{
   list_type testlist;
   testlist.assign (values.begin() + 2, values.begin() + 5);

   const list_type& const_testlist = testlist;
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
   ic = testlist.iterator_to (values[4]);
   BOOST_TEST (&*ic == &values[4]);

   ic = list_type::s_iterator_to (values[4]);
   BOOST_TEST (&*ic == &values[4]);

   i = testlist.erase (i);
   BOOST_TEST (i == testlist.end());

   {  int init_values [] = { 3, 1, 4 };
      TEST_INTRUSIVE_SEQUENCE( init_values, const_testlist.begin() );  }
}

template < class List_Type, typename Value_Container >
void test_list< List_Type, Value_Container >
   ::test_shift(Value_Container& values)
{
   list_type testlist;
   const int num_values = (int)values.size();
   std::vector<int> expected_values(num_values);

   for(int s = 1; s <= num_values; ++s){
      expected_values.resize(s);
      //Shift forward all possible positions 3 times
      for(int i = 0; i < s*3; ++i){
         testlist.insert(testlist.begin(), values.begin(), values.begin() + s);
         testlist.shift_forward(i);
         for(int j = 0; j < s; ++j){
            expected_values[(j + s - i%s) % s] = (j + 1);
         }
         TEST_INTRUSIVE_SEQUENCE_EXPECTED(expected_values, testlist.begin());
         testlist.clear();
      }

      //Shift backwards all possible positions
      for(int i = 0; i < s*3; ++i){
         testlist.insert(testlist.begin(), values.begin(), values.begin() + s);
         testlist.shift_backwards(i);
         for(int j = 0; j < s; ++j){
            expected_values[(j + i) % s] = (j + 1);
         }
         TEST_INTRUSIVE_SEQUENCE_EXPECTED(expected_values, testlist.begin());
         testlist.clear();
      }
   }
}

//test: insert (seq-version), swap, splice, erase (seq-version):
template < class List_Type, typename Value_Container >
void test_list< List_Type, Value_Container >
   ::test_swap(Value_Container& values)
{
   {
      list_type testlist1 (values.begin(), values.begin() + 2);
      list_type testlist2;
      testlist2.insert (testlist2.end(), values.begin() + 2, values.begin() + 5);
      testlist1.swap (testlist2);

      {  int init_values [] = { 3, 4, 5 };
         TEST_INTRUSIVE_SEQUENCE( init_values, testlist1.begin() );  }
      {  int init_values [] = { 1, 2 };
         TEST_INTRUSIVE_SEQUENCE( init_values, testlist2.begin() );  }

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
   {
      list_type testlist1 (values.begin(), values.begin() + 2);
      list_type testlist2 (values.begin() + 3, values.begin() + 5);

      //values[0].swap_nodes(values[2]);
      swap_nodes(values[0], values[2]);
      {  int init_values [] = { 3, 2 };
         TEST_INTRUSIVE_SEQUENCE( init_values, testlist1.begin() );  }

      //values[2].swap_nodes(values[4]);
      swap_nodes(values[2], values[4]);
      {  int init_values [] = { 5, 2 };
         TEST_INTRUSIVE_SEQUENCE( init_values, testlist1.begin() );  }
      {  int init_values [] = { 4, 3 };
         TEST_INTRUSIVE_SEQUENCE( init_values, testlist2.begin() );  }
   }
   {
      list_type testlist1 (values.begin(), values.begin() + 1);

      {  int init_values [] = { 1 };
         TEST_INTRUSIVE_SEQUENCE( init_values, testlist1.begin() );  }

      //values[1].swap_nodes(values[2]);
      swap_nodes(values[1], values[2]);

      {  int init_values [] = { 1 };
         TEST_INTRUSIVE_SEQUENCE( init_values, testlist1.begin() );  }

      //values[0].swap_nodes(values[2]);
      swap_nodes(values[0], values[2]);

      {  int init_values [] = { 3 };
         TEST_INTRUSIVE_SEQUENCE( init_values, testlist1.begin() );  }

      //values[0].swap_nodes(values[2]);
      swap_nodes(values[0], values[2]);

      {  int init_values [] = { 1 };
         TEST_INTRUSIVE_SEQUENCE( init_values, testlist1.begin() );  }
   }
}

template < class List_Type, typename Value_Container, bool >
struct test_container_from_end_impl
{
    void operator () (Value_Container&)
    {
        std::clog << "skipping test_container_from_end\n";
    }
};

template < class List_Type, typename Value_Container >
struct test_container_from_end_impl< List_Type, Value_Container, true >
{
    void operator () (Value_Container& values)
{
   typedef List_Type list_type;
   list_type testlist1 (values.begin(), values.begin() + values.size());
   BOOST_TEST (testlist1 == list_type::container_from_end_iterator(testlist1.end()));
   BOOST_TEST (testlist1 == list_type::container_from_end_iterator(testlist1.cend()));
}
};

template < class List_Type, typename Value_Container >
void test_list< List_Type, Value_Container >
   ::test_container_from_end(Value_Container& values)
{
    test_container_from_end_impl< List_Type, Value_Container, List_Type::has_container_from_iterator >()(values);
}

template < class List_Type, typename Value_Container, bool Has_Value_Allocator >
struct test_clone_impl
{
   typedef List_Type list_type;
   typedef typename list_type::value_traits value_traits;
   typedef typename list_type::node_allocator_type node_allocator_type;
   typedef typename value_traits::value_type value_type;
   void operator () (Value_Container& values)
{
      list_type testlist1 (values.begin(), values.begin() + values.size());
      list_type testlist2;

      testlist2.clone_from(testlist1, test::new_cloner<value_type>(), test::delete_disposer<value_type>());
      BOOST_TEST (testlist2 == testlist1);
      testlist2.clear_and_dispose(test::delete_disposer<value_type>());
      BOOST_TEST (testlist2.empty());
}
};

template < class List_Type, typename Value_Container >
struct test_clone_impl< List_Type, Value_Container, true >
{
   typedef List_Type list_type;
   typedef typename list_type::value_traits value_traits;
   typedef typename list_type::node_allocator_type node_allocator_type;
   typedef typename value_traits::value_type value_type;
   void operator () (Value_Container& values)
   {
      list_type testlist1 (values.begin(), values.begin() + values.size());
      list_type testlist2;

      testlist2.clone_from(testlist1);
      BOOST_TEST (testlist2 == testlist1);
      testlist2.clear_and_dispose();
      BOOST_TEST (testlist2.empty());
   }
};

template < class List_Type, typename Value_Container >
void test_list< List_Type, Value_Container >
   ::test_clone(Value_Container& values)
{
    test_clone_impl< List_Type, Value_Container, List_Type::has_value_allocator >()(values);
}

template < typename Value_Traits, bool Constant_Time_Size, bool Void_Allocator, typename Value_Container >
struct make_and_test_list
   : test_list< list< typename Value_Traits::value_type,
                      value_traits< Value_Traits >,
                      size_type< std::size_t >,
                      constant_time_size< Constant_Time_Size >,
                      node_allocator_type< std::allocator< void > >
                    >,
                Value_Container
              >
{};

template < typename Value_Traits, bool Constant_Time_Size, typename Value_Container >
struct make_and_test_list< Value_Traits, Constant_Time_Size, false, Value_Container >
   : test_list< list< typename Value_Traits::value_type,
                      value_traits< Value_Traits >,
                      size_type< std::size_t >,
                      constant_time_size< Constant_Time_Size >,
                      node_allocator_type< std::allocator< typename Value_Traits::node_traits::node > >
                    >,
                Value_Container
              >
{};


template < class VoidPointer, bool ConstantTimeSize, bool Void_Allocator >
class test_main_template
{
   public:
   int operator()()
   {
      typedef testvalue<hooks<VoidPointer>, ConstantTimeSize> value_type;
      std::vector<value_type> data (5);
      for (int i = 0; i < 5; ++i)
         data[i].value_ = i + 1;

      make_and_test_list < typename detail::get_base_value_traits <
                              value_type,
                              typename hooks<VoidPointer>::base_hook_type
                           >::type,
                           ConstantTimeSize,
                           Void_Allocator,
                           std::vector< value_type >
                         >::test_all(data);
      make_and_test_list < typename detail::get_member_value_traits <
                              value_type,
                              member_hook< value_type, typename hooks<VoidPointer>::member_hook_type, &value_type::node_>
                           >::type,
                           ConstantTimeSize,
                           Void_Allocator,
                           std::vector< value_type >
                         >::test_all(data);
      make_and_test_list< nonhook_node_member_value_traits <
                             value_type,
                             typename hooks<VoidPointer>::nonhook_node_member_type,
                             &value_type::nhn_member_,
                             safe_link
                          >,
                          ConstantTimeSize,
                          Void_Allocator,
                          std::vector< value_type >
                        >::test_all(data);

      return 0;
   }
};

template < class VoidPointer, bool Void_Allocator >
class test_main_template< VoidPointer, false, Void_Allocator >
{
   public:
   int operator()()
   {
      typedef testvalue<hooks<VoidPointer>, false> value_type;
      std::vector<value_type> data (5);
      for (int i = 0; i < 5; ++i)
         data[i].value_ = i + 1;

      make_and_test_list < typename detail::get_base_value_traits <
                              value_type,
                              typename hooks<VoidPointer>::base_hook_type
                           >::type,
                           false,
                           Void_Allocator,
                           std::vector< value_type >
                         >::test_all(data);
      make_and_test_list < typename detail::get_member_value_traits <
                              value_type,
                              member_hook< value_type, typename hooks<VoidPointer>::member_hook_type, &value_type::node_>
                           >::type,
                           false,
                           Void_Allocator,
                           std::vector< value_type >
                         >::test_all(data);

//      test_list<stateful_value_traits
//                  < value_type
//                  , list_node_traits<VoidPointer>
//                  , safe_link>
//               >::test_all(data);

      make_and_test_list < typename detail::get_base_value_traits <
                              value_type,
                              typename hooks<VoidPointer>::auto_base_hook_type
                           >::type,
                           false,
                           Void_Allocator,
                           std::vector< value_type >
                         >::test_all(data);
      make_and_test_list < typename detail::get_member_value_traits <
                              value_type,
                              member_hook< value_type, typename hooks<VoidPointer>::auto_member_hook_type, &value_type::auto_node_>
                            >::type,
                            false,
                         Void_Allocator,
                         std::vector< value_type >
                       >::test_all(data);

//      test_list<stateful_value_traits
//                  < value_type
//                  , list_node_traits<VoidPointer>
//                  , auto_unlink>
//               >::test_all(data);

      return 0;
   }
};

template < bool Constant_Time_Size >
struct test_main_template_bptr
{
   int operator()()
   {
      typedef BPtr_Value< circular_list_algorithms< List_BPtr_Node_Traits > > value_type;
      typedef List_BPtr_Value_Traits::node_ptr node_ptr;
      typedef Bounded_Allocator< value_type > allocator_type;

      allocator_type::init();
      allocator_type allocator;

      {
          Bounded_Reference_Cont< value_type > ref_cont;
          for (int i = 0; i < 5; ++i)
          {
              node_ptr tmp = allocator.allocate(1);
              new (tmp.raw()) value_type(i + 1);
              ref_cont.push_back(*tmp);
          }

          test_list < list < value_type,
                             value_traits< List_BPtr_Value_Traits >,
                             size_type< std::size_t >,
                             constant_time_size< Constant_Time_Size >,
                             node_allocator_type< Bounded_Allocator< value_type > >
                           >,
                      Bounded_Reference_Cont< value_type >
          >::test_all(ref_cont);
      }

      assert(allocator_type::is_clear());
      allocator_type::destroy();
      return 0;
   }
};

int main( int, char* [] )
{
   // test combinations of (regular/smart)_ptr x (const/nonconst)_size x (void/non-void)_allocator
   // skip smart_ptr with non-void_allocator
   test_main_template<void*, false, true>()();
   test_main_template<smart_ptr<void>, false, true>()();
   test_main_template<void*, true, true>()();
   test_main_template<smart_ptr<void>, true, true>()();
   test_main_template<void*, false, false>()();
   test_main_template<void*, true, false>()();

   // test bounded pointers with const/non_const size (always with non-void_allocator)
   test_main_template_bptr< true >()();
   test_main_template_bptr< false >()();
   return boost::report_errors();
}
