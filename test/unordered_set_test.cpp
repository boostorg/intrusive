/////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Ion Gaztanaga  2006-2015.
//
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/intrusive for documentation.
//
/////////////////////////////////////////////////////////////////////////////
#include <boost/intrusive/unordered_set.hpp>
#include <boost/intrusive/pointer_traits.hpp>
#include <boost/intrusive/detail/iterator.hpp>
#include "itestvalue.hpp"
#include "smart_ptr.hpp"
#include "common_functors.hpp"
#include <vector>
#include <set>
#include <boost/core/lightweight_test.hpp>
#include "test_macros.hpp"
#include "test_container.hpp"
#include "unordered_test_common.hpp"
#include "unordered_test.hpp"
#include <boost/intrusive/hashtable.hpp>

using namespace boost::intrusive;

template < class ValueTraits, bool ConstantTimeSize, bool CacheBegin, bool CompareHash
         , bool Incremental, bool Map, bool DefaultHolder, bool LinearBuckets, bool FastMod>
struct rebinder
{
   typedef unordered_rebinder_common<ValueTraits, DefaultHolder, Map> common_t;
   typedef typename ValueContainer< typename ValueTraits::value_type >::type value_cont_type;

   template < class Option1 =void
            , class Option2 =void
            >
   struct container
   {
      typedef unordered_set
         < typename common_t::value_type
         , value_traits<ValueTraits>
         , constant_time_size<ConstantTimeSize>
         , cache_begin<CacheBegin>
         , compare_hash<CompareHash>
         , incremental<Incremental>
         , linear_buckets<LinearBuckets>
         , fastmod_buckets<FastMod>
         , typename common_t::holder_opt
         , typename common_t::key_of_value_opt
         , size_type<unsigned short>
         , Option1
         , Option2
         > type;
      BOOST_INTRUSIVE_STATIC_ASSERT((key_type_tester<typename common_t::key_of_value_opt, type>::value));
      BOOST_INTRUSIVE_STATIC_ASSERT((!boost::intrusive::test::is_multikey_true<type>::value));
   };
};

enum HookType
{
   Base,
   Member,
   NonMember
};

template<class VoidPointer, bool ConstantTimeSize, bool DefaultHolder, bool Map, HookType Type, bool LinearBuckets>
class test_main_template;

template<class VoidPointer, bool ConstantTimeSize, bool DefaultHolder, bool Map, bool LinearBuckets>
class test_main_template<VoidPointer, ConstantTimeSize, DefaultHolder, Map, Base, LinearBuckets>
{
   public:
   static void execute()
   {
      typedef testvalue<unordered_hooks<VoidPointer> > value_type;
      static const int random_init[6] = { 3, 2, 4, 1, 5, 2 };
      typedef typename ValueContainer< value_type >::type value_cont_type;
      value_cont_type data (6);
      for (std::size_t i = 0; i < 6u; ++i)
         data[i].value_ = random_init[i];

      typedef testvalue_traits< unordered_hooks<VoidPointer> > testval_traits_t;
      //base
      typedef typename detail::if_c
         < ConstantTimeSize
         , typename testval_traits_t::base_value_traits
         , typename testval_traits_t::auto_base_value_traits   //store_hash<true>
         >::type base_hook_t;
      test::test_unordered
         < //cache_begin, compare_hash, incremental
           rebinder< base_hook_t, ConstantTimeSize, ConstantTimeSize
                   , !ConstantTimeSize, !!ConstantTimeSize, Map, DefaultHolder
                   , LinearBuckets, !LinearBuckets && !ConstantTimeSize>
         >::test_all(data);
   }
};

template<class VoidPointer, bool ConstantTimeSize, bool DefaultHolder, bool Map, bool LinearBuckets>
class test_main_template<VoidPointer, ConstantTimeSize, DefaultHolder, Map, Member, LinearBuckets>
{
   public:
   static void execute()
   {
      typedef testvalue<unordered_hooks<VoidPointer> > value_type;
      static const int random_init[6] = { 3, 2, 4, 1, 5, 2 };
      typedef typename ValueContainer< value_type >::type value_cont_type;
      value_cont_type data (6);
      for (std::size_t i = 0; i < 6u; ++i)
         data[i].value_ = random_init[i];

      typedef testvalue_traits< unordered_hooks<VoidPointer> > testval_traits_t;
      //member
      typedef typename detail::if_c
         < ConstantTimeSize
         , typename testval_traits_t::member_value_traits      //optimize_multikey<true>
         , typename testval_traits_t::auto_member_value_traits //store_hash<true>, optimize_multikey<true>
         >::type member_hook_t;
      test::test_unordered
         < //cache_begin, compare_hash, incremental
           rebinder<member_hook_t, ConstantTimeSize, false, !ConstantTimeSize, false, Map, DefaultHolder
                  , LinearBuckets, !LinearBuckets && !ConstantTimeSize>
         >::test_all(data);
   }
};

template<class VoidPointer, bool ConstantTimeSize, bool DefaultHolder, bool Map, bool LinearBuckets>
class test_main_template<VoidPointer, ConstantTimeSize, DefaultHolder, Map, NonMember, LinearBuckets>

{
   public:
   static void execute()
   {
      typedef testvalue<unordered_hooks<VoidPointer> > value_type;
      static const int random_init[6] = { 3, 2, 4, 1, 5, 2 };
      typedef typename ValueContainer< value_type >::type value_cont_type;
      value_cont_type data (6);
      for (std::size_t i = 0; i < 6u; ++i)
         data[i].value_ = random_init[i];

      typedef testvalue_traits< unordered_hooks<VoidPointer> > testval_traits_t;
      //nonmember
      test::test_unordered
         < //cache_begin, compare_hash, incremental
           rebinder<typename testval_traits_t::nonhook_value_traits, ConstantTimeSize, false, false, false, Map, DefaultHolder
                  , LinearBuckets, !LinearBuckets && !ConstantTimeSize>
         >::test_all(data);
   }
};

//All the options of the unordered containers can be used at the same time
//(without variadic templates, they accept a limited number of options)
struct all_options_value
   : public unordered_set_base_hook< store_hash<true> >
{
   explicit all_options_value(int v) : v_(v) {}
   int v_;
};

struct all_options_key_of_value
{
   typedef int type;
   int operator()(const all_options_value &v) const {  return v.v_;  }
};

struct all_options_hash
{
   std::size_t operator()(int i) const {  return std::size_t(i);  }
};

struct all_options_equal
{
   bool operator()(int a, int b) const {  return a == b;  }
};

template<class Container>
void test_all_options_container()
{
   typedef typename Container::bucket_type   bucket_type;
   typedef typename Container::bucket_traits bucket_traits;
   bucket_type buckets[8];
   Container c(bucket_traits(buckets, 8u));
   all_options_value v1(1), v2(2);
   c.insert_equal(v1);
   c.insert_equal(v2);
   BOOST_TEST(c.size() == 2u);
   BOOST_TEST(c.find(2) != c.end());
   c.clear();
}

void test_all_options()
{
   typedef unordered_set_base_hook< store_hash<true> > hook_t;
   typedef unordered_default_bucket_traits< base_hook<hook_t> >::type bucket_traits_t;
   //13 options: value traits (hook), size_type, key_of_value, hash, equal, bucket traits
   //and the 7 flags
   #define BOOST_INTRUSIVE_TEST_ALL_UNORDERED_OPTIONS \
        base_hook<hook_t>, size_type<std::size_t>, key_of_value<all_options_key_of_value> \
      , hash<all_options_hash>, equal<all_options_equal>, bucket_traits<bucket_traits_t> \
      , constant_time_size<true>, power_2_buckets<true>, cache_begin<true>, compare_hash<true> \
      , incremental<true>, linear_buckets<false>, fastmod_buckets<false>
   test_all_options_container< hashtable<all_options_value, BOOST_INTRUSIVE_TEST_ALL_UNORDERED_OPTIONS> >();
   test_all_options_container< unordered_multiset<all_options_value, BOOST_INTRUSIVE_TEST_ALL_UNORDERED_OPTIONS> >();
   {
      typedef unordered_set<all_options_value, BOOST_INTRUSIVE_TEST_ALL_UNORDERED_OPTIONS> uset_t;
      uset_t::bucket_type buckets[8];
      uset_t c(uset_t::bucket_traits(buckets, 8u));
      all_options_value v1(1);
      BOOST_TEST(c.insert(v1).second);
      BOOST_TEST(c.find(1) != c.end());
      c.clear();
   }
   #undef BOOST_INTRUSIVE_TEST_ALL_UNORDERED_OPTIONS
}

int main()
{
   test_all_options();
   //Combinations: VoidPointer x ConstantTimeSize x DefaultHolder x Map x HookType x LinearBuckets
   //Minimize them selecting different combinations for raw and smart pointers
   //Base and Member hooks use auto_unlink if !ConstantTimeSize, and auto_unlink
   //hooks are not compatible with linear buckets.

   //void pointer
   test_main_template<void*, false, false, false, Base, false>::execute();
   test_main_template<void*,  true,  true,  true, Base, true>::execute();
   test_main_template<void*, false,  true,  true, Member, false>::execute();
   test_main_template<void*,  true, false, false, NonMember, true>::execute();

   //smart_ptr
   test_main_template<smart_ptr<void>,  true, false,  true, Member, true>::execute();
   test_main_template<smart_ptr<void>, false,  true, false, Base, false>::execute();

   //bounded_ptr is not supported

   return boost::report_errors();
}
