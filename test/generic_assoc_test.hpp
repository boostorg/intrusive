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
#include <boost/container/vector.hpp> //vector
#include <algorithm> //sort, random_shuffle
#include <boost/intrusive/detail/config_begin.hpp>
#include "common_functors.hpp"
#include <boost/intrusive/options.hpp>
#include <boost/detail/lightweight_test.hpp>
#include "test_macros.hpp"
#include "test_container.hpp"

namespace boost{
namespace intrusive{
namespace test{

template<class T>
struct has_splay
{
   static const bool value = false;
};

template<class T>
struct has_rebalance
{
   static const bool value = false;
};

template<class T>
struct has_insert_before
{
   static const bool value = false;
};

template<class T>
struct has_const_searches
{
   static const bool value = true;
};

template<class T, bool = has_const_searches<T>::value>
struct search_const_iterator
{
   typedef typename T::const_iterator type;
};

template<class T>
struct search_const_iterator<T, false>
{
   typedef typename T::iterator type;
};

template<class T, bool = has_const_searches<T>::value>
struct search_const_container
{
   typedef const T type;
};

template<class T>
struct search_const_container<T, false>
{
   typedef T type;
};

template<class ValueTraits, template <class = void, class = void, class = void, class = void> class ContainerDefiner>
struct test_generic_assoc
{
   typedef typename ValueTraits::value_type value_type;
   typedef typename Value_Container< value_type >::type value_cont_type;
   typedef typename ValueTraits::reference reference;
   typedef typename ValueTraits::const_reference const_reference;
   static void test_all(value_cont_type&);
   static void test_clone(value_cont_type&);
   static void test_insert_erase_burst();
   static void test_container_from_end(value_cont_type&, detail::true_type);
   static void test_container_from_end(value_cont_type&, detail::false_type) {}
   static void test_splay_up(value_cont_type&, detail::true_type);
   static void test_splay_up(value_cont_type&, detail::false_type) {}
   static void test_splay_down(value_cont_type&, detail::true_type);
   static void test_splay_down(value_cont_type&, detail::false_type) {}
   static void test_rebalance(value_cont_type&, detail::true_type);
   static void test_rebalance(value_cont_type&, detail::false_type) {}
   static void test_insert_before(value_cont_type&, detail::true_type);
   static void test_insert_before(value_cont_type&, detail::false_type) {}
   static void test_container_from_iterator(value_cont_type&, detail::true_type);
   static void test_container_from_iterator(value_cont_type&, detail::false_type) {}
};

template<class ValueTraits, template <class = void, class = void, class = void, class = void> class ContainerDefiner>
void test_generic_assoc<ValueTraits, ContainerDefiner>::
   test_container_from_iterator(value_cont_type& values, detail::true_type)
{
   typedef ContainerDefiner
      < value_type
      , value_traits<ValueTraits>
      , constant_time_size<value_type::constant_time_size>
      > definer_function;
   typedef typename definer_function::type assoc_type;
   assoc_type testset(values.begin(), values.end());
   typedef typename assoc_type::iterator        it_type;
   typedef typename assoc_type::const_iterator  cit_type;
   typedef typename assoc_type::size_type       sz_type;
   sz_type sz = testset.size();
   for(it_type b(testset.begin()), e(testset.end()); b != e; ++b)
   {
      assoc_type &s = assoc_type::container_from_iterator(b);
      const assoc_type &cs = assoc_type::container_from_iterator(cit_type(b));
      BOOST_TEST(&s == &cs);
      BOOST_TEST(&s == &testset);
      s.erase(b);
      BOOST_TEST(testset.size() == (sz-1));
      s.insert(*b);
      BOOST_TEST(testset.size() == sz);
   }
}

template<class ValueTraits, template <class = void, class = void, class = void, class = void> class ContainerDefiner>
void test_generic_assoc<ValueTraits, ContainerDefiner>::test_insert_erase_burst()
{
   typedef typename ValueTraits::value_type value_type;

   //value_cont_type values;
   const int MaxValues = 100;
   value_cont_type values(MaxValues);
   for(int i = 0; i != MaxValues; ++i){
      (&values[i])->value_ = i;
   }

   typedef ContainerDefiner
      < value_type
      , value_traits<ValueTraits>
      , constant_time_size<value_type::constant_time_size>
      > definer_function;
   typedef typename definer_function::type assoc_type;
   typedef typename assoc_type::iterator iterator;

   {  //Ordered insertion + erasure
      assoc_type testset (values.begin(), values.begin() + values.size());
      TEST_INTRUSIVE_SEQUENCE_EXPECTED(testset, testset.begin());

      iterator it(testset.begin()), itend(testset.end());
      for(int i = 0; it != itend; ++i){
         BOOST_TEST(&*it == &values[i]);
         it = testset.erase(it);
      }
      BOOST_TEST(testset.empty());
   }
  
   {  //Now random insertions + erasure
      assoc_type testset;
      typedef typename value_cont_type::iterator vec_iterator;
      boost::container::vector<vec_iterator> it_vector;
      //Random insertion
      for(vec_iterator it(values.begin()), itend(values.end())
         ; it != itend
         ; ++it){
         it_vector.push_back(it);
      }
      for(int i = 0; i != MaxValues; ++i){
         testset.insert(*it_vector[i]);
      }
      TEST_INTRUSIVE_SEQUENCE_EXPECTED(testset, testset.begin());
      //Random erasure
      std::random_shuffle(it_vector.begin(), it_vector.end());
      for(int i = 0; i != MaxValues; ++i){
         testset.erase(testset.iterator_to(*it_vector[i]));
      }
      BOOST_TEST(testset.empty());
   }
}

template<class ValueTraits, template <class = void, class = void, class = void, class = void> class ContainerDefiner>
void test_generic_assoc<ValueTraits, ContainerDefiner>::test_all(value_cont_type& values)
{
   typedef ContainerDefiner
      < value_type
      , value_traits<ValueTraits>
      , constant_time_size<value_type::constant_time_size>
      > definer_function;
   typedef typename definer_function::type assoc_type;

   test_clone(values);
   test_container_from_end(values, detail::bool_< assoc_type::has_container_from_iterator >());
   test_splay_up(values, detail::bool_< has_splay< assoc_type >::value >());
   test_splay_down(values, detail::bool_< has_splay< assoc_type >::value >());
   test_rebalance(values, detail::bool_< has_rebalance< assoc_type >::value >());
   test_insert_before(values, detail::bool_< has_insert_before< assoc_type >::value >());
   test_insert_erase_burst();
   test_container_from_iterator(values, detail::bool_< assoc_type::has_container_from_iterator >());
}

template<class ValueTraits, template <class = void, class = void, class = void, class = void> class ContainerDefiner>
void test_generic_assoc<ValueTraits, ContainerDefiner>
   ::test_clone(value_cont_type& values)
{
   typedef ContainerDefiner
      < value_type
      , value_traits<ValueTraits>
      , constant_time_size<value_type::constant_time_size>
      > definer_function;
   typedef typename definer_function::type assoc_type;
   assoc_type testset1 (values.begin(), values.begin() + values.size());
   assoc_type testset2;

   testset2.clone_from(testset1, test::new_cloner<value_type>(), test::delete_disposer<value_type>());
   BOOST_TEST (testset2 == testset1);
   testset2.clear_and_dispose(test::delete_disposer<value_type>());
   BOOST_TEST (testset2.empty());
}

template<class ValueTraits, template <class = void, class = void, class = void, class = void> class ContainerDefiner>
void test_generic_assoc<ValueTraits, ContainerDefiner>
   ::test_container_from_end(value_cont_type& values, detail::true_type)
{
   typedef typename ValueTraits::value_type value_type;
   typedef ContainerDefiner
      < value_type
      , value_traits<ValueTraits>
      , constant_time_size<value_type::constant_time_size>
      > definer_function;
   typedef typename definer_function::type assoc_type;
   assoc_type testset (values.begin(), values.begin() + values.size());
   BOOST_TEST (testset == assoc_type::container_from_end_iterator(testset.end()));
   BOOST_TEST (testset == assoc_type::container_from_end_iterator(testset.cend()));
}

template<class ValueTraits, template <class = void, class = void, class = void, class = void> class ContainerDefiner>
void test_generic_assoc<ValueTraits, ContainerDefiner>::test_splay_up
(value_cont_type& values, detail::true_type)
{
   typedef typename ValueTraits::value_type value_type;
   typedef ContainerDefiner
      < value_type
      , value_traits<ValueTraits>
      , constant_time_size<value_type::constant_time_size>
      > definer_function;
   typedef typename definer_function::type assoc_type;

   typedef typename assoc_type::iterator iterator;
   typedef value_cont_type orig_set_t;
   std::size_t num_values;
   orig_set_t original_testset;
   {
      assoc_type testset (values.begin(), values.end());
      num_values = testset.size();
      original_testset = value_cont_type(testset.begin(), testset.end());
   }

   for(std::size_t i = 0; i != num_values; ++i){
      assoc_type testset (values.begin(), values.end());
      {
         iterator it = testset.begin();
         for(std::size_t j = 0; j != i; ++j, ++it){}
         testset.splay_up(it);
      }
      BOOST_TEST (testset.size() == num_values);
      iterator it = testset.begin();
      for( typename orig_set_t::const_iterator origit    = original_testset.begin()
         , origitend = original_testset.end()
         ; origit != origitend
         ; ++origit, ++it){
         BOOST_TEST(*origit == *it);
      }
   }
}

template<class ValueTraits, template <class = void, class = void, class = void, class = void> class ContainerDefiner>
void test_generic_assoc<ValueTraits, ContainerDefiner>::test_splay_down
(value_cont_type& values, detail::true_type)
{
   typedef typename ValueTraits::value_type value_type;
   typedef ContainerDefiner
      < value_type
      , value_traits<ValueTraits>
      , constant_time_size<value_type::constant_time_size>
      > definer_function;
   typedef typename definer_function::type assoc_type;

   typedef typename assoc_type::iterator iterator;
   typedef value_cont_type orig_set_t;
   std::size_t num_values;
   orig_set_t original_testset;
   {
      assoc_type testset (values.begin(), values.end());
      num_values = testset.size();
      original_testset = value_cont_type(testset.begin(), testset.end());
   }

   for(std::size_t i = 0; i != num_values; ++i){
      assoc_type testset (values.begin(), values.end());
      BOOST_TEST(testset.size() == num_values);
      {
         iterator it = testset.begin();
         for(std::size_t j = 0; j != i; ++j, ++it){}
         BOOST_TEST(*it == *testset.splay_down(*it));
      }
      BOOST_TEST (testset.size() == num_values);
      iterator it = testset.begin();
      for( typename orig_set_t::const_iterator origit    = original_testset.begin()
         , origitend = original_testset.end()
         ; origit != origitend
         ; ++origit, ++it){
         BOOST_TEST(*origit == *it);
      }
   }
}

template<class ValueTraits, template <class = void, class = void, class = void, class = void> class ContainerDefiner>
void test_generic_assoc<ValueTraits, ContainerDefiner>::test_rebalance
(value_cont_type& values, detail::true_type)
{
   typedef typename ValueTraits::value_type value_type;
   typedef ContainerDefiner
      < value_type
      , value_traits<ValueTraits>
      , constant_time_size<value_type::constant_time_size>
      > definer_function;
   typedef typename definer_function::type assoc_type;
   typedef value_cont_type orig_set_t;
   orig_set_t original_testset;
   {
      assoc_type testset (values.begin(), values.end());
      //original_testset.insert(original_testset.end(), testset.begin(), testset.end());
      original_testset = value_cont_type(testset.begin(), testset.end());
   }
   {
      assoc_type testset(values.begin(), values.end());
      testset.rebalance();
      TEST_INTRUSIVE_SEQUENCE_EXPECTED(original_testset, testset.begin());
   }

   {
      std::size_t numdata;
      {
         assoc_type testset(values.begin(), values.end());
         numdata = testset.size();
      }

      for(int i = 0; i != (int)numdata; ++i){
         assoc_type testset(values.begin(), values.end());
         typename assoc_type::iterator it = testset.begin();
         for(int j = 0; j  != i; ++j)  ++it;
         testset.rebalance_subtree(it);
         TEST_INTRUSIVE_SEQUENCE_EXPECTED(original_testset, testset.begin());
      }
   }
}

template<class ValueTraits, template <class = void, class = void, class = void, class = void> class ContainerDefiner>
void test_generic_assoc<ValueTraits, ContainerDefiner>::test_insert_before
(value_cont_type& values, detail::true_type)
{
   typedef typename ValueTraits::value_type value_type;
   typedef ContainerDefiner
      < value_type
      , value_traits<ValueTraits>
      , constant_time_size<value_type::constant_time_size>
      > definer_function;
   typedef typename definer_function::type assoc_type;
   {
      assoc_type testset;
      typedef typename value_cont_type::iterator vec_iterator;
      for(vec_iterator it(values.begin()), itend(values.end())
         ; it != itend
         ; ++it){
         testset.push_back(*it);
      }
      BOOST_TEST(testset.size() == values.size());
      TEST_INTRUSIVE_SEQUENCE_EXPECTED(values, testset.begin());
   }
   {
      assoc_type testset;
      typedef typename value_cont_type::iterator vec_iterator;

      for(vec_iterator it(--values.end()); true; --it){
         testset.push_front(*it);
       if(it == values.begin()){
            break;
       }
      }
      BOOST_TEST(testset.size() == values.size());
      TEST_INTRUSIVE_SEQUENCE_EXPECTED(values, testset.begin());
   }
   {
      assoc_type testset;
      typedef typename value_cont_type::iterator vec_iterator;
      typename assoc_type::iterator it_pos =
         testset.insert_before(testset.end(), *values.rbegin());
      testset.insert_before(testset.begin(), *values.begin());
      for(vec_iterator it(++values.begin()), itend(--values.end())
         ; it != itend
         ; ++it){
         testset.insert_before(it_pos, *it);
      }
      BOOST_TEST(testset.size() == values.size());
      TEST_INTRUSIVE_SEQUENCE_EXPECTED(values, testset.begin());
   }
}

}}}   //namespace boost::intrusive::test

#include <boost/intrusive/detail/config_end.hpp>
