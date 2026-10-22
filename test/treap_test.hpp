/////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Ion Gaztanaga 2026-2026
//
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/intrusive for documentation.
//
/////////////////////////////////////////////////////////////////////////////
#ifndef BOOST_INTRUSIVE_TEST_TREAP_TEST_HPP
#define BOOST_INTRUSIVE_TEST_TREAP_TEST_HPP

#include <boost/intrusive/detail/config_begin.hpp>
#include <boost/intrusive/options.hpp>
#include <boost/core/lightweight_test.hpp>
#include <boost/move/utility_core.hpp>
#include <boost/move/detail/meta_utils.hpp>   //declval
#include "common_functors.hpp"
#include "test_macros.hpp"
#include <cstddef>

namespace boost{
namespace intrusive{
namespace test{

//Tests of the treap containers (sets and multisets) that need their priority
template<class ContainerDefiner>
struct test_treap
{
   typedef typename ContainerDefiner::value_cont_type    value_cont_type;

   static void test_all()
   {
      value_cont_type values(6);
      for (std::size_t i = 0; i < 6u; ++i)
         (&values[i])->value_ = (int)i + 1;
      test_move_constructor_predicates(values);
   }

   //The move constructor also copies the priority comparison
   static void test_move_constructor_predicates(value_cont_type &values)
   {
      typedef typename ContainerDefiner::template container
         < priority<tracking_priority> >::type treap_type;
      typedef typename treap_type::size_type          size_type;
      typedef typename treap_type::key_compare        key_compare;

      treap_type src(values.begin(), values.end(), key_compare(), tracking_priority(8));
      const size_type size = src.size();
      predicate_counters::reset();
      treap_type dst(boost::move(src));
      BOOST_TEST(predicate_counters::moves == 0);
      BOOST_TEST(predicate_counters::swaps == 0);
      BOOST_TEST(src.priority_comp().tag_ == 8);
      BOOST_TEST(dst.priority_comp().tag_ == 8);
      BOOST_TEST(src.empty());
      BOOST_TEST(dst.size() == size);
      dst.check();
      //Move assignment: the priority comparison is copied too
      treap_type assigned(key_compare(), tracking_priority(3));
      predicate_counters::reset();
      assigned = boost::move(dst);
      BOOST_TEST(predicate_counters::moves == 0);
      BOOST_TEST(predicate_counters::swaps == 0);
      BOOST_TEST(assigned.priority_comp().tag_ == 8);
      BOOST_TEST(dst.priority_comp().tag_ == 8);
      BOOST_TEST(dst.empty());
      BOOST_TEST(assigned.size() == size);
      assigned.check();
      assigned.clear();
      src.insert(values.begin(), values.end());
      BOOST_TEST(src.size() == size);
      src.check();
      src.clear();

      #if !defined(BOOST_NO_CXX11_NOEXCEPT)
      using boost::move_detail::declval;
      BOOST_INTRUSIVE_STATIC_ASSERT(( !noexcept(treap_type(declval<treap_type>())) ));
      BOOST_INTRUSIVE_STATIC_ASSERT(( !noexcept(declval<treap_type&>() = declval<treap_type>()) ));
      #endif
   }
};

}  //namespace test{
}  //namespace intrusive{
}  //namespace boost{

#include <boost/intrusive/detail/config_end.hpp>

#endif   //BOOST_INTRUSIVE_TEST_TREAP_TEST_HPP
