/////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Ion Gaztanaga  2015-2015.
//
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/intrusive for documentation.
//
/////////////////////////////////////////////////////////////////////////////
#include <boost/intrusive/pointer_traits.hpp>
#include <boost/intrusive/detail/iterator.hpp>
#include "common_functors.hpp"
#include <vector>
#include <algorithm> //std::sort
#include <set>
#include <boost/core/lightweight_test.hpp>

#include "test_macros.hpp"
#include "test_container.hpp"
#include "unordered_test_common.hpp"

namespace boost{
namespace intrusive{
namespace test{

static const std::size_t BucketSize = 8;

//Bucket counts used in tests. fastmod_buckets only supports
//bucket counts from its prime table, so use values from that table.
template<class UnorderedType, bool FastMod = UnorderedType::fastmod_buckets>
struct bucket_sizes
{
   static const std::size_t Single = 1u;
   static const std::size_t Small  = BucketSize/4u;
   static const std::size_t Normal = BucketSize;
   static const std::size_t Big    = BucketSize*2u;
};

template<class UnorderedType>
struct bucket_sizes<UnorderedType, true>
{
   static const std::size_t Single = 3u;
   static const std::size_t Small  = 3u;
   static const std::size_t Normal = 7u;
   static const std::size_t Big    = 17u;
};

//Hash function with a modifiable seed, used to test full_rehash
template<class Dummy = void>
struct seeded_hash_t
{
   static std::size_t seed;

   template<class T>
   std::size_t operator()(const T &t) const
   {  return hash_value(t) ^ seed;  }
};

template<class Dummy>
std::size_t seeded_hash_t<Dummy>::seed = 0u;

typedef seeded_hash_t<> seeded_hash;

//Hash function that stores all the elements in the same bucket
struct zero_hash
{
   template<class T>
   std::size_t operator()(const T &) const
   {  return 0u;  }
};

template<class ContainerDefiner>
struct test_unordered
{
   typedef typename ContainerDefiner::value_cont_type value_cont_type;

   static void test_all(value_cont_type& values);
   private:
   static void test_sort(value_cont_type& values);
   static void test_insert(value_cont_type& values, detail::true_);
   static void test_insert(value_cont_type& values, detail::false_);
   static void test_swap(value_cont_type& values);
   static void test_rehash(value_cont_type& values, detail::true_);
   static void test_rehash(value_cont_type& values, detail::false_);
   template<class Set>
   static void check_begin(Set &testset, std::size_t expected_size);
   static void test_begin_after_rehash(detail::true_);
   static void test_begin_after_rehash(detail::false_);
   template<bool FullRehash>
   static void test_rehash_groups(detail::true_);
   static void test_rehash_groups(detail::true_);
   static void test_rehash_groups(detail::false_);
   static void test_erase_range_in_group(detail::true_);
   static void test_erase_range_in_group(detail::false_);
   static void test_find(value_cont_type& values);
   static bool test_equal(const int *xv, std::size_t xn, const int *yv, std::size_t yn);
   static void test_equal();
   static void test_impl();
   static void test_clone(value_cont_type& values);
   template<class Option>
   static void test_clone_functors(value_cont_type& values);
   static void test_moved_from(value_cont_type& values);
};

template<class ContainerDefiner>
void test_unordered<ContainerDefiner>::test_all (value_cont_type& values)
{
   typedef typename ContainerDefiner::template container
      <>::type unordered_type;
   const std::size_t ExtraBuckets = unordered_type::bucket_overhead;
   typedef bucket_sizes<unordered_type> sizes;

   typedef typename unordered_type::bucket_traits bucket_traits;
   typedef typename unordered_type::bucket_ptr    bucket_ptr;

   {
      typename unordered_type::bucket_type buckets [sizes::Normal + ExtraBuckets];
      unordered_type testset
         (bucket_traits(pointer_traits<bucket_ptr>::pointer_to(buckets[0]), sizeof(buckets)/sizeof(*buckets)));
      testset.insert(values.begin(), values.end());
      test::test_container(testset);
      testset.clear();
      testset.insert(values.begin(), values.end());
      test::test_common_unordered_and_associative_container(testset, values);
      testset.clear();
      testset.insert(values.begin(), values.end());
      test::test_unordered_associative_container(testset, values);
      testset.clear();
      testset.insert(values.begin(), values.end());
      typedef detail::bool_<boost::intrusive::test::is_multikey_true
         <unordered_type>::value> select_t;
      test::test_maybe_unique_container(testset, values, select_t());
   }
   {
      value_cont_type vals(sizes::Normal);
      for (std::size_t i = 0; i < sizes::Normal; ++i)
         (&vals[i])->value_ = (int)i;
      typename unordered_type::bucket_type buckets[sizes::Normal + ExtraBuckets];
      unordered_type testset(bucket_traits(
         pointer_traits<bucket_ptr>::pointer_to(buckets[0]), sizeof(buckets)/sizeof(*buckets)));
      testset.insert(vals.begin(), vals.end());
      test::test_iterator_forward(testset);
   }
   test_sort(values);
   test_insert(values, detail::bool_<boost::intrusive::test::is_multikey_true<unordered_type>::value>());
   test_swap(values);
   test_rehash(values, detail::bool_<unordered_type::incremental>());
   test_begin_after_rehash(detail::bool_<unordered_type::incremental>());
   test_rehash_groups(detail::bool_<boost::intrusive::test::is_multikey_true<unordered_type>::value>());
   test_erase_range_in_group(detail::bool_<boost::intrusive::test::is_multikey_true<unordered_type>::value>());
   test_find(values);
   test_equal();
   test_impl();
   test_clone(values);
   //Only one option can be added to the container in C++03 compilers
   test_clone_functors<hash<instance_seed_hash> >(values);
   test_clone_functors<equal<tagged_equal> >(values);
   test_moved_from(values);
}

//test case due to an error in tree implementation:
template<class ContainerDefiner>
void test_unordered<ContainerDefiner>::test_impl()
{
   typedef typename ContainerDefiner::template container
      <>::type unordered_type;
   const std::size_t ExtraBuckets = unordered_type::bucket_overhead;
   typedef bucket_sizes<unordered_type> sizes;

   typedef typename unordered_type::bucket_traits bucket_traits;
   typedef typename unordered_type::bucket_ptr    bucket_ptr;

   value_cont_type values (5);
   for (std::size_t i = 0u; i < 5u; ++i)
      values[i].value_ = (int)i;

   typename unordered_type::bucket_type buckets[sizes::Normal + ExtraBuckets];
   unordered_type testset(bucket_traits(
      pointer_traits<bucket_ptr>::pointer_to(buckets[0]), sizeof(buckets)/sizeof(*buckets)));

   for (std::size_t i = 0u; i < 5u; ++i)
      testset.insert (values[i]);

   testset.erase (testset.iterator_to (values[0]));
   testset.erase (testset.iterator_to (values[1]));
   testset.insert (values[1]);

   testset.erase (testset.iterator_to (values[2]));
   testset.erase (testset.iterator_to (values[3]));
}

//test: constructor, iterator, clear, reverse_iterator, front, back, size:
template<class ContainerDefiner>
void test_unordered<ContainerDefiner>::test_sort(value_cont_type& values)
{
   typedef typename ContainerDefiner::template container
      <>::type unordered_type;
   const std::size_t ExtraBuckets = unordered_type::bucket_overhead;
   typedef bucket_sizes<unordered_type> sizes;

   typedef typename unordered_type::bucket_traits bucket_traits;
   typedef typename unordered_type::bucket_ptr    bucket_ptr;

   typename unordered_type::bucket_type buckets[sizes::Normal + ExtraBuckets];
   unordered_type testset1
      (values.begin(), values.end(), bucket_traits
         (pointer_traits<bucket_ptr>::pointer_to(buckets[0]), sizeof(buckets)/sizeof(*buckets)));

   if(unordered_type::incremental){
      {  int init_values [] = { 4, 5, 1, 2, 2, 3 };
         TEST_INTRUSIVE_SEQUENCE_MAYBEUNIQUE( init_values, testset1 );  }
   }
   else{
      {  int init_values [] = { 1, 2, 2, 3, 4, 5 };
         TEST_INTRUSIVE_SEQUENCE_MAYBEUNIQUE( init_values, testset1 );  }
   }
   testset1.clear();
   BOOST_TEST (testset1.empty());
}

//test: insert, const_iterator, const_reverse_iterator, erase, iterator_to:
template<class ContainerDefiner>
void test_unordered<ContainerDefiner>::test_insert(value_cont_type& values, detail::false_) //not multikey
{
   typedef typename ContainerDefiner::template container
      <>::type unordered_set_type;
   typedef typename unordered_set_type::bucket_traits bucket_traits;
   typedef typename unordered_set_type::key_of_value  key_of_value;
   typedef typename unordered_set_type::bucket_ptr bucket_ptr;

   const std::size_t ExtraBuckets = unordered_set_type::bucket_overhead;
   typedef bucket_sizes<unordered_set_type> sizes;
   typename unordered_set_type::bucket_type buckets[sizes::Normal + ExtraBuckets];
   const bucket_traits orig_bucket_traits( pointer_traits<bucket_ptr>::pointer_to(buckets[0])
                                         , sizeof(buckets) / sizeof(*buckets));
   unordered_set_type testset(orig_bucket_traits);
   testset.insert(&values[0] + 2, &values[0] + 5);

   typename unordered_set_type::insert_commit_data commit_data;
   BOOST_TEST ((!testset.insert_check(key_of_value()(values[2]), commit_data).second));
   BOOST_TEST (( testset.insert_check(key_of_value()(values[0]), commit_data).second));

   //Test insert_fast_commit
   {
      BOOST_TEST(testset.find(key_of_value()(values[0])) == testset.end());
      testset.insert_fast_commit(values[0], commit_data);
      BOOST_TEST(testset.find(key_of_value()(values[0])) != testset.end());
      testset.erase(key_of_value()(values[0]));
      BOOST_TEST(testset.find(key_of_value()(values[0])) == testset.end());
   }

   //Test insert_commit
   BOOST_IF_CONSTEXPR(!unordered_set_type::incremental)
   {
      BOOST_TEST((testset.insert_check(key_of_value()(values[0]), commit_data).second));
      typename unordered_set_type::bucket_type buckets2[sizes::Small + ExtraBuckets];
      //Two rehashes to be compatible with incremental hashing
      testset.rehash(bucket_traits(
         pointer_traits<bucket_ptr>::pointer_to(buckets2[0]), sizes::Small + ExtraBuckets));
      testset.insert_commit(values[0], commit_data);
      BOOST_TEST(testset.find(key_of_value()(values[0])) != testset.end());
      testset.erase(key_of_value()(values[0]));
      BOOST_TEST(testset.find(key_of_value()(values[0])) == testset.end());
      //Two rehashes to be compatible with incremental hashing
      testset.clear();
      testset.rehash(orig_bucket_traits);
      testset.insert(&values[0] + 2, &values[0] + 5);
   }

   const unordered_set_type& const_testset = testset;
   BOOST_IF_CONSTEXPR(unordered_set_type::incremental)
   {
      {  int init_values [] = { 4, 5, 1 };
         TEST_INTRUSIVE_SEQUENCE_MAYBEUNIQUE( init_values, const_testset );  }
      typename unordered_set_type::iterator i = testset.begin();
      BOOST_TEST (i->value_ == 4);

      i = testset.insert(values[0]).first;
      BOOST_TEST (&*i == &values[0]);

      i = testset.iterator_to (values[2]);
      BOOST_TEST (&*i == &values[2]);

      testset.erase (i);

      {  int init_values [] = { 5, 1, 3 };
         TEST_INTRUSIVE_SEQUENCE_MAYBEUNIQUE( init_values, const_testset );  }
   }
   else{
      {  int init_values [] = { 1, 4, 5 };
         TEST_INTRUSIVE_SEQUENCE_MAYBEUNIQUE( init_values, const_testset );  }
      typename unordered_set_type::iterator i = testset.begin();
      BOOST_TEST (i->value_ == 1);

      i = testset.insert(values[0]).first;
      BOOST_TEST (&*i == &values[0]);

      i = testset.iterator_to (values[2]);
      BOOST_TEST (&*i == &values[2]);

      testset.erase (i);

      {  int init_values [] = { 1, 3, 5 };
         TEST_INTRUSIVE_SEQUENCE_MAYBEUNIQUE( init_values, const_testset );  }
   }
}

template<class ContainerDefiner>
void test_unordered<ContainerDefiner>::test_insert(value_cont_type& values, detail::true_) //is multikey
{
   typedef typename ContainerDefiner::template container
      <>::type unordered_type;
   const std::size_t ExtraBuckets = unordered_type::bucket_overhead;
   typedef bucket_sizes<unordered_type> sizes;

   typedef typename unordered_type::bucket_traits bucket_traits;
   typedef typename unordered_type::bucket_ptr bucket_ptr;
   typedef typename unordered_type::iterator iterator;
   typedef typename unordered_type::key_type key_type;
   {
      typename unordered_type::bucket_type buckets[sizes::Normal + ExtraBuckets];
      unordered_type testset(bucket_traits(
         pointer_traits<bucket_ptr>::pointer_to(buckets[0]), sizeof(buckets)/sizeof(*buckets)));

      testset.insert(&values[0] + 2, &values[0] + 5);

      const unordered_type& const_testset = testset;

      if(unordered_type::incremental){
         {
            {  int init_values [] = { 4, 5, 1 };
               TEST_INTRUSIVE_SEQUENCE_MAYBEUNIQUE( init_values, const_testset );  }

            typename unordered_type::iterator i = testset.begin();
            BOOST_TEST (i->value_ == 4);

            i = testset.insert (values[0]);
            BOOST_TEST (&*i == &values[0]);

            i = testset.iterator_to (values[2]);
            BOOST_TEST (&*i == &values[2]);
            testset.erase(i);

            {  int init_values [] = { 5, 1, 3 };
               TEST_INTRUSIVE_SEQUENCE_MAYBEUNIQUE( init_values, const_testset );  }
            testset.clear();
            testset.insert(&values[0], &values[0] + values.size());

            {  int init_values [] = { 4, 5, 1, 2, 2, 3 };
               TEST_INTRUSIVE_SEQUENCE_MAYBEUNIQUE( init_values, const_testset );  }

            BOOST_TEST (testset.erase(key_type(1)) == 1);
            BOOST_TEST (testset.erase(key_type(2)) == 2);
            BOOST_TEST (testset.erase(key_type(3)) == 1);
            BOOST_TEST (testset.erase(key_type(4)) == 1);
            BOOST_TEST (testset.erase(key_type(5)) == 1);
            BOOST_TEST (testset.empty() == true);

            //Now with a single bucket
            typename unordered_type::bucket_type single_bucket[sizes::Single + ExtraBuckets];
            unordered_type testset2(bucket_traits(
               pointer_traits<bucket_ptr>::pointer_to(single_bucket[0]), sizeof(single_bucket)/sizeof(*single_bucket)));
            testset2.insert(&values[0], &values[0] + values.size());
            BOOST_TEST (testset2.erase(key_type(5)) == 1);
            BOOST_TEST (testset2.erase(key_type(2)) == 2);
            BOOST_TEST (testset2.erase(key_type(1)) == 1);
            BOOST_TEST (testset2.erase(key_type(4)) == 1);
            BOOST_TEST (testset2.erase(key_type(3)) == 1);
            BOOST_TEST (testset2.empty() == true);
         }
      }
      else{
         {
            {  int init_values [] = { 1, 4, 5 };
               TEST_INTRUSIVE_SEQUENCE_MAYBEUNIQUE( init_values, const_testset );  }

            typename unordered_type::iterator i = testset.begin();
            BOOST_TEST (i->value_ == 1);

            i = testset.insert (values[0]);
            BOOST_TEST (&*i == &values[0]);

            i = testset.iterator_to (values[2]);
            BOOST_TEST (&*i == &values[2]);
            testset.erase(i);

            {  int init_values [] = { 1, 3, 5 };
               TEST_INTRUSIVE_SEQUENCE_MAYBEUNIQUE( init_values, const_testset );  }
            testset.clear();
            testset.insert(&values[0], &values[0] + values.size());

            {  int init_values [] = { 1, 2, 2, 3, 4, 5 };
               TEST_INTRUSIVE_SEQUENCE_MAYBEUNIQUE( init_values, const_testset );  }

            BOOST_TEST (testset.erase(key_type(1)) == 1);
            BOOST_TEST (testset.erase(key_type(2)) == 2);
            BOOST_TEST (testset.erase(key_type(3)) == 1);
            BOOST_TEST (testset.erase(key_type(4)) == 1);
            BOOST_TEST (testset.erase(key_type(5)) == 1);
            BOOST_TEST (testset.empty() == true);

            //Now with a single bucket
            typename unordered_type::bucket_type single_bucket[sizes::Single + ExtraBuckets];
            unordered_type testset2(bucket_traits(
               pointer_traits<bucket_ptr>::pointer_to(single_bucket[0]), sizeof(single_bucket)/sizeof(*single_bucket)));
            testset2.insert(&values[0], &values[0] + values.size());
            BOOST_TEST (testset2.erase(key_type(5)) == 1);
            BOOST_TEST (testset2.erase(key_type(2)) == 2);
            BOOST_TEST (testset2.erase(key_type(1)) == 1);
            BOOST_TEST (testset2.erase(key_type(4)) == 1);
            BOOST_TEST (testset2.erase(key_type(3)) == 1);
            BOOST_TEST (testset2.empty() == true);
         }
      }
      {
         //Now erase just one per loop
         const int random_init[] = { 3, 2, 4, 1, 5, 2, 2 };
         const std::size_t random_size = sizeof(random_init)/sizeof(random_init[0]);
         typename unordered_type::bucket_type single_bucket[sizes::Single + ExtraBuckets];
         for(std::size_t i = 0u, max = random_size; i != max; ++i){
            value_cont_type data (random_size);
            for (std::size_t j = 0; j < random_size; ++j)
               data[j].value_ = random_init[j];
            unordered_type testset_new(bucket_traits(
               pointer_traits<bucket_ptr>::pointer_to(single_bucket[0]), sizeof(single_bucket)/sizeof(*single_bucket)));
            testset_new.insert(&data[0], &data[0]+max);
            testset_new.erase(testset_new.iterator_to(data[i]));
            BOOST_TEST (testset_new.size() == (max -1));
         }
      }
   }
   {
      const std::size_t LoadFactor    = 3;
      const std::size_t NumIterations = sizes::Normal*LoadFactor;
      value_cont_type random_init(NumIterations);//Preserve memory

      //Initialize values
      for (std::size_t i = 0u; i < NumIterations; ++i){
         random_init[i].value_ = (int)i*2;
      }

      typename unordered_type::bucket_type buckets[sizes::Normal + ExtraBuckets];
      bucket_traits btraits(pointer_traits<bucket_ptr>::pointer_to(buckets[0]), sizeof(buckets)/sizeof(*buckets));

      for(std::size_t initial_pos = 0; initial_pos != (NumIterations+1u); ++initial_pos){
         for(std::size_t final_pos = initial_pos; final_pos != (NumIterations+1); ++final_pos){

            //Create intrusive container inserting values
            unordered_type testset
               ( random_init.data()
               , random_init.data() + random_init.size()
               , btraits);

            BOOST_TEST (testset.size() == random_init.size());

            //Obtain the iterator range to erase
            iterator it_beg_pos = testset.begin();
            for(std::size_t it_beg_pos_num = 0; it_beg_pos_num != initial_pos; ++it_beg_pos_num){
               ++it_beg_pos;
            }
            iterator it_end_pos(it_beg_pos);
            for(std::size_t it_end_pos_num = 0; it_end_pos_num != (final_pos - initial_pos); ++it_end_pos_num){
               ++it_end_pos;
            }

            //Erase the same values in both the intrusive and original vector
            std::size_t erased_cnt = boost::intrusive::iterator_udistance(it_beg_pos, it_end_pos);

            //Erase values from the intrusive container
            testset.erase(it_beg_pos, it_end_pos);

            BOOST_TEST (testset.size() == (random_init.size()-(final_pos - initial_pos)));

            //Now test...
            BOOST_TEST ((random_init.size() - erased_cnt) == testset.size());

            //for non-linear buckets is_linked is a reliable marker for a node
            //inserted in a hash map, but not for linear buckets, which are null-ended
            BOOST_IF_CONSTEXPR(!unordered_type::linear_buckets){
               value_cont_type set_tester;
               set_tester.reserve(NumIterations);
               //Create an ordered copy of the intrusive container
               set_tester.insert(set_tester.end(), testset.begin(), testset.end());
               std::sort(set_tester.begin(), set_tester.end());
               {
                  typename value_cont_type::iterator it = set_tester.begin(), itend = set_tester.end();
                  typename value_cont_type::iterator random_init_it(random_init.begin());
                  for( ; it != itend; ++it){
                     while(!random_init_it->is_linked())
                        ++random_init_it;
                     BOOST_TEST(*it == *random_init_it);
                     ++random_init_it;
                  }
               }
            }
         }
      }
   }
}

//test: insert (seq-version), swap, erase (seq-version), size:
template<class ContainerDefiner>
void test_unordered<ContainerDefiner>::test_swap(value_cont_type& values)
{
   typedef typename ContainerDefiner::template container
      <>::type unordered_type;
   const std::size_t ExtraBuckets = unordered_type::bucket_overhead;
   typedef bucket_sizes<unordered_type> sizes;

   typedef typename unordered_type::bucket_traits bucket_traits;
   typedef typename unordered_type::bucket_ptr    bucket_ptr;
   typename unordered_type::bucket_type buckets[sizes::Normal + ExtraBuckets];

   typename unordered_type::bucket_type buckets2[sizes::Normal + ExtraBuckets];
   unordered_type testset1(&values[0], &values[0] + 2,
      bucket_traits(pointer_traits<bucket_ptr>::pointer_to(buckets[0]), sizeof(buckets)/sizeof(*buckets)));
   unordered_type testset2(bucket_traits(
      pointer_traits<bucket_ptr>::pointer_to(buckets2[0]), sizeof(buckets2)/sizeof(*buckets2)));

   testset2.insert (&values[0] + 2, &values[0] + 6);
   testset1.swap (testset2);

   if(unordered_type::incremental){
      {  int init_values [] = { 4, 5, 1, 2 };
         TEST_INTRUSIVE_SEQUENCE_MAYBEUNIQUE( init_values, testset1 );  }

      {  int init_values [] = { 2, 3 };
         TEST_INTRUSIVE_SEQUENCE_MAYBEUNIQUE( init_values, testset2 );  }
      testset1.erase (testset1.iterator_to(values[4]), testset1.end());
      BOOST_TEST (testset1.size() == 1);
      //  BOOST_TEST (&testset1.front() == &values[3]);
      BOOST_TEST (&*testset1.begin() == &values[2]);
   }
   else{
      {  int init_values [] = { 1, 2, 4, 5 };
         TEST_INTRUSIVE_SEQUENCE_MAYBEUNIQUE( init_values, testset1 );  }

      {  int init_values [] = { 2, 3 };
         TEST_INTRUSIVE_SEQUENCE_MAYBEUNIQUE( init_values, testset2 );  }
      testset1.erase (testset1.iterator_to(values[5]), testset1.end());
      BOOST_TEST (testset1.size() == 1);
      //  BOOST_TEST (&testset1.front() == &values[3]);
      BOOST_TEST (&*testset1.begin() == &values[3]);
   }
}



//test: rehash:

template<class ContainerDefiner>
void test_unordered<ContainerDefiner>::test_rehash(value_cont_type& values, detail::true_)
{
   typedef typename ContainerDefiner::template container
      <>::type unordered_type;

   const std::size_t ExtraBuckets = unordered_type::bucket_overhead;
   typedef bucket_sizes<unordered_type> sizes;

   typedef typename unordered_type::bucket_traits bucket_traits;
   typedef typename unordered_type::bucket_ptr bucket_ptr;
   //Build a uset
   typename unordered_type::bucket_type buckets1[sizes::Normal + ExtraBuckets];
   typename unordered_type::bucket_type buckets2[sizes::Big + ExtraBuckets];
   unordered_type testset1(&values[0], &values[0] + values.size(),
      bucket_traits(pointer_traits<bucket_ptr>::
         pointer_to(buckets1[0]), sizeof(buckets1)/sizeof(*buckets1)));
   //Test current state
   BOOST_TEST(testset1.split_count() == BucketSize/2);
   {  int init_values [] = { 4, 5, 1, 2, 2, 3 };
      TEST_INTRUSIVE_SEQUENCE_MAYBEUNIQUE( init_values, testset1 );  }
   //Incremental rehash step
   BOOST_TEST (testset1.incremental_rehash() == true);
   BOOST_TEST(testset1.split_count() == (BucketSize/2+1));
   {  int init_values [] = { 5, 1, 2, 2, 3, 4 };
      TEST_INTRUSIVE_SEQUENCE_MAYBEUNIQUE( init_values, testset1 );  }
   //Rest of incremental rehashes should lead to the same sequence
   for(std::size_t split_bucket = testset1.split_count(); split_bucket != BucketSize; ++split_bucket){
      BOOST_TEST (testset1.incremental_rehash() == true);
      BOOST_TEST(testset1.split_count() == (split_bucket+1));
      {  int init_values [] = { 1, 2, 2, 3, 4, 5 };
      TEST_INTRUSIVE_SEQUENCE_MAYBEUNIQUE( init_values, testset1 );  }
   }
   //This incremental rehash should fail because we've reached the end of the bucket array
   BOOST_TEST (testset1.incremental_rehash() == false);
   BOOST_TEST(testset1.split_count() == BucketSize);
   {  int init_values [] = { 1, 2, 2, 3, 4, 5 };
   TEST_INTRUSIVE_SEQUENCE_MAYBEUNIQUE( init_values, testset1 );  }

   //
   //Try incremental hashing specifying a new bucket traits pointing to the same array
   //
   //This incremental rehash should fail because the new size is not twice the original
   BOOST_TEST(testset1.incremental_rehash(bucket_traits(
      pointer_traits<bucket_ptr>::pointer_to(buckets1[0])
      , sizeof(buckets1)/sizeof(*buckets1))) == false);
   BOOST_TEST(testset1.split_count() == BucketSize);
   {  int init_values [] = { 1, 2, 2, 3, 4, 5 };
   TEST_INTRUSIVE_SEQUENCE_MAYBEUNIQUE( init_values, testset1 );  }

   //
   //Try incremental hashing specifying a new bucket traits pointing to the same array
   //
   //This incremental rehash should fail because the new size is not twice the original
   BOOST_TEST(testset1.incremental_rehash(bucket_traits(
      pointer_traits<bucket_ptr>::
               pointer_to(buckets2[0])
               , BucketSize + ExtraBuckets)) == false);
   BOOST_TEST(testset1.split_count() == BucketSize);
   {  int init_values [] = { 1, 2, 2, 3, 4, 5 };
   TEST_INTRUSIVE_SEQUENCE_MAYBEUNIQUE( init_values, testset1 );  }

   //This incremental rehash should success because the new size is twice the original
   //and split_count is the same as the old bucket count
   BOOST_TEST(testset1.incremental_rehash(bucket_traits(
      pointer_traits<bucket_ptr>::
                     pointer_to(buckets2[0])
                     , sizeof(buckets2)/sizeof(*buckets2))) == true);
   BOOST_TEST(testset1.split_count() == BucketSize);
   {  int init_values [] = { 1, 2, 2, 3, 4, 5 };
   TEST_INTRUSIVE_SEQUENCE_MAYBEUNIQUE( init_values, testset1 );  }

   //This incremental rehash should also success because the new size is half the original
   //and split_count is the same as the new bucket count
   BOOST_TEST(testset1.incremental_rehash(bucket_traits(
      pointer_traits<bucket_ptr>::
                           pointer_to(buckets1[0])
                           , sizeof(buckets1)/sizeof(*buckets1))) == true);
   BOOST_TEST(testset1.split_count() == BucketSize);
   {  int init_values [] = { 1, 2, 2, 3, 4, 5 };
   TEST_INTRUSIVE_SEQUENCE_MAYBEUNIQUE( init_values, testset1 );  }

   //Shrink rehash
   testset1.rehash(bucket_traits(
      pointer_traits<bucket_ptr>::
         pointer_to(buckets1[0])
         , (sizeof(buckets1) / sizeof(*buckets1)- ExtraBuckets) / 2u + ExtraBuckets));
   BOOST_TEST (testset1.incremental_rehash() == false);
   {  int init_values [] = { 4, 5, 1, 2, 2, 3 };
      TEST_INTRUSIVE_SEQUENCE_MAYBEUNIQUE( init_values, testset1 );  }

   //Shrink rehash again
   testset1.rehash(bucket_traits(
      pointer_traits<bucket_ptr>::
         pointer_to(buckets1[0])
         , (sizeof(buckets1) / sizeof(*buckets1) - ExtraBuckets) / 4u + ExtraBuckets));
   BOOST_TEST (testset1.incremental_rehash() == false);
   {  int init_values [] = { 2, 2, 4, 3, 5, 1 };
      TEST_INTRUSIVE_SEQUENCE_MAYBEUNIQUE( init_values, testset1 );  }

   //Growing rehash
   testset1.rehash(bucket_traits(
      pointer_traits<bucket_ptr>::
         pointer_to(buckets1[0])
         , sizeof(buckets1)/sizeof(*buckets1)));

   {  int init_values [] = { 1, 2, 2, 3, 4, 5 };
      TEST_INTRUSIVE_SEQUENCE_MAYBEUNIQUE( init_values, testset1 );  }

   //Full rehash (no effects)
   testset1.full_rehash();
   {  int init_values [] = { 1, 2, 2, 3, 4, 5 };
      TEST_INTRUSIVE_SEQUENCE_MAYBEUNIQUE( init_values, testset1 );  }

   //Incremental rehash shrinking
   //First incremental rehashes should lead to the same sequence
   for(std::size_t split_bucket = testset1.split_count(); split_bucket > 6; --split_bucket){
      BOOST_TEST (testset1.incremental_rehash(false) == true);
      BOOST_TEST(testset1.split_count() == (split_bucket-1));
      {  int init_values [] = { 1, 2, 2, 3, 4, 5 };
      TEST_INTRUSIVE_SEQUENCE_MAYBEUNIQUE( init_values, testset1 );  }
   }

   //Incremental rehash step
   BOOST_TEST (testset1.incremental_rehash(false) == true);
   BOOST_TEST(testset1.split_count() == (BucketSize/2+1));
   {  int init_values [] = { 5, 1, 2, 2, 3, 4 };
      TEST_INTRUSIVE_SEQUENCE_MAYBEUNIQUE( init_values, testset1 );  }

   //Incremental rehash step 2
   BOOST_TEST (testset1.incremental_rehash(false) == true);
   BOOST_TEST(testset1.split_count() == (BucketSize/2));
   {  int init_values [] = { 4, 5, 1, 2, 2, 3 };
      TEST_INTRUSIVE_SEQUENCE_MAYBEUNIQUE( init_values, testset1 );  }

   //This incremental rehash should fail because we've reached the half of the bucket array
   BOOST_TEST(testset1.incremental_rehash(false) == false);
   BOOST_TEST(testset1.split_count() == BucketSize/2);
   {  int init_values [] = { 4, 5, 1, 2, 2, 3 };
   TEST_INTRUSIVE_SEQUENCE_MAYBEUNIQUE( init_values, testset1 );  }
}
template<class ContainerDefiner>
void test_unordered<ContainerDefiner>::test_rehash(value_cont_type& values, detail::false_)
{
   typedef typename ContainerDefiner::template container
      <>::type unordered_type;
   const std::size_t ExtraBuckets = unordered_type::bucket_overhead;
   typedef bucket_sizes<unordered_type> sizes;

   typedef typename unordered_type::bucket_traits bucket_traits;
   typedef typename unordered_type::bucket_ptr    bucket_ptr;

   typename unordered_type::bucket_type buckets1[sizes::Normal + ExtraBuckets];
   typename unordered_type::bucket_type buckets2 [sizes::Small + ExtraBuckets];
   typename unordered_type::bucket_type buckets3[sizes::Big + ExtraBuckets];

   unordered_type testset1(&values[0], &values[0] + 6, bucket_traits(
      pointer_traits<bucket_ptr>::
         pointer_to(buckets1[0]), sizeof(buckets1)/sizeof(*buckets1)));
   {  int init_values [] = { 1, 2, 2, 3, 4, 5 };
      TEST_INTRUSIVE_SEQUENCE_MAYBEUNIQUE( init_values, testset1 );  }

   testset1.rehash(bucket_traits(
      pointer_traits<bucket_ptr>::pointer_to(buckets2[0]), sizes::Small + ExtraBuckets));
   BOOST_IF_CONSTEXPR(unordered_type::fastmod_buckets){
      int init_values [] = { 3, 4, 1, 5, 2, 2 };
      TEST_INTRUSIVE_SEQUENCE_MAYBEUNIQUE( init_values, testset1 );
   }
   else{
      int init_values [] = { 4, 2, 2, 5, 3, 1 };
      TEST_INTRUSIVE_SEQUENCE_MAYBEUNIQUE( init_values, testset1 );
   }

   testset1.rehash(bucket_traits(
      pointer_traits<bucket_ptr>::pointer_to(buckets3[0]), sizeof(buckets3) / sizeof(*buckets3)));
   {  int init_values [] = { 1, 2, 2, 3, 4, 5 };
      TEST_INTRUSIVE_SEQUENCE_MAYBEUNIQUE( init_values, testset1 );  }

   //Now rehash reducing the buckets
   testset1.rehash(bucket_traits(
      pointer_traits<bucket_ptr>::pointer_to(buckets3[0]), sizes::Small + ExtraBuckets));
   BOOST_IF_CONSTEXPR(unordered_type::fastmod_buckets){
      int init_values [] = { 3, 4, 1, 5, 2, 2 };
      TEST_INTRUSIVE_SEQUENCE_MAYBEUNIQUE( init_values, testset1 );
   }
   else{
      int init_values [] = { 4, 2, 2, 5, 3, 1 };
      TEST_INTRUSIVE_SEQUENCE_MAYBEUNIQUE( init_values, testset1 );
   }

   //Now rehash increasing the buckets
   testset1.rehash(bucket_traits(
      pointer_traits<bucket_ptr>::pointer_to(buckets3[0]), sizeof(buckets3) / sizeof(*buckets3)));
   {  int init_values [] = { 1, 2, 2, 3, 4, 5 };
      TEST_INTRUSIVE_SEQUENCE_MAYBEUNIQUE( init_values, testset1 );  }

   //Full rehash (no effects)
   testset1.full_rehash();
   {  int init_values [] = { 1, 2, 2, 3, 4, 5 };
      TEST_INTRUSIVE_SEQUENCE_MAYBEUNIQUE( init_values, testset1 );  }

   //Test empty rehash
   testset1.clear();
   testset1.rehash(bucket_traits(
      pointer_traits<bucket_ptr>::pointer_to(buckets1[0]), sizeof(buckets1) / sizeof(*buckets1)));
   BOOST_TEST(testset1.empty());
   testset1.full_rehash();
   BOOST_TEST(testset1.empty());
}

//Checks that begin() is the first element of the first non-empty bucket or end()
template<class ContainerDefiner>
template<class Set>
void test_unordered<ContainerDefiner>::check_begin(Set &testset, std::size_t expected_size)
{
   BOOST_TEST(std::size_t(boost::intrusive::iterator_distance(testset.begin(), testset.end())) == expected_size);
   BOOST_TEST(testset.size() == expected_size);
   typedef typename Set::size_type size_type;
   size_type first_used = 0u;
   const size_type bucket_count = testset.bucket_count();
   while(first_used != bucket_count && testset.bucket_size(first_used) == 0u)
      ++first_used;
   if(first_used == bucket_count){
      BOOST_TEST(expected_size == 0u);
      BOOST_TEST(testset.begin() == testset.end());
   }
   else{
      BOOST_TEST(testset.begin() != testset.end());
      BOOST_TEST(&*testset.begin() == &*testset.begin(first_used));
   }
}

//test: rehash functions must keep begin() correct, with empty and non-empty containers
//and bucket arrays that are bigger or smaller than the original one
template<class ContainerDefiner>
void test_unordered<ContainerDefiner>::test_begin_after_rehash(detail::false_)   //not incremental
{
   typedef typename ContainerDefiner::template container
      <>::type unordered_type;
   typedef typename unordered_type::bucket_traits  bucket_traits;
   typedef typename unordered_type::bucket_ptr     bucket_ptr;
   const std::size_t ExtraBuckets = unordered_type::bucket_overhead;
   typedef bucket_sizes<unordered_type> sizes;

   //Elements are in buckets that are not the first ones and that are removed when shrinking
   value_cont_type values(2);
   (&values[0])->value_ = 3;
   (&values[1])->value_ = 6;

   for(std::size_t num_elements = 0u; num_elements <= 2u; num_elements += 2u){
      typename unordered_type::bucket_type buckets[sizes::Big + ExtraBuckets];
      typename unordered_type::bucket_type buckets2[sizes::Big + ExtraBuckets];
      unordered_type testset(values.begin(), values.begin() + std::ptrdiff_t(num_elements), bucket_traits(
         pointer_traits<bucket_ptr>::pointer_to(buckets[0]), sizes::Normal + ExtraBuckets));
      check_begin(testset, num_elements);

      //Shrink and grow in the same buffer
      testset.rehash(bucket_traits(pointer_traits<bucket_ptr>::pointer_to(buckets[0]), sizes::Small + ExtraBuckets));
      check_begin(testset, num_elements);
      testset.rehash(bucket_traits(pointer_traits<bucket_ptr>::pointer_to(buckets[0]), sizes::Big + ExtraBuckets));
      check_begin(testset, num_elements);
      testset.rehash(bucket_traits(pointer_traits<bucket_ptr>::pointer_to(buckets[0]), sizes::Normal + ExtraBuckets));
      check_begin(testset, num_elements);

      //Shrink and grow using another buffer
      testset.rehash(bucket_traits(pointer_traits<bucket_ptr>::pointer_to(buckets2[0]), sizes::Small + ExtraBuckets));
      check_begin(testset, num_elements);
      testset.rehash(bucket_traits(pointer_traits<bucket_ptr>::pointer_to(buckets[0]), sizes::Big + ExtraBuckets));
      check_begin(testset, num_elements);
      testset.clear();
   }
}

template<class ContainerDefiner>
void test_unordered<ContainerDefiner>::test_begin_after_rehash(detail::true_)   //incremental
{
   typedef typename ContainerDefiner::template container
      <>::type unordered_type;
   typedef typename unordered_type::bucket_traits  bucket_traits;
   typedef typename unordered_type::bucket_ptr     bucket_ptr;
   const std::size_t ExtraBuckets = unordered_type::bucket_overhead;
   typedef bucket_sizes<unordered_type> sizes;

   value_cont_type values(2);
   (&values[0])->value_ = 3;
   (&values[1])->value_ = 7;

   for(std::size_t num_elements = 0u; num_elements <= 2u; num_elements += 2u){
      typename unordered_type::bucket_type buckets1[sizes::Normal + ExtraBuckets];
      typename unordered_type::bucket_type buckets2[sizes::Big + ExtraBuckets];
      unordered_type testset(values.begin(), values.begin() + std::ptrdiff_t(num_elements), bucket_traits(
         pointer_traits<bucket_ptr>::pointer_to(buckets1[0]), sizes::Normal + ExtraBuckets));
      check_begin(testset, num_elements);

      //Split all the buckets
      while(testset.incremental_rehash(true))
         check_begin(testset, num_elements);
      //Use a bigger bucket array
      BOOST_TEST(testset.incremental_rehash(bucket_traits(
         pointer_traits<bucket_ptr>::pointer_to(buckets2[0]), sizes::Big + ExtraBuckets)));
      check_begin(testset, num_elements);
      //And again the original one
      BOOST_TEST(testset.incremental_rehash(bucket_traits(
         pointer_traits<bucket_ptr>::pointer_to(buckets1[0]), sizes::Normal + ExtraBuckets)));
      check_begin(testset, num_elements);
      //Merge buckets until the original split count
      while(testset.incremental_rehash(false))
         check_begin(testset, num_elements);
      testset.clear();
   }
}

//test: rehash and full_rehash must keep groups of equivalent elements
//correctly linked (e.g. when optimize_multikey is activated)
template<class ContainerDefiner>
template<bool FullRehash>
void test_unordered<ContainerDefiner>::test_rehash_groups(detail::true_)   //multikey
{
   typedef typename ContainerDefiner::template container
      <hash<seeded_hash> >::type unordered_type;
   typedef typename unordered_type::value_type     value_type;
   typedef typename unordered_type::bucket_traits  bucket_traits;
   typedef typename unordered_type::bucket_ptr     bucket_ptr;
   typedef typename unordered_type::key_of_value   key_of_value;
   const std::size_t ExtraBuckets = unordered_type::bucket_overhead;
   typedef bucket_sizes<unordered_type> sizes;

   //A group of more than two equivalent elements is needed to detect broken groups
   value_cont_type values(5);
   for (std::size_t i = 0u; i < 4u; ++i)
      (&values[i])->value_ = 9;
   (&values[4])->value_ = 1;

   value_type cmp_val;
   cmp_val.value_ = 9;
   seeded_hash::seed = 0u;

   typename unordered_type::bucket_type buckets1[sizes::Normal + ExtraBuckets];
   typename unordered_type::bucket_type buckets2[sizes::Big + ExtraBuckets];
   unordered_type testset(values.begin(), values.end(), bucket_traits(
      pointer_traits<bucket_ptr>::pointer_to(buckets1[0]), sizeof(buckets1)/sizeof(*buckets1)));

   if(FullRehash){
      //Change the hash function so that the group goes to another bucket
      seeded_hash::seed = 6u;
      testset.full_rehash();
   }
   else{
      testset.rehash(bucket_traits(
         pointer_traits<bucket_ptr>::pointer_to(buckets2[0]), sizeof(buckets2)/sizeof(*buckets2)));
   }
   BOOST_TEST(testset.size() == 5u);
   BOOST_TEST(testset.count(key_of_value()(cmp_val)) == 4u);
   BOOST_TEST(testset.count(key_of_value()(values[4])) == 1u);

   //Erase by iterator, as it uses the stored hash (if any) to obtain the bucket
   for(std::size_t i = 0; i != 4u; ++i){
      testset.erase(testset.iterator_to(values[i]));
      BOOST_TEST(testset.count(key_of_value()(cmp_val)) == 3u - i);
   }
   BOOST_TEST(testset.size() == 1u);
   testset.clear();
   seeded_hash::seed = 0u;
}

template<class ContainerDefiner>
void test_unordered<ContainerDefiner>::test_rehash_groups(detail::true_)   //multikey
{
   test_rehash_groups<false>(detail::true_());
   test_rehash_groups<true>(detail::true_());
}

template<class ContainerDefiner>
void test_unordered<ContainerDefiner>::test_rehash_groups(detail::false_)   //not multikey
{}

//test: erasing a range that starts and ends inside the same group of equivalent elements
template<class ContainerDefiner>
void test_unordered<ContainerDefiner>::test_erase_range_in_group(detail::true_)   //multikey
{
   typedef typename ContainerDefiner::template container
      <hash<zero_hash> >::type unordered_type;   //All elements are stored in the same bucket
   typedef typename unordered_type::iterator       iterator;
   typedef typename unordered_type::bucket_traits  bucket_traits;
   typedef typename unordered_type::bucket_ptr     bucket_ptr;
   typedef typename unordered_type::key_of_value   key_of_value;
   const std::size_t ExtraBuckets = unordered_type::bucket_overhead;
   typedef bucket_sizes<unordered_type> sizes;

   //Groups: (key 2, 2 elements), (key 9, 6 elements), (key 1, 2 elements). New groups are inserted
   //at the beginning of the bucket, so the group of key 9 has other groups before and after it.
   const std::size_t GroupSize = 6u, OtherSize = 2u, Total = GroupSize + 2u*OtherSize;   const int Key = 9, Before = 1, After = 2;

   typename unordered_type::bucket_type buckets[sizes::Normal + ExtraBuckets];

   //Try all the ranges of the group, including the ones that start or end at the group limits
   for(std::size_t a = 0u; a != GroupSize; ++a){
      for(std::size_t b = a + 1u; b <= GroupSize; ++b){
         value_cont_type values(Total);
         std::size_t k = 0u;
         for (std::size_t i = 0u; i != OtherSize; ++i)
            (&values[k++])->value_ = After;
         for (std::size_t i = 0u; i != GroupSize; ++i)
            (&values[k++])->value_ = Key;
         for (std::size_t i = 0u; i != OtherSize; ++i)
            (&values[k++])->value_ = Before;

         unordered_type testset(values.begin(), values.end(), bucket_traits(
            pointer_traits<bucket_ptr>::pointer_to(buckets[0]), sizeof(buckets)/sizeof(*buckets)));

         //Check that the group is surrounded by other groups in the same bucket
         BOOST_TEST(testset.bucket_count() >= 1u);
         BOOST_TEST(testset.bucket_size(0u) == Total);
         BOOST_TEST(testset.begin()->value_ != Key);
         iterator last_in_bucket = testset.begin();
         for(iterator it = testset.begin(); it != testset.end(); ++it)
            last_in_bucket = it;
         BOOST_TEST(last_in_bucket->value_ != Key);

         typename unordered_type::value_type key_val, before_val, after_val;
         key_val.value_ = Key;
         before_val.value_ = Before;
         after_val.value_ = After;

         std::pair<iterator, iterator> range = testset.equal_range(key_of_value()(key_val));
         BOOST_TEST(boost::intrusive::iterator_distance(range.first, range.second) == (std::ptrdiff_t)GroupSize);
         iterator first = range.first;
         for(std::size_t i = 0u; i != a; ++i)
            ++first;
         iterator last = first;
         for(std::size_t i = a; i != b; ++i)
            ++last;
         testset.erase(first, last);

         const std::size_t remaining = GroupSize - (b - a);
         BOOST_TEST(testset.size() == remaining + 2u*OtherSize);
         BOOST_TEST(testset.bucket_size(0u) == remaining + 2u*OtherSize);
         BOOST_TEST(testset.count(key_of_value()(key_val)) == remaining);
         BOOST_TEST(testset.count(key_of_value()(before_val)) == OtherSize);
         BOOST_TEST(testset.count(key_of_value()(after_val)) == OtherSize);
         range = testset.equal_range(key_of_value()(key_val));
         BOOST_TEST(boost::intrusive::iterator_distance(range.first, range.second) == (std::ptrdiff_t)remaining);
         range = testset.equal_range(key_of_value()(before_val));
         BOOST_TEST(boost::intrusive::iterator_distance(range.first, range.second) == (std::ptrdiff_t)OtherSize);
         range = testset.equal_range(key_of_value()(after_val));
         BOOST_TEST(boost::intrusive::iterator_distance(range.first, range.second) == (std::ptrdiff_t)OtherSize);

         //All the groups are still correctly linked, so they can be erased by key
         BOOST_TEST(testset.erase(key_of_value()(key_val)) == remaining);
         BOOST_TEST(testset.erase(key_of_value()(before_val)) == OtherSize);
         BOOST_TEST(testset.erase(key_of_value()(after_val)) == OtherSize);
         BOOST_TEST(testset.empty());
      }
   }
}

template<class ContainerDefiner>
void test_unordered<ContainerDefiner>::test_erase_range_in_group(detail::false_)   //not multikey
{}

//Builds two containers from the "xv" and "yv" arrays, compares them
//and checks that operator== and operator!= are consistent and symmetric
template<class ContainerDefiner>
bool test_unordered<ContainerDefiner>::test_equal
   (const int *xv, std::size_t xn, const int *yv, std::size_t yn)
{
   typedef typename ContainerDefiner::template container
      <>::type unordered_type;
   typedef typename unordered_type::bucket_traits  bucket_traits;
   typedef typename unordered_type::bucket_ptr     bucket_ptr;
   const std::size_t ExtraBuckets = unordered_type::bucket_overhead;
   typedef bucket_sizes<unordered_type> sizes;

   value_cont_type xvalues(xn);
   for (std::size_t i = 0u; i < xn; ++i)
      (&xvalues[i])->value_ = xv[i];
   value_cont_type yvalues(yn);
   for (std::size_t i = 0u; i < yn; ++i)
      (&yvalues[i])->value_ = yv[i];

   //Use different bucket counts so that iteration order can differ
   typename unordered_type::bucket_type xbuckets[sizes::Normal + ExtraBuckets];
   typename unordered_type::bucket_type ybuckets[sizes::Big + ExtraBuckets];
   unordered_type x(xvalues.begin(), xvalues.end(), bucket_traits(
      pointer_traits<bucket_ptr>::pointer_to(xbuckets[0]), sizeof(xbuckets)/sizeof(*xbuckets)));
   unordered_type y(yvalues.begin(), yvalues.end(), bucket_traits(
      pointer_traits<bucket_ptr>::pointer_to(ybuckets[0]), sizeof(ybuckets)/sizeof(*ybuckets)));

   const bool equal = x == y;
   BOOST_TEST(equal == (y == x));
   BOOST_TEST(equal == !(x != y));
   BOOST_TEST(equal == !(y != x));
   return equal;
}

//test: operator==, operator!=
template<class ContainerDefiner>
void test_unordered<ContainerDefiner>::test_equal()
{
   typedef typename ContainerDefiner::template container
      <>::type unordered_type;
   typedef bucket_sizes<unordered_type> sizes;
   const bool is_multikey = boost::intrusive::test::is_multikey_true<unordered_type>::value;
   const int N = int(sizes::Normal);

   const int empty[] = { 0 };
   const int a[] = { 1, 2, 3 };
   const int b[] = { 3, 1, 2 };
   const int c[] = { 1, 3, 4 };
   const int d[] = { 1, 2, 3, 9 };
   //Values in the same bucket of x
   const int e[] = { 1, 1 + N, 1 + 2*N };
   const int f[] = { 1, 1 + N, 2 + 2*N };

   BOOST_TEST(( test_equal(empty, 0u, empty, 0u)));
   BOOST_TEST((!test_equal(a, 3u, empty, 0u)));
   BOOST_TEST(( test_equal(a, 3u, a, 3u)));
   BOOST_TEST(( test_equal(a, 3u, b, 3u)));
   BOOST_TEST((!test_equal(a, 3u, c, 3u)));
   BOOST_TEST((!test_equal(a, 3u, d, 4u)));
   BOOST_TEST((!test_equal(a, 2u, a, 3u)));
   BOOST_TEST(( test_equal(e, 3u, e, 3u)));
   BOOST_TEST((!test_equal(e, 3u, f, 3u)));

   BOOST_IF_CONSTEXPR(is_multikey){
      const int g[] = { 2, 2, 3 };
      const int h[] = { 3, 2, 2 };
      const int i[] = { 2, 3, 3 };
      const int j[] = { 2, 2, 2, 3, 10, 10 };
      const int k[] = { 10, 2, 3, 2, 10, 2 };
      const int l[] = { 10, 2, 3, 2, 3, 2 };
      BOOST_TEST(( test_equal(g, 3u, h, 3u)));
      BOOST_TEST((!test_equal(g, 3u, i, 3u)));
      BOOST_TEST((!test_equal(g, 3u, j, 6u)));
      BOOST_TEST(( test_equal(j, 6u, k, 6u)));
      BOOST_TEST((!test_equal(j, 6u, l, 6u)));
   }
}

//test: find, equal_range (lower_bound, upper_bound):
template<class ContainerDefiner>
void test_unordered<ContainerDefiner>::test_find(value_cont_type& values)
{
   typedef typename ContainerDefiner::template container
      <>::type unordered_type;
   typedef typename unordered_type::value_type value_type;

   typedef typename unordered_type::bucket_traits  bucket_traits;
   typedef typename unordered_type::bucket_ptr     bucket_ptr;
   typedef typename unordered_type::key_of_value   key_of_value;
   const bool is_multikey = boost::intrusive::test::is_multikey_true<unordered_type>::value;
   const std::size_t ExtraBuckets = unordered_type::bucket_overhead;
   typedef bucket_sizes<unordered_type> sizes;

   typename unordered_type::bucket_type buckets[sizes::Normal + ExtraBuckets];
   unordered_type testset(values.begin(), values.end(), bucket_traits(
      pointer_traits<bucket_ptr>::pointer_to(buckets[0]), sizeof(buckets)/sizeof(*buckets)));

   typedef typename unordered_type::iterator iterator;

   value_type cmp_val;
   cmp_val.value_ = 2;
   BOOST_TEST (testset.count(key_of_value()(cmp_val)) == (is_multikey ? 2 : 1));
   iterator i = testset.find (key_of_value()(cmp_val));
   BOOST_TEST (i->value_ == 2);
   if(is_multikey)
      BOOST_TEST ((++i)->value_ == 2);
   else
      BOOST_TEST ((++i)->value_ != 2);
   std::pair<iterator,iterator> range = testset.equal_range (key_of_value()(cmp_val));

   BOOST_TEST (range.first->value_ == 2);
   BOOST_TEST (range.second->value_ == 3);
   BOOST_TEST (boost::intrusive::iterator_distance (range.first, range.second) == (is_multikey ? 2 : 1));
   cmp_val.value_ = 7;
   BOOST_TEST (testset.find (key_of_value()(cmp_val)) == testset.end());
   BOOST_TEST (testset.count(key_of_value()(cmp_val)) == 0);
}


template<class ContainerDefiner>
void test_unordered<ContainerDefiner>::test_clone(value_cont_type& values)
{
   typedef typename ContainerDefiner::template container
      <>::type unordered_type;
   const std::size_t ExtraBuckets = unordered_type::bucket_overhead;
   typedef bucket_sizes<unordered_type> sizes;

   typedef typename unordered_type::value_type value_type;
   typedef std::multiset<value_type> std_multiset_t;

   typedef typename unordered_type::bucket_traits bucket_traits;
   typedef typename unordered_type::bucket_ptr    bucket_ptr;

   {
      //Test with equal bucket arrays
      typename unordered_type::bucket_type buckets1[sizes::Normal + ExtraBuckets];
      typename unordered_type::bucket_type buckets2[sizes::Normal + ExtraBuckets];
      unordered_type testset1 (values.begin(), values.end(), bucket_traits(
         pointer_traits<bucket_ptr>::pointer_to(buckets1[0]), sizeof(buckets1)/sizeof(*buckets1)));
      unordered_type testset2 (bucket_traits(
         pointer_traits<bucket_ptr>::pointer_to(buckets2[0]), sizeof(buckets2)/sizeof(*buckets2)));
      //clone_from must not modify split_count as the bucket array of the target does not change
      const typename unordered_type::size_type split = testset2.split_count();

      testset2.clone_from(testset1, test::new_cloner<value_type>(), test::delete_disposer<value_type>());
      BOOST_TEST(testset2.split_count() == split);
      BOOST_TEST(testset1 == testset2);
      //Ordering is not guarantee in the cloning so insert data in a set and test
      std_multiset_t src(testset1.begin(), testset1.end());
      std_multiset_t dst(testset2.begin(), testset2.end());
      BOOST_TEST (src.size() == dst.size() && std::equal(src.begin(), src.end(), dst.begin()));
      testset2.clear_and_dispose(test::delete_noexcept_disposer<value_type>());
      BOOST_TEST (testset2.empty());

      testset2.clone_from(boost::move(testset1), test::new_nonconst_cloner<value_type>(), test::delete_disposer<value_type>());
      BOOST_TEST(testset2.split_count() == split);
      BOOST_TEST(testset1 == testset2);
      //Ordering is not guarantee in the cloning so insert data in a set and test
      std_multiset_t(testset1.begin(), testset1.end()).swap(src);
      std_multiset_t(testset2.begin(), testset2.end()).swap(dst);
      BOOST_TEST(src.size() == dst.size() && std::equal(src.begin(), src.end(), dst.begin()));
      testset2.clear_and_dispose(test::delete_noexcept_disposer<value_type>());
      BOOST_TEST (testset2.empty());
   }
   {
      //Test with bigger source bucket arrays
      typename unordered_type::bucket_type buckets1[sizes::Big + ExtraBuckets];
      typename unordered_type::bucket_type buckets2[sizes::Normal + ExtraBuckets];
      unordered_type testset1 (values.begin(), values.end(), bucket_traits(
         pointer_traits<bucket_ptr>::pointer_to(buckets1[0]), sizeof(buckets1)/sizeof(*buckets1)));
      unordered_type testset2 (bucket_traits(
         pointer_traits<bucket_ptr>::pointer_to(buckets2[0]), sizeof(buckets2)/sizeof(*buckets2)));
      //clone_from must not modify split_count as the bucket array of the target does not change
      const typename unordered_type::size_type split = testset2.split_count();

      testset2.clone_from(testset1, test::new_cloner<value_type>(), test::delete_disposer<value_type>());
      BOOST_TEST(testset2.split_count() == split);
      BOOST_TEST(testset1 == testset2);
      //Ordering is not guarantee in the cloning so insert data in a set and test
      std_multiset_t src(testset1.begin(), testset1.end());
      std_multiset_t dst(testset2.begin(), testset2.end());
      BOOST_TEST (src.size() == dst.size() && std::equal(src.begin(), src.end(), dst.begin()));
      testset2.clear_and_dispose(test::delete_disposer<value_type>());
      BOOST_TEST (testset2.empty());

      testset2.clone_from(boost::move(testset1), test::new_nonconst_cloner<value_type>(), test::delete_disposer<value_type>());
      BOOST_TEST(testset2.split_count() == split);
      BOOST_TEST(testset1 == testset2);
      //Ordering is not guarantee in the cloning so insert data in a set and test
      std_multiset_t(testset1.begin(), testset1.end()).swap(src);
      std_multiset_t(testset2.begin(), testset2.end()).swap(dst);
      BOOST_TEST (src.size() == dst.size() && std::equal(src.begin(), src.end(), dst.begin()));
      testset2.clear_and_dispose(test::delete_disposer<value_type>());
      BOOST_TEST (testset2.empty());
   }
   {
      //Test with smaller source bucket arrays
      typename unordered_type::bucket_type buckets1[sizes::Normal + ExtraBuckets];
      typename unordered_type::bucket_type buckets2[sizes::Big + ExtraBuckets];
      unordered_type testset1 (values.begin(), values.end(), bucket_traits(
         pointer_traits<bucket_ptr>::pointer_to(buckets1[0]), sizeof(buckets1)/sizeof(*buckets1)));
      unordered_type testset2 (bucket_traits(
         pointer_traits<bucket_ptr>::pointer_to(buckets2[0]), sizeof(buckets2)/sizeof(*buckets2)));
      //clone_from must not modify split_count as the bucket array of the target does not change
      const typename unordered_type::size_type split = testset2.split_count();

      testset2.clone_from(testset1, test::new_cloner<value_type>(), test::delete_disposer<value_type>());
      BOOST_TEST(testset2.split_count() == split);
      BOOST_TEST(testset1 == testset2);
      //Ordering is not guaranteed in the cloning so insert data in a set and test
      std_multiset_t src(testset1.begin(), testset1.end());
      std_multiset_t dst(testset2.begin(), testset2.end());
      BOOST_TEST (src.size() == dst.size() && std::equal(src.begin(), src.end(), dst.begin()));
      testset2.clear_and_dispose(test::delete_disposer<value_type>());
      BOOST_TEST (testset2.empty());

      testset2.clone_from(boost::move(testset1), test::new_nonconst_cloner<value_type>(), test::delete_disposer<value_type>());
      BOOST_TEST(testset2.split_count() == split);
      BOOST_TEST(testset1 == testset2);
      //Ordering is not guaranteed in the cloning so insert data in a set and test
      std_multiset_t(testset1.begin(), testset1.end()).swap(src);
      std_multiset_t(testset2.begin(), testset2.end()).swap(dst);
      BOOST_TEST (src.size() == dst.size() && std::equal(src.begin(), src.end(), dst.begin()));
      testset2.clear_and_dispose(test::delete_disposer<value_type>());
      BOOST_TEST (testset2.empty());
   }
   {
      //Empty source with equal, bigger and smaller source bucket arrays,
      //with an empty and a non-empty target
      const std::size_t src_sizes[] = { sizes::Normal, sizes::Big,    sizes::Normal };
      const std::size_t dst_sizes[] = { sizes::Normal, sizes::Normal, sizes::Big    };
      for(std::size_t i = 0; i != sizeof(src_sizes)/sizeof(src_sizes[0]); ++i){
         typename unordered_type::bucket_type buckets1[sizes::Normal + ExtraBuckets];
         typename unordered_type::bucket_type buckets2[sizes::Big + ExtraBuckets];
         typename unordered_type::bucket_type buckets3[sizes::Big + ExtraBuckets];
         unordered_type testset1 (values.begin(), values.end(), bucket_traits(
            pointer_traits<bucket_ptr>::pointer_to(buckets1[0]), sizeof(buckets1)/sizeof(*buckets1)));
         unordered_type empty_set (bucket_traits(
            pointer_traits<bucket_ptr>::pointer_to(buckets2[0]), src_sizes[i] + ExtraBuckets));
         unordered_type testset2 (bucket_traits(
            pointer_traits<bucket_ptr>::pointer_to(buckets3[0]), dst_sizes[i] + ExtraBuckets));
         //clone_from must not modify split_count as the bucket array of the target does not change
         const typename unordered_type::size_type split = testset2.split_count();

         testset2.clone_from(empty_set, test::new_cloner<value_type>(), test::delete_disposer<value_type>());
         BOOST_TEST(testset2.split_count() == split);
         BOOST_TEST (testset2.empty());
         BOOST_TEST (testset2.begin() == testset2.end());
         testset2.clone_from(testset1, test::new_cloner<value_type>(), test::delete_disposer<value_type>());
         testset2.clone_from(empty_set, test::new_cloner<value_type>(), test::delete_disposer<value_type>());
         BOOST_TEST(testset2.split_count() == split);
         BOOST_TEST (testset2.empty());
         BOOST_TEST (testset2.begin() == testset2.end());
         testset2.clone_from(testset1, test::new_cloner<value_type>(), test::delete_disposer<value_type>());
         testset2.clone_from(boost::move(empty_set), test::new_nonconst_cloner<value_type>(), test::delete_disposer<value_type>());
         BOOST_TEST(testset2.split_count() == split);
         BOOST_TEST (testset2.empty());
         BOOST_TEST (testset2.begin() == testset2.end());
         //The target is still usable
         testset2.clone_from(testset1, test::new_cloner<value_type>(), test::delete_disposer<value_type>());
         BOOST_TEST(testset1 == testset2);
         testset2.clear_and_dispose(test::delete_disposer<value_type>());
         BOOST_TEST (testset2.empty());
      }
   }
}

//Builds a functor with the state "state" if the functor has a state, else a default functor
inline instance_seed_hash make_tagged_functor(instance_seed_hash*, unsigned state)
{  return instance_seed_hash(state);  }

inline tagged_equal make_tagged_functor(tagged_equal*, unsigned state)
{  return tagged_equal(int(state));  }

template<class Functor>
Functor make_tagged_functor(Functor*, unsigned)
{  return Functor();  }

//Checks the state of a functor, if the functor has a state
inline bool functor_tag_is(const instance_seed_hash &f, unsigned state)
{  return f.seed_ == state;  }

inline bool functor_tag_is(const tagged_equal &f, unsigned state)
{  return f.tag_ == int(state);  }

template<class Functor>
bool functor_tag_is(const Functor &, unsigned)
{  return true;  }

//test: clone_from copies the hash function and the equality predicate of the source, also if
//the source is empty or the bucket array of the source can not be cloned structurally.
//"Option" is hash<instance_seed_hash> or equal<tagged_equal>
template<class ContainerDefiner>
template<class Option>
void test_unordered<ContainerDefiner>::test_clone_functors(value_cont_type& values)
{
   typedef typename ContainerDefiner::template container
      <Option>::type unordered_type;
   typedef typename unordered_type::hasher        hasher;
   typedef typename unordered_type::key_equal     key_equal;
   const std::size_t ExtraBuckets = unordered_type::bucket_overhead;
   typedef bucket_sizes<unordered_type> sizes;
   typedef typename unordered_type::value_type    value_type;
   typedef typename unordered_type::key_of_value  key_of_value;
   typedef typename unordered_type::iterator      iterator;
   typedef typename unordered_type::bucket_traits bucket_traits;
   typedef typename unordered_type::bucket_ptr    bucket_ptr;

   //0: the target has more buckets than the source (no structural copy)
   //1: the target has less buckets than the source
   //2: the source is empty
   for(int combination = 0; combination != 3; ++combination){
      typename unordered_type::bucket_type buckets1[sizes::Big + ExtraBuckets];
      typename unordered_type::bucket_type buckets2[sizes::Big + ExtraBuckets];
      const std::size_t src_buckets = (combination == 0 ? sizes::Normal : sizes::Big);
      const std::size_t dst_buckets = (combination == 0 ? sizes::Big : sizes::Normal);
      typename value_cont_type::iterator src_end = values.begin();
      if(combination != 2)
         src_end = values.end();

      unordered_type src (values.begin(), src_end, bucket_traits(
         pointer_traits<bucket_ptr>::pointer_to(buckets1[0]), src_buckets + ExtraBuckets)
         , make_tagged_functor((hasher*)0, 5u), make_tagged_functor((key_equal*)0, 7u));
      unordered_type dst (bucket_traits(
         pointer_traits<bucket_ptr>::pointer_to(buckets2[0]), dst_buckets + ExtraBuckets)
         , make_tagged_functor((hasher*)0, 2u), make_tagged_functor((key_equal*)0, 3u));

      dst.clone_from(src, test::new_cloner<value_type>(), test::delete_disposer<value_type>());
      BOOST_TEST(functor_tag_is(dst.hash_function(), 5u));
      BOOST_TEST(functor_tag_is(dst.key_eq(), 7u));
      BOOST_TEST(dst.size() == src.size());
      //The cloned elements are found using the copied hash function
      for(iterator it = src.begin(), itend = src.end(); it != itend; ++it){
         BOOST_TEST(dst.find(key_of_value()(*it)) != dst.end());
         BOOST_TEST(dst.count(key_of_value()(*it)) == src.count(key_of_value()(*it)));
      }
      dst.clear_and_dispose(test::delete_disposer<value_type>());
      BOOST_TEST(dst.empty());
   }
}

//test: a moved-from container has no bucket array. It can be destroyed, cleared, iterated,
//swapped, move assigned, cloned from an empty container, used as an empty source of
//clone_from and rehashed with a new bucket array.
template<class ContainerDefiner>
void test_unordered<ContainerDefiner>::test_moved_from(value_cont_type& values)
{
   typedef typename ContainerDefiner::template container
      <>::type unordered_type;
   const std::size_t ExtraBuckets = unordered_type::bucket_overhead;
   typedef bucket_sizes<unordered_type> sizes;
   typedef typename unordered_type::value_type    value_type;
   typedef typename unordered_type::key_of_value  key_of_value;
   typedef typename unordered_type::iterator      iterator;
   typedef typename unordered_type::bucket_traits bucket_traits;
   typedef typename unordered_type::bucket_ptr    bucket_ptr;
   typedef typename unordered_type::size_type     size_type;

   typename unordered_type::bucket_type buckets1[sizes::Normal + ExtraBuckets];
   typename unordered_type::bucket_type buckets2[sizes::Normal + ExtraBuckets];
   unordered_type src (values.begin(), values.end(), bucket_traits(
      pointer_traits<bucket_ptr>::pointer_to(buckets1[0]), sizes::Normal + ExtraBuckets));
   const size_type size = src.size();
   unordered_type dst (boost::move(src));
   BOOST_TEST(dst.size() == size);

   //Observers, iteration, clear and full_rehash
   BOOST_TEST(src.bucket_count() == 0u);
   BOOST_TEST(src.size() == 0u);
   BOOST_TEST(src.empty());
   BOOST_TEST(src.begin() == src.end());
   src.clear();
   src.full_rehash();
   BOOST_TEST(src.empty());

   {  //clone_from an empty container and to a container
      unordered_type empty_cont (bucket_traits(
         pointer_traits<bucket_ptr>::pointer_to(buckets2[0]), sizes::Normal + ExtraBuckets));
      src.clone_from(empty_cont, test::new_cloner<value_type>(), test::delete_disposer<value_type>());
      BOOST_TEST(src.empty());
      empty_cont.clone_from(src, test::new_cloner<value_type>(), test::delete_disposer<value_type>());
      BOOST_TEST(empty_cont.empty());
   }

   //swap
   src.swap(dst);
   BOOST_TEST(src.size() == size);
   BOOST_TEST(dst.size() == 0u);
   BOOST_TEST(dst.begin() == dst.end());
   src.swap(dst);
   BOOST_TEST(dst.size() == size);

   //rehash with a new bucket array, the container can be used again
   dst.clear();
   src.rehash(bucket_traits(
      pointer_traits<bucket_ptr>::pointer_to(buckets2[0]), sizes::Normal + ExtraBuckets));
   BOOST_TEST(src.bucket_count() == sizes::Normal);
   src.insert(values.begin(), values.end());
   BOOST_TEST(src.size() == size);
   for(iterator it = src.begin(), itend = src.end(); it != itend; ++it){
      BOOST_TEST(src.find(key_of_value()(*it)) != src.end());
   }

   //move assignment to a moved-from container
   unordered_type moved (boost::move(src));
   BOOST_TEST(src.bucket_count() == 0u);
   src = boost::move(moved);
   BOOST_TEST(src.size() == size);
   BOOST_TEST(moved.bucket_count() == 0u);
   BOOST_TEST(moved.empty());
   src.clear();
}

}  //namespace test{
}  //namespace intrusive{
}  //namespace boost{
