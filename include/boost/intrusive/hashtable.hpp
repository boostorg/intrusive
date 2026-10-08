/////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Ion Gaztanaga  2006-2022
// (C) Copyright 2022 Joaquin M Lopez Munoz.
// (C) Copyright 2022 Christian Mazakas
//
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/intrusive for documentation.
//
/////////////////////////////////////////////////////////////////////////////

// fastmod_buckets option is implemented reusing parts of Joaquin M. Lopez
// Munoz's "fxa_unordered" library (proof of concept of closed- and
// open-addressing unordered associative containers), released under
// Boost Software License:
// 
// https://github.com/joaquintides/fxa_unordered/
// 
// On cases and systems that can't take advantage of Daniel Lemire's
// "fastmod" (https://github.com/lemire/fastmod) approach, 
// precomputed divisions are used.
//
// As always, thanks Joaquin for your great work!


#ifndef BOOST_INTRUSIVE_HASHTABLE_HPP
#define BOOST_INTRUSIVE_HASHTABLE_HPP

#include <boost/intrusive/detail/config_begin.hpp>
#include <boost/intrusive/detail/workaround.hpp>
#include <boost/intrusive/intrusive_fwd.hpp>

#include <boost/move/detail/meta_utils_core.hpp>

//General intrusive utilities
#include <boost/intrusive/detail/hashtable_node.hpp>
#include <boost/intrusive/detail/transform_iterator.hpp>
#include <boost/intrusive/link_mode.hpp>
#include <boost/intrusive/detail/ebo_functor_holder.hpp>
#include <boost/intrusive/detail/is_stateful_value_traits.hpp>
#include <boost/intrusive/detail/node_to_value.hpp>
#include <boost/intrusive/detail/exception_disposer.hpp>
#include <boost/intrusive/detail/node_cloner_disposer.hpp>
#include <boost/intrusive/detail/simple_disposers.hpp>
#include <boost/intrusive/detail/size_holder.hpp>
#include <boost/intrusive/detail/iterator.hpp>
#include <boost/intrusive/detail/get_value_traits.hpp>
#include <boost/intrusive/detail/algorithm.hpp>
#include <boost/intrusive/detail/value_functors.hpp>
#include <boost/intrusive/detail/prime_holder.hpp>

//Implementation utilities
#include <boost/intrusive/unordered_set_hook.hpp>
#include <boost/intrusive/detail/slist_iterator.hpp>
#include <boost/intrusive/pointer_traits.hpp>
#include <boost/intrusive/detail/mpl.hpp>
#include <boost/intrusive/circular_slist_algorithms.hpp>
#include <boost/intrusive/linear_slist_algorithms.hpp>

//boost
#include <boost/intrusive/detail/assert.hpp>
#include <boost/move/utility_core.hpp>
#include <boost/move/adl_move_swap.hpp>

//std C++
#include <boost/intrusive/detail/minimal_pair_header.hpp>   //std::pair
#include <cstddef>      //std::size_t


#include "detail/hash.hpp"

#if defined(BOOST_HAS_PRAGMA_ONCE)
#  pragma once
#endif



namespace boost {

namespace intrusive {


#if !defined(BOOST_INTRUSIVE_DOXYGEN_INVOKED)

template<class InputIt, class T>
InputIt priv_algo_find(InputIt first, InputIt last, const T& value)
{
   for (; first != last; ++first) {
      if (*first == value) {
         return first;
      }
   }
   return last;
}

template<class InputIt, class T>
typename boost::intrusive::iterator_traits<InputIt>::difference_type
   priv_algo_count(InputIt first, InputIt last, const T& value)
{
   typename boost::intrusive::iterator_traits<InputIt>::difference_type ret = 0;
   for (; first != last; ++first) {
      if (*first == value) {
            ret++;
      }
   }
   return ret;
}

template <class ForwardIterator1, class ForwardIterator2>
bool priv_algo_is_permutation(ForwardIterator1 first1, ForwardIterator1 last1, ForwardIterator2 first2)
{
   typedef typename
      boost::intrusive::iterator_traits<ForwardIterator2>::difference_type
         distance_type;
   //Efficiently compare identical prefixes: O(N) if sequences
   //have the same elements in the same order.
   for ( ; first1 != last1; ++first1, ++first2){
      if (! (*first1 == *first2))
         break;
   }
   if (first1 == last1){
      return true;
   }

   //Establish last2 assuming equal ranges by iterating over the
   //rest of the list.
   ForwardIterator2 last2 = first2;
   boost::intrusive::iterator_advance(last2, boost::intrusive::iterator_distance(first1, last1));
   for(ForwardIterator1 scan = first1; scan != last1; ++scan){
      if (scan != (priv_algo_find)(first1, scan, *scan)){
         continue;   //We've seen this one before.
      }
      distance_type matches = (priv_algo_count)(first2, last2, *scan);
      if (0 == matches || (priv_algo_count)(scan, last1, *scan) != matches){
         return false;
      }
   }
   return true;
}

struct hash_bool_flags
{
   static const std::size_t unique_keys_pos        = 1u;
   static const std::size_t constant_time_size_pos = 2u;
   static const std::size_t power_2_buckets_pos    = 4u;
   static const std::size_t cache_begin_pos        = 8u;
   static const std::size_t compare_hash_pos       = 16u;
   static const std::size_t incremental_pos        = 32u;
   static const std::size_t linear_buckets_pos     = 64u;
   static const std::size_t fastmod_buckets_pos    = 128u;
};

//Sets the sentinel bucket of linear bucket arrays again
//if an exception is thrown while it is unset.
template<class Table>
class exception_sentinel_bucket_restorer
{
   const Table *table_;

   exception_sentinel_bucket_restorer(const exception_sentinel_bucket_restorer&);
   exception_sentinel_bucket_restorer &operator=(const exception_sentinel_bucket_restorer&);

   public:
   BOOST_INTRUSIVE_FORCEINLINE explicit exception_sentinel_bucket_restorer(const Table &table)
      :  table_(&table)
   {}

   BOOST_INTRUSIVE_FORCEINLINE void release()
   {  table_ = 0;  }

   ~exception_sentinel_bucket_restorer()
   {
      if(table_)
         table_->priv_set_sentinel_bucket();
   }
};

template<class Bucket, class Algo, class Disposer, class SizeType>
class exception_bucket_disposer
{
   Bucket         *cont_;
   Disposer       &disp_;
   const SizeType &constructed_;

   exception_bucket_disposer(const exception_bucket_disposer&);
   exception_bucket_disposer &operator=(const exception_bucket_disposer&);

   public:

   BOOST_INTRUSIVE_FORCEINLINE exception_bucket_disposer
      (Bucket *cont, Disposer &disp, const SizeType &constructed)
      :  cont_(cont), disp_(disp), constructed_(constructed)
   {}

   BOOST_INTRUSIVE_FORCEINLINE void release()
   {  cont_ = 0;  }

   ~exception_bucket_disposer()
   {
      SizeType n = constructed_;
      if(cont_){
         while(n--){
            Algo::detach_and_dispose(cont_[n].get_node_ptr(), disp_);
         }
      }
   }
};

template<class SupposedValueTraits>
struct unordered_bucket_impl
{
   typedef typename detail::get_node_traits
      <SupposedValueTraits>::type               node_traits;
   typedef typename reduced_slist_node_traits
      <node_traits>::type                       reduced_node_traits;
   typedef bucket_impl<reduced_node_traits>     type;

   typedef typename pointer_traits
         <typename reduced_node_traits::node_ptr>
            ::template rebind_pointer<type>::type pointer;
};

template<class SupposedValueTraits>
struct unordered_bucket_ptr_impl
{
   typedef typename unordered_bucket_impl<SupposedValueTraits>::pointer type;
};


template <class BucketPtr, class SizeType>
struct bucket_traits_impl
{
private:
   BOOST_COPYABLE_AND_MOVABLE(bucket_traits_impl)

public:

   typedef BucketPtr bucket_ptr;
   typedef SizeType  size_type;

   BOOST_INTRUSIVE_FORCEINLINE bucket_traits_impl(bucket_ptr buckets, size_type len)
      : buckets_(buckets), buckets_len_(len)
   {}

   #if !defined(BOOST_NO_CXX11_DEFAULTED_FUNCTIONS) && !defined(BOOST_NO_CXX11_RVALUE_REFERENCES)
   //Defaulted copy operations are trivial for raw bucket pointers
   bucket_traits_impl(const bucket_traits_impl&) = default;
   bucket_traits_impl& operator=(const bucket_traits_impl&) = default;
   #else
   BOOST_INTRUSIVE_FORCEINLINE bucket_traits_impl(const bucket_traits_impl& x)
      : buckets_(x.buckets_), buckets_len_(x.buckets_len_)
   {}

   inline bucket_traits_impl& operator=(BOOST_COPY_ASSIGN_REF(bucket_traits_impl) x)
   {
      buckets_ = x.buckets_;  buckets_len_ = x.buckets_len_; return *this;
   }
   #endif

   inline bucket_traits_impl(BOOST_RV_REF(bucket_traits_impl) x)
      : buckets_(x.buckets_), buckets_len_(x.buckets_len_)
   {
      x.buckets_ = bucket_ptr();   x.buckets_len_ = 0u;
   }

   inline bucket_traits_impl& operator=(BOOST_RV_REF(bucket_traits_impl) x)
   {
      buckets_ = x.buckets_; buckets_len_ = x.buckets_len_;
      x.buckets_ = bucket_ptr();   x.buckets_len_ = 0u; return *this;
   }

   BOOST_INTRUSIVE_FORCEINLINE bucket_ptr bucket_begin() const
   {
      return buckets_;
   }

   BOOST_INTRUSIVE_FORCEINLINE size_type  bucket_count() const BOOST_NOEXCEPT
   {
      return buckets_len_;
   }

private:
   bucket_ptr  buckets_;
   size_type   buckets_len_;
};


template <class T>
struct store_hash_is_true
{
   template<bool Add>
   struct two_or_three {detail::yes_type _[2u + (unsigned)Add];};
   template <class U> static detail::yes_type test(...);
   template <class U> static two_or_three<U::store_hash> test (int);
   static const bool value = sizeof(test<T>(0)) > sizeof(detail::yes_type)*2u;
};

template <class T>
struct optimize_multikey_is_true
{
   template<bool Add>
   struct two_or_three { detail::yes_type _[2u + (unsigned)Add];};
   template <class U> static detail::yes_type test(...);
   template <class U> static two_or_three<U::optimize_multikey> test (int);
   static const bool value = sizeof(test<T>(0)) > sizeof(detail::yes_type)*2u;
};

struct insert_commit_data_impl
{
   std::size_t hash;
   std::size_t bucket_idx;
   BOOST_INTRUSIVE_FORCEINLINE std::size_t get_hash() const
   {  return hash; }

   BOOST_INTRUSIVE_FORCEINLINE void set_hash(std::size_t h)
   {  hash = h;  }
};

template<class Node, class SlistNodePtr>
inline typename pointer_traits<SlistNodePtr>::template rebind_pointer<Node>::type
   dcast_bucket_ptr(const SlistNodePtr &p)
{
   typedef typename pointer_traits<SlistNodePtr>::template rebind_pointer<Node>::type node_ptr;
   return pointer_traits<node_ptr>::static_cast_from(p);
}

template<class NodeTraits>
struct group_functions
{
   //           A group is reverse-linked
   //
   //          A is "first in group"
   //          C is "last  in group"
   //           __________________
   //          |  _____   _____   |
   //          | |     | |      | |  <- Group links
   //          ^ V     ^ V      ^ V
   //           _       _        _      _
   //         A|_|    B|_|     C|_|   D|_|
   //
   //          ^ |     ^ |      ^ |    ^ V  <- Bucket links
   //   _ _____| |_____| |______| |____| |
   //  |B|                               |
   //   ^________________________________|
   //

   typedef NodeTraits                                             node_traits;
   typedef unordered_group_adapter<node_traits>                   group_traits;
   typedef typename node_traits::node_ptr                         node_ptr;
   typedef typename node_traits::node                             node;
   typedef typename reduced_slist_node_traits
      <node_traits>::type                                         reduced_node_traits;
   typedef typename reduced_node_traits::node_ptr                 slist_node_ptr;
   typedef typename reduced_node_traits::node                     slist_node;
   typedef circular_slist_algorithms<group_traits>                group_algorithms;
   typedef circular_slist_algorithms<node_traits>                 node_algorithms;

   static slist_node_ptr get_bucket_before_begin
      (slist_node_ptr bucket_beg, slist_node_ptr bucket_last, slist_node_ptr sp, detail::true_)
   {
      //First find the last node of p's group.
      //This requires checking the first node of the next group or
      //the bucket node.
      node_ptr p = dcast_bucket_ptr<node>(sp);
      node_ptr prev_node = p;
      node_ptr nxt(node_traits::get_next(p));
      while(!(bucket_beg <= nxt && nxt <= bucket_last) &&
             (group_traits::get_next(nxt) == prev_node)){
         prev_node = nxt;
         nxt = node_traits::get_next(nxt);
      }

      //If we've reached the bucket node just return it.
      if(bucket_beg <= nxt && nxt <= bucket_last){
         return nxt;
      }

      //Otherwise, iterate using group links until the bucket node
      node_ptr first_node_of_group  = nxt;
      node_ptr last_node_group      = group_traits::get_next(first_node_of_group);
      slist_node_ptr possible_end   = node_traits::get_next(last_node_group);

      while(!(bucket_beg <= possible_end && possible_end <= bucket_last)){
         first_node_of_group = dcast_bucket_ptr<node>(possible_end);
         last_node_group   = group_traits::get_next(first_node_of_group);
         possible_end      = node_traits::get_next(last_node_group);
      }
      return possible_end;
   }

   static slist_node_ptr get_bucket_before_begin
      (slist_node_ptr bucket_beg, slist_node_ptr bucket_last, slist_node_ptr sp, detail::false_)
   {
      //The end node is embedded in the singly linked list:
      //iterate until we reach it.
      while (!(bucket_beg <= sp && sp <= bucket_last)){
         sp = reduced_node_traits::get_next(sp);
      }
      return sp;
   }

   static node_ptr get_prev_to_first_in_group(slist_node_ptr bucket_node, node_ptr first_in_group)
   {
      node_ptr nb = dcast_bucket_ptr<node>(bucket_node);
      node_ptr n;
      while((n = node_traits::get_next(nb)) != first_in_group){
         nb = group_traits::get_next(n);  //go to last in group
      }
      return nb;
   }

   static void erase_from_group(slist_node_ptr end_ptr, node_ptr to_erase_ptr, detail::true_)
   {
      node_ptr const nxt_ptr(node_traits::get_next(to_erase_ptr));
      //Check if the next node is in the group (not end node) and reverse linked to
      //'to_erase_ptr'. Erase if that's the case.
      if(nxt_ptr != end_ptr && to_erase_ptr == group_traits::get_next(nxt_ptr)){
         group_algorithms::unlink_after(nxt_ptr);
      }
   }

   BOOST_INTRUSIVE_FORCEINLINE static void erase_from_group(slist_node_ptr, node_ptr, detail::false_)
   {}

   BOOST_INTRUSIVE_FORCEINLINE static node_ptr get_last_in_group(node_ptr first_in_group, detail::true_)
   {  return group_traits::get_next(first_in_group);  }

   BOOST_INTRUSIVE_FORCEINLINE static node_ptr get_last_in_group(node_ptr n, detail::false_)
   {  return n;  }

   static node_ptr get_first_in_group(node_ptr n, detail::true_)
   {
      node_ptr ng;
      while(n == node_traits::get_next((ng = group_traits::get_next(n)))){
         n = ng;
      }
      return n;
   }

   BOOST_INTRUSIVE_FORCEINLINE static node_ptr get_first_in_group(node_ptr n, detail::false_)
   {  return n;  }

   BOOST_INTRUSIVE_FORCEINLINE static bool is_first_in_group(node_ptr ptr)
   {  return node_traits::get_next(group_traits::get_next(ptr)) != ptr;  }


   BOOST_INTRUSIVE_FORCEINLINE static void insert_in_group(node_ptr first_in_group, node_ptr n, detail::true_)
   {  group_algorithms::link_after(first_in_group, n);  }

   BOOST_INTRUSIVE_FORCEINLINE static void insert_in_group(node_ptr, node_ptr, detail::false_)
   {}

   //Splits a group in two groups, and makes "new_first" the first node in the second group.
   //Returns the first element of the first group
   static node_ptr split_group(node_ptr const new_first)
   {
      node_ptr const old_first((get_first_in_group)(new_first, detail::true_()));
      //Check new_first was not the first in group
      if(old_first != new_first){
         node_ptr const last = group_traits::get_next(old_first);
         group_traits::set_next(old_first, group_traits::get_next(new_first));
         group_traits::set_next(new_first, last);
      }
      return old_first;
   }
};

template<class BucketType, class SplitTraits, class SlistNodeAlgorithms>
class incremental_rehash_rollback
{
   private:
   typedef BucketType   bucket_type;
   typedef SplitTraits  split_traits;

   incremental_rehash_rollback();
   incremental_rehash_rollback & operator=(const incremental_rehash_rollback &);
   incremental_rehash_rollback (const incremental_rehash_rollback &);

   public:
   BOOST_INTRUSIVE_FORCEINLINE incremental_rehash_rollback
      (bucket_type &source_bucket, bucket_type &destiny_bucket, split_traits &split_tr)
      :  source_bucket_(source_bucket),  destiny_bucket_(destiny_bucket)
      ,  split_traits_(split_tr),  released_(false)
   {}

   BOOST_INTRUSIVE_FORCEINLINE void release()
   {  released_ = true; }

   ~incremental_rehash_rollback()
   {
      if(!released_){
         //If an exception is thrown, just put all moved nodes back in the old bucket
         //and move back the split mark.
         SlistNodeAlgorithms::transfer_after(destiny_bucket_.get_node_ptr(), source_bucket_.get_node_ptr());
         split_traits_.decrement();
      }
   }

   private:
   bucket_type &source_bucket_;
   bucket_type &destiny_bucket_;
   split_traits &split_traits_;
   bool released_;
};

template<class NodeTraits>
struct node_functions
{
   BOOST_INTRUSIVE_FORCEINLINE static void store_hash(typename NodeTraits::node_ptr p, std::size_t h, detail::true_)
   {  return NodeTraits::set_hash(p, h); }

   BOOST_INTRUSIVE_FORCEINLINE static void store_hash(typename NodeTraits::node_ptr, std::size_t, detail::false_)
   {}
};

BOOST_INTRUSIVE_FORCEINLINE std::size_t hash_to_bucket(std::size_t hash_value, std::size_t bucket_cnt, detail::false_)
{  return hash_value % bucket_cnt;  }

BOOST_INTRUSIVE_FORCEINLINE std::size_t hash_to_bucket(std::size_t hash_value, std::size_t bucket_cnt, detail::true_)
{  return hash_value & (bucket_cnt - 1);   }

//Rules that map hash values to buckets and compute the split count and bucket counts.
//They only depend on the bucket options (and the size type), not on the value type.
//"split" is the split count with incremental<true> and the index of the bucket count
//with fastmod_buckets<true>.
template<bool Power2Buckets, bool Incremental, bool FastmodBuckets>
struct hashtable_index_ops
{
   //"usable_cnt" is the bucket count without the sentinel bucket of linear buckets.
   //It is not used with fastmod_buckets.
   static std::size_t hash_to_bucket(std::size_t hash_value, std::size_t usable_cnt, std::size_t split)
   {
      (void)usable_cnt; (void)split;
      BOOST_IF_CONSTEXPR(FastmodBuckets){
         return prime_fmod_size::position(hash_value, split);
      }
      else{
         std::size_t bucket_number = ::boost::intrusive::hash_to_bucket
            (hash_value, usable_cnt, detail::bool_<Power2Buckets>());
         BOOST_IF_CONSTEXPR(Incremental)
            bucket_number -= static_cast<std::size_t>(bucket_number >= split)*(usable_cnt/2);
         return bucket_number;
      }
   }

   template<class SizeType>
   static SizeType initial_split_from_bucket_count(SizeType bc) BOOST_NOEXCEPT
   {
      BOOST_IF_CONSTEXPR(FastmodBuckets) {
         const SizeType split = static_cast<SizeType>(prime_fmod_size::lower_size_index(bc));
         //The passed bucket size must be exactly the supported one
         BOOST_ASSERT(prime_fmod_size::size(split) == bc);
         return split;
      }
      else {
         BOOST_IF_CONSTEXPR(Incremental) {
            BOOST_ASSERT(0 == (std::size_t(bc) & (std::size_t(bc) - 1u)));
            return SizeType(bc >> 1u);
         }
         else{
            return bc;
         }
      }
   }

   template<class SizeType>
   BOOST_INTRUSIVE_FORCEINLINE static SizeType rehash_split_from_bucket_count(SizeType bc) BOOST_NOEXCEPT
   {
      BOOST_IF_CONSTEXPR(FastmodBuckets) {
         return (initial_split_from_bucket_count)(bc);
      }
      else {
         BOOST_IF_CONSTEXPR(Incremental) {
            BOOST_ASSERT(0 == (std::size_t(bc) & (std::size_t(bc) - 1u)));
         }
         return bc;
      }
   }

   template<class SizeType>
   static SizeType suggested_upper_bucket_count(SizeType n) BOOST_NOEXCEPT
   {
      BOOST_IF_CONSTEXPR(FastmodBuckets){
         std::size_t s = prime_fmod_size::upper_size_index(n);
         return static_cast<SizeType>(prime_fmod_size::size(s));
      }
      else{
         return prime_list_holder<0>::suggested_upper_bucket_count(n);
      }
   }

   template<class SizeType>
   static SizeType suggested_lower_bucket_count(SizeType n) BOOST_NOEXCEPT
   {
      BOOST_IF_CONSTEXPR(FastmodBuckets){
         std::size_t s = prime_fmod_size::lower_size_index(n);
         return static_cast<SizeType>(prime_fmod_size::size(s));
      }
      else{
         return prime_list_holder<0>::suggested_lower_bucket_count(n);
      }
   }
};

//!This metafunction will obtain the type of a bucket
//!from the value_traits or hook option to be used with
//!a hash container.
template<class ValueTraitsOrHookOption>
struct unordered_bucket
   : public unordered_bucket_impl
      < typename ValueTraitsOrHookOption::
         template pack<empty>::proto_value_traits>
{};

//!This metafunction will obtain the type of a bucket pointer
//!from the value_traits or hook option to be used with
//!a hash container.
template<class ValueTraitsOrHookOption>
struct unordered_bucket_ptr
   : public unordered_bucket_ptr_impl
      < typename ValueTraitsOrHookOption::
         template pack<empty>::proto_value_traits>
{};

//!This metafunction will obtain the type of the default bucket traits
//!(when the user does not specify the bucket_traits<> option) from the
//!value_traits or hook option to be used with
//!a hash container.
template<class ValueTraitsOrHookOption>
struct unordered_default_bucket_traits
{
   typedef typename ValueTraitsOrHookOption::
      template pack<empty>::proto_value_traits     supposed_value_traits;
   
   typedef bucket_traits_impl
      < typename unordered_bucket_ptr_impl
            <supposed_value_traits>::type
      , std::size_t>                               type;
};

struct default_bucket_traits;

//hashtable default hook traits
struct default_hashtable_hook_applier
{  template <class T> struct apply{ typedef typename T::default_hashtable_hook type;  };  };

template<>
struct is_default_hook_tag<default_hashtable_hook_applier>
{  static const bool value = true;  };

struct hashtable_defaults
{
   typedef default_hashtable_hook_applier   proto_value_traits;
   typedef std::size_t                 size_type;
   typedef void                        key_of_value;
   typedef void                        equal;
   typedef void                        hash;
   typedef default_bucket_traits       bucket_traits;
   static const bool constant_time_size   = true;
   static const bool power_2_buckets      = false;
   static const bool cache_begin          = false;
   static const bool compare_hash         = false;
   static const bool incremental          = false;
   static const bool linear_buckets       = false;
   static const bool fastmod_buckets      = false;
};

template<class ValueTraits, bool IsConst>
struct downcast_node_to_value_t
   :  public detail::node_to_value<ValueTraits, IsConst>
{
   typedef detail::node_to_value<ValueTraits, IsConst>   base_t;
   typedef typename base_t::result_type                  result_type;
   typedef ValueTraits                                   value_traits;
   typedef typename unordered_bucket_impl
      <value_traits>::type::node_traits::node            node;
   typedef typename detail::add_const_if_c
         <node, IsConst>::type                          &first_argument_type;
   typedef typename detail::add_const_if_c
         < typename ValueTraits::node_traits::node
         , IsConst>::type                               &intermediate_argument_type;
   typedef typename pointer_traits
      <typename ValueTraits::pointer>::
         template rebind_pointer
            <const ValueTraits>::type                   const_value_traits_ptr;

   BOOST_INTRUSIVE_FORCEINLINE downcast_node_to_value_t()
      :  base_t(const_value_traits_ptr())
   {}

   BOOST_INTRUSIVE_FORCEINLINE downcast_node_to_value_t(const_value_traits_ptr ptr)
      :  base_t(ptr)
   {}

   //Conversion from the non-const functor (only enabled if IsConst is true)
   inline downcast_node_to_value_t
      (const typename detail::if_c< IsConst
                                  , downcast_node_to_value_t<ValueTraits, false>
                                  , move_detail::nat>::type &other)
      :  base_t(other.get_value_traits())
   {}

   BOOST_INTRUSIVE_FORCEINLINE result_type operator()(first_argument_type arg) const
   {  return this->base_t::operator()(static_cast<intermediate_argument_type>(arg)); }
};

template<class F, class SlistNodePtr, class NodePtr>
struct node_cast_adaptor
   //Use public inheritance to avoid MSVC bugs with closures
   :  public detail::ebo_functor_holder<F>
{
   typedef detail::ebo_functor_holder<F> base_t;

   typedef typename pointer_traits<SlistNodePtr>::element_type slist_node;
   typedef typename pointer_traits<NodePtr>::element_type      node;

   template<class ConvertibleToF, class RealValuTraits>
   BOOST_INTRUSIVE_FORCEINLINE node_cast_adaptor(const ConvertibleToF &c2f, const RealValuTraits *traits)
      :  base_t(base_t(c2f, traits))
   {}

   BOOST_INTRUSIVE_FORCEINLINE NodePtr operator()(const slist_node &to_clone)
   {  return base_t::operator()(static_cast<const node &>(to_clone));   }

   BOOST_INTRUSIVE_FORCEINLINE void operator()(SlistNodePtr to_clone)
   {
      base_t::operator()(pointer_traits<NodePtr>::pointer_to(static_cast<node &>(*to_clone)));
   }
};
 
//Hash functor for node-level rehash operations: returns the hash value stored in the node.
//It does not depend on the value type.
template<class NodeTraits>
struct hashtable_stored_hash
{
   BOOST_INTRUSIVE_FORCEINLINE std::size_t operator()(const typename NodeTraits::node_ptr &n) const
   {  return NodeTraits::get_hash(n);  }
};

//Hash functor for node-level rehash operations: calculates the hash value of the value of the node.
//It's a temporary object that refers to the value traits and the hasher of the container.
template<class ValueTraits, class Hasher, class KeyOfValue>
struct hashtable_value_hasher
{
   BOOST_INTRUSIVE_FORCEINLINE hashtable_value_hasher(const ValueTraits &vtraits, Hasher &hasher)
      : vtraits_(vtraits), hasher_(hasher)
   {}

   BOOST_INTRUSIVE_FORCEINLINE std::size_t operator()(const typename ValueTraits::node_traits::node_ptr &n) const
   {  return hasher_(KeyOfValue()(*vtraits_.to_value_ptr(n)));  }

   private:
   hashtable_value_hasher &operator=(const hashtable_value_hasher &);

   const ValueTraits &vtraits_;
   Hasher &hasher_;
};

//hashtable_node_ops contains the bucket and node operations of bucket_plus_vtraits
//that don't depend on ValueTraits/BucketTraits
template<class NodeTraits, bool LinearBuckets>
struct hashtable_node_ops
{
   typedef NodeTraits                                    node_traits;
   typedef typename reduced_slist_node_traits
      <node_traits>::type                                reduced_node_traits;
   typedef bucket_impl<reduced_node_traits>              bucket_type;
   typedef typename pointer_traits
      <typename reduced_node_traits::node_ptr>
         ::template rebind_pointer<bucket_type>::type    bucket_ptr;
   typedef typename bucket_type::node_traits             slist_node_traits;
   typedef unordered_group_adapter<node_traits>          group_traits;
   typedef group_functions<node_traits>                  group_functions_t;
   typedef typename detail::if_c
      < LinearBuckets
      , linear_slist_algorithms<slist_node_traits>
      , circular_slist_algorithms<slist_node_traits>
      >::type                                            slist_node_algorithms;
   typedef typename slist_node_traits::node_ptr          slist_node_ptr;
   typedef trivial_value_traits
      <slist_node_traits, normal_link>                   slist_value_traits;
   typedef slist_iterator<slist_value_traits, false>     siterator;
   typedef typename node_traits::node_ptr                node_ptr;
   typedef typename node_traits::node                    node;
   typedef circular_slist_algorithms<group_traits>       group_algorithms;

   BOOST_INTRUSIVE_FORCEINLINE static siterator priv_bucket_lbegin(bucket_type &b)
   {  return siterator(slist_node_traits::get_next(b.get_node_ptr()));  }

   BOOST_INTRUSIVE_FORCEINLINE static siterator priv_bucket_lbbegin(bucket_type& b)
   {  return siterator(b.get_node_ptr());  }

   BOOST_INTRUSIVE_FORCEINLINE static siterator priv_bucket_lend(bucket_type& b)
   {  return siterator(slist_node_algorithms::end_node(b.get_node_ptr()));  }

   BOOST_INTRUSIVE_FORCEINLINE static std::size_t priv_bucket_size(const bucket_type& b)
   {  return slist_node_algorithms::count(b.get_node_ptr())-1u;  }

   BOOST_INTRUSIVE_FORCEINLINE static bool priv_bucket_empty(const bucket_type& b)
   {  return slist_node_algorithms::is_empty(b.get_node_ptr());  }

   template<class NodeDisposer>
   static std::size_t priv_erase_from_single_bucket
      (bucket_type &b, siterator sbefore_first, siterator slast, NodeDisposer node_disposer, detail::true_)   //optimize multikey
   {
      std::size_t n = 0;
      siterator const sfirst(++siterator(sbefore_first));
      if(sfirst != slast){
         node_ptr const nf = dcast_bucket_ptr<node>(sfirst.pointed_node());
         node_ptr const nl = dcast_bucket_ptr<node>(slast.pointed_node());
         slist_node_ptr const ne = (priv_bucket_lend(b)).pointed_node();

         if(group_functions_t::is_first_in_group(nf)) {
            // The first node is at the beginning of a group.
            if(nl != ne){
               group_functions_t::split_group(nl);
            }
         }
         else {
            node_ptr const group1 = group_functions_t::split_group(nf);
            if(nl != ne) {
               node_ptr const group2 = group_functions_t::split_group(nl);
               if(nf == group2) {   //Both first and last in the same group
                                    //so join group1 and group2
                  node_ptr const end1 = group_traits::get_next(group1);
                  //After splitting, nl is the first node of the second group
                  node_ptr const end2 = group_traits::get_next(nl);
                  group_traits::set_next(group1, end2);
                  group_traits::set_next(nl, end1);
               }
            }
         }

         n = slist_node_algorithms::unlink_after_and_dispose(sbefore_first.pointed_node(), slast.pointed_node(), node_disposer);
      }
      return n;
   }

   template<class NodeDisposer>
   BOOST_INTRUSIVE_FORCEINLINE static std::size_t priv_erase_from_single_bucket
      (bucket_type &, siterator sbefore_first, siterator slast, NodeDisposer node_disposer, detail::false_)   //optimize multikey
   {
      return slist_node_algorithms::unlink_after_and_dispose(sbefore_first.pointed_node(), slast.pointed_node(), node_disposer);
   }

   template<class NodeDisposer>
   static void priv_erase_node(bucket_type &b, siterator i, NodeDisposer node_disposer, detail::true_)   //optimize multikey
   {
      slist_node_ptr const ne(priv_bucket_lend(b).pointed_node());
      slist_node_ptr const nbb(priv_bucket_lbbegin(b).pointed_node());
      node_ptr n(dcast_bucket_ptr<node>(i.pointed_node()));
      node_ptr pos = node_traits::get_next(group_traits::get_next(n));
      node_ptr bn;
      node_ptr nn(node_traits::get_next(n));
      
      if(pos != n) {
         //Node is the first of the group
         bn = group_functions_t::get_prev_to_first_in_group(nbb, n);

         //Unlink the rest of the group if it's not the last node of its group
         if(nn != ne && group_traits::get_next(nn) == n){
            group_algorithms::unlink_after(nn);
         }
      }
      else if(nn != ne && group_traits::get_next(nn) == n){
         //Node is not the end of the group
         bn = group_traits::get_next(n);
         group_algorithms::unlink_after(nn);
      }
      else{
         //Node is the end of the group
         bn = group_traits::get_next(n);
         node_ptr const x(group_algorithms::get_previous_node(n));
         group_algorithms::unlink_after(x);
      }
      slist_node_algorithms::unlink_after_and_dispose(bn, node_disposer);
   }

   template<class NodeDisposer>
   inline static void priv_erase_node(bucket_type &b, siterator i, NodeDisposer node_disposer, detail::false_)   //!optimize multikey
   {
      slist_node_ptr bi = slist_node_algorithms::get_previous_node(b.get_node_ptr(), i.pointed_node());
      slist_node_algorithms::unlink_after_and_dispose(bi, node_disposer);
   }

   static siterator priv_get_last(bucket_type &b, detail::true_)  //optimize multikey
   {
      //First find the last node of p's group.
      //This requires checking the first node of the next group or
      //the bucket node.
      slist_node_ptr end_ptr(sit_end(b).pointed_node());
      slist_node_ptr last_node_group(b.get_node_ptr());
      slist_node_ptr possible_end(slist_node_traits::get_next(last_node_group));

      while(end_ptr != possible_end){
         last_node_group   = group_traits::get_next(dcast_bucket_ptr<node>(possible_end));
         possible_end      = slist_node_traits::get_next(last_node_group);
      }
      return siterator(last_node_group);
   }

   inline static siterator priv_get_last(bucket_type &b, detail::false_) //NOT optimize multikey
   {
      slist_node_ptr p = b.get_node_ptr();
      return siterator(slist_node_algorithms::get_previous_node(p, slist_node_algorithms::end_node(p)));
   }

   template<class NodeDisposer>
   BOOST_INTRUSIVE_FORCEINLINE static std::size_t priv_erase_whole_bucket(bucket_type &b, NodeDisposer node_disposer)
   {  return slist_node_algorithms::detach_and_dispose(b.get_node_ptr(), node_disposer);  }

   static siterator priv_get_previous(bucket_type &b, siterator i, detail::true_)   //optimize multikey
   {
      node_ptr const elem(dcast_bucket_ptr<node>(i.pointed_node()));
      node_ptr const prev_in_group(group_traits::get_next(elem));
      bool const first_in_group = node_traits::get_next(prev_in_group) != elem;
      slist_node_ptr n = first_in_group
         ? group_functions_t::get_prev_to_first_in_group(b.get_node_ptr(), elem)
         : group_traits::get_next(elem)
         ;
      return siterator(n);
   }

   BOOST_INTRUSIVE_FORCEINLINE static siterator priv_get_previous(bucket_type &b, siterator i, detail::false_)   //NOT optimize multikey
   {  return siterator(slist_node_algorithms::get_previous_node(b.get_node_ptr(), i.pointed_node()));   }

   BOOST_INTRUSIVE_FORCEINLINE static bucket_ptr to_ptr(bucket_type &b)
   {  return pointer_traits<bucket_ptr>::pointer_to(b);   }

   BOOST_INTRUSIVE_FORCEINLINE static siterator sit_bbegin(bucket_type& b)
   {  return siterator(b.get_node_ptr());  }

   BOOST_INTRUSIVE_FORCEINLINE static siterator sit_begin(bucket_type& b)
   {  return siterator(b.begin_ptr());  }

   BOOST_INTRUSIVE_FORCEINLINE static siterator sit_end(bucket_type& b)
   {  return siterator(slist_node_algorithms::end_node(b.get_node_ptr()));  }

   BOOST_INTRUSIVE_FORCEINLINE static std::size_t priv_stored_hash(siterator s, detail::true_) //store_hash
   {  return node_traits::get_hash(dcast_bucket_ptr<node>(s.pointed_node()));  }

   BOOST_INTRUSIVE_FORCEINLINE static std::size_t priv_stored_hash(siterator, detail::false_)  //NO store_hash
   {  return std::size_t(-1);   }

   static void priv_init_buckets(const bucket_ptr buckets_ptr, const std::size_t bucket_cnt)
   {
      bucket_ptr buckets_it = buckets_ptr;
      for (std::size_t bucket_i = 0; bucket_i != bucket_cnt; ++buckets_it, ++bucket_i) {
         slist_node_algorithms::init_header(buckets_it->get_node_ptr());
      }
   }

   //Bucket array operations. "buckets" is the bucket array and "usable_cnt" the
   //number of buckets that can hold elements (the sentinel bucket of linear
   //buckets is not included).

   //The bucket pointer is null in a moved-from container: convert it without dereferencing it
   BOOST_INTRUSIVE_FORCEINLINE static siterator priv_end_sit(bucket_ptr buckets, std::size_t usable_cnt)
   {
      (void)usable_cnt;
      BOOST_IF_CONSTEXPR(LinearBuckets)
         return siterator(buckets + std::ptrdiff_t(usable_cnt));
      else
         return siterator(pointer_traits<slist_node_ptr>::static_cast_from(buckets));
   }

   BOOST_INTRUSIVE_FORCEINLINE static bucket_ptr priv_invalid_bucket_ptr(bucket_ptr buckets, std::size_t usable_cnt)
   {
      (void)buckets; (void)usable_cnt;
      BOOST_IF_CONSTEXPR(LinearBuckets)
         return bucket_ptr();
      else
         return buckets + std::ptrdiff_t(usable_cnt);
   }

   //Prepares a bucket array of "bucket_cnt" buckets (the sentinel bucket of linear buckets
   //included) to be used by an empty container: marks the sentinel bucket and empties the other buckets.
   static void priv_init_bucket_array(bucket_ptr buckets, std::size_t bucket_cnt, bool power_2_buckets)
   {
      (void)power_2_buckets;
      const std::size_t usable_cnt = bucket_cnt - std::size_t(LinearBuckets && bucket_cnt != 0);
      BOOST_INTRUSIVE_INVARIANT_ASSERT(usable_cnt != 0);
      //Check power of two bucket array if the option is activated
      BOOST_INTRUSIVE_INVARIANT_ASSERT
         (!power_2_buckets || (0 == (usable_cnt & (usable_cnt - 1))));
      BOOST_IF_CONSTEXPR(LinearBuckets){
         priv_set_sentinel_bucket(buckets, bucket_cnt);
      }
      priv_init_buckets(buckets, usable_cnt);
   }

   //Linear buckets: marks the last bucket of the array ("bucket_cnt" is the total count)
   //as the sentinel that ends the iteration
   static void priv_set_sentinel_bucket(bucket_ptr buckets, std::size_t bucket_cnt)
   {
      BOOST_INTRUSIVE_INVARIANT_ASSERT(bucket_cnt > 1);
      slist_node_algorithms::set_sentinel(buckets[std::ptrdiff_t(bucket_cnt - 1u)].get_node_ptr());
   }

   static void priv_unset_sentinel_bucket(bucket_ptr buckets, std::size_t bucket_cnt)
   {
      //A moved-from container has no bucket array and no sentinel bucket
      if(!bucket_cnt)
         return;
      BOOST_INTRUSIVE_INVARIANT_ASSERT(bucket_cnt > 1);
      slist_node_algorithms::init_header(buckets[std::ptrdiff_t(bucket_cnt - 1u)].get_node_ptr());
   }

   //Unlinks all the nodes of "bucket_cnt" buckets. Nodes are initialized if the link mode requires it.
   template<bool SafeModeOrAutoUnlink>
   static void priv_clear_buckets(const bucket_ptr buckets_ptr, const std::size_t bucket_cnt)
   {
      //Same type as the node disposer that containers build from null_disposer, to share instantiations
      typedef node_cast_adaptor
         < detail::node_null_disposer<typename get_algo<CommonSListAlgorithms, node_traits>::type>
         , slist_node_ptr, node_ptr>                     init_disposer_t;
      bucket_ptr buckets_it = buckets_ptr;
      for(std::size_t bucket_i = 0; bucket_i != bucket_cnt; ++buckets_it, ++bucket_i){
         BOOST_IF_CONSTEXPR(SafeModeOrAutoUnlink){
            slist_node_algorithms::detach_and_dispose
               (buckets_it->get_node_ptr(), init_disposer_t(detail::null_disposer(), static_cast<const void*>(0)));
         }
         else{
            slist_node_algorithms::init_header(buckets_it->get_node_ptr());
         }
      }
   }

   //Returns the first element of the first non-empty bucket, searching from bucket "n",
   //and stores that bucket in "pbucketptr". If all buckets are empty, returns the end
   //iterator and stores the invalid bucket pointer.
   static siterator priv_begin_from(bucket_ptr buckets, std::size_t n, std::size_t usable_cnt, bucket_ptr &pbucketptr)
   {
      for (; n < usable_cnt; ++n){
         bucket_type &b = buckets[std::ptrdiff_t(n)];
         if(!slist_node_algorithms::is_empty(b.get_node_ptr())){
            pbucketptr = to_ptr(b);
            return siterator(b.begin_ptr());
         }
      }
      pbucketptr = priv_invalid_bucket_ptr(buckets, usable_cnt);
      return priv_end_sit(buckets, usable_cnt);
   }

   //Advances "cache" to the first non-empty bucket in [cache, cache_end)
   static void priv_skip_empty_buckets(bucket_ptr &cache, const bucket_ptr cache_end)
   {
      while( cache != cache_end) {
         if (!slist_node_algorithms::is_empty(cache->get_node_ptr())) {
            return;
         }
         ++cache;
      }
   }

   static std::size_t priv_count_elements(bucket_ptr buckets, std::size_t usable_cnt)
   {
      std::size_t len = 0;
      for (std::size_t n = 0; n < usable_cnt; ++n){
         len += slist_node_algorithms::count(buckets[std::ptrdiff_t(n)].get_node_ptr()) - 1u;
      }
      return len;
   }

   static bool priv_all_buckets_empty(bucket_ptr buckets, std::size_t usable_cnt)
   {
      for (std::size_t n = 0; n < usable_cnt; ++n){
         if(!slist_node_algorithms::is_empty(buckets[std::ptrdiff_t(n)].get_node_ptr())){
            return false;
         }
      }
      return true;
   }

   //Initializes the buckets of "new_buckets" that are not already initialized buckets of "old_buckets"
   static void priv_initialize_new_buckets
      ( bucket_ptr old_buckets, std::size_t old_bucket_count
      , bucket_ptr new_buckets, std::size_t new_bucket_count)
   {
      const bool same_buffer = old_buckets == new_buckets;
      if (same_buffer && new_bucket_count <= old_bucket_count) {
         //Nothing to do here
      }
      else if (same_buffer) {
         priv_init_buckets(old_buckets + std::ptrdiff_t(old_bucket_count), new_bucket_count - old_bucket_count);
      }
      else {
         priv_init_buckets(new_buckets, new_bucket_count);
      }
   }

   //Returns the number of the bucket that holds the node "sp", searching the bucket node
   //that ends the bucket. Used when the bucket can't be obtained from the stored hash.
   template<bool OptimizeMultikey>
   static std::size_t priv_bucket_num_from_node(bucket_ptr buckets, std::size_t usable_cnt, slist_node_ptr sp)
   {
      const bucket_type &f = buckets[0];
      slist_node_ptr bb = group_functions_t::get_bucket_before_begin
         ( sit_bbegin(buckets[0]).pointed_node()
         , sit_bbegin(buckets[std::ptrdiff_t(usable_cnt - 1u)]).pointed_node()
         , sp
         , detail::bool_<OptimizeMultikey>());

      //Now get the bucket_impl from the iterator
      const bucket_type &b = static_cast<const bucket_type&>(*bb);
      //Now just calculate the index b has in the bucket array
      return static_cast<std::size_t>(&b - &f);
   }

   //Links "n" after "prev", storing the hash value if the node stores it. If OptimizeMultikey
   //is true and "next_is_in_group" is true, "n" joins the group of the node that follows "prev",
   //otherwise "n" starts a new group.
   template<bool OptimizeMultikey, bool SafeModeOrAutoUnlink>
   static void priv_link_after(slist_node_ptr prev, node_ptr n, std::size_t hash_value, bool next_is_in_group)
   {
      BOOST_INTRUSIVE_SAFE_HOOK_DEFAULT_ASSERT(!SafeModeOrAutoUnlink || slist_node_algorithms::unique(n));
      node_functions<node_traits>::store_hash(n, hash_value, detail::bool_<store_hash_is_true<node_traits>::value>());
      group_functions_t::insert_in_group
         ( next_is_in_group ? dcast_bucket_ptr<node>(slist_node_traits::get_next(prev)) : n
         , n, detail::bool_<OptimizeMultikey>());
      slist_node_algorithms::link_after(prev, n);
   }

   //Erases [first, last), "first" being an element of bucket "first_bucket" and "last"
   //an element of bucket "last_bucket". If "last_is_end" is true, the range ends at the
   //end of the bucket "last_bucket" and "last" is not used. Returns the number of erased elements.
   template<class NodeDisposer, bool OptimizeMultikey>
   static std::size_t priv_erase_range
      ( bucket_ptr buckets
      , siterator first, std::size_t const first_bucket
      , siterator last,  std::size_t const last_bucket, bool const last_is_end
      , NodeDisposer node_disposer, detail::bool_<OptimizeMultikey> optimize_multikey_tag)
   {
      siterator const before_first
         = priv_get_previous(buckets[std::ptrdiff_t(first_bucket)], first, optimize_multikey_tag);
      if(last_is_end){
         last = sit_end(buckets[std::ptrdiff_t(last_bucket)]);
      }
      return priv_erase_node_range
         (buckets, before_first, first_bucket, last, last_bucket, node_disposer, optimize_multikey_tag);
   }

   template<class NodeDisposer, bool OptimizeMultikey>
   static std::size_t priv_erase_node_range
      ( bucket_ptr buckets
      , siterator const &before_first_it,  std::size_t const first_bucket
      , siterator const &last_it,          std::size_t const last_bucket
      , NodeDisposer node_disposer, detail::bool_<OptimizeMultikey> optimize_multikey_tag)
   {
      std::size_t num_erased(0);
      siterator last_step_before_it;
      if(first_bucket != last_bucket){
         bucket_type &fb = buckets[std::ptrdiff_t(first_bucket)];
         num_erased += priv_erase_from_single_bucket
            (fb, before_first_it, sit_end(fb), node_disposer, optimize_multikey_tag);
         for(std::size_t i = first_bucket + 1u; i != last_bucket; ++i){
            num_erased += priv_erase_whole_bucket(buckets[std::ptrdiff_t(i)], node_disposer);
         }
         last_step_before_it = sit_bbegin(buckets[std::ptrdiff_t(last_bucket)]);
      }
      else{
         last_step_before_it = before_first_it;
      }
      num_erased += priv_erase_from_single_bucket
                  (buckets[std::ptrdiff_t(last_bucket)], last_step_before_it, last_it, node_disposer, optimize_multikey_tag);
      return num_erased;
   }

   //Moves the nodes of buckets [old_first, old_cnt) of "old_buckets" to "new_buckets" (both arrays
   //can be the same buffer). "IndexOps" maps hashes to buckets of the new array. If "fast_shrink"
   //is true, the old bucket count is a multiple of the new one and whole buckets are moved without
   //hashing. Otherwise "node_hash" obtains the hash of each node and, if "store_new_hash" is true,
   //the hash is stored in the group of nodes. Returns the minimum of "new_first_bucket_num" and the
   //first new bucket that receives nodes.
   //
   //If "node_hash" throws, nodes already moved are in the new buckets and the rest in the old ones.
   template<bool OptimizeMultikey, class IndexOps, class NodeHash>
   static std::size_t priv_rehash_nodes
      ( bucket_ptr old_buckets, std::size_t old_first, std::size_t old_cnt
      , bucket_ptr new_buckets, std::size_t new_cnt, std::size_t split
      , bool fast_shrink, bool store_new_hash, std::size_t new_first_bucket_num, NodeHash node_hash)
   {
      typedef detail::bool_<OptimizeMultikey> optimize_multikey_t;
      typedef detail::bool_<store_hash_is_true<node_traits>::value> store_hash_t;
      (void)store_new_hash;
      const bool same_buffer = old_buckets == new_buckets;
      for(std::size_t n = old_first; n < old_cnt; ++n){
         bucket_type &old_bucket = old_buckets[std::ptrdiff_t(n)];
         if(!fast_shrink){
            siterator before_i(old_bucket.get_node_ptr());
            siterator i(before_i); ++i;
            siterator const end_sit(sit_end(old_bucket));
            for( //
               ; i != end_sit
               ; i = before_i, ++i){
               node_ptr const first_n = dcast_bucket_ptr<node>(i.pointed_node());
               const std::size_t hash_value = node_hash(first_n);
               const std::size_t new_n = IndexOps::hash_to_bucket(hash_value, new_cnt, split);

               //Update first used bucket
               if(new_n < new_first_bucket_num)
                  new_first_bucket_num = new_n;

               //Transfer the whole group
               siterator const last(group_functions_t::get_last_in_group(first_n, optimize_multikey_t()));

               //All nodes of the group have the same key, so store the new hash in all of them
               BOOST_IF_CONSTEXPR(store_hash_t::value){
                  if(store_new_hash){
                     for(siterator it = i; ; ++it){
                        node_functions<node_traits>::store_hash
                           (dcast_bucket_ptr<node>(it.pointed_node()), hash_value, store_hash_t());
                        if(it == last)
                           break;
                     }
                  }
               }

               if(same_buffer && new_n == n){
                  before_i = last;
               }
               else{
                  bucket_type &new_b = new_buckets[std::ptrdiff_t(new_n)];
                  slist_node_algorithms::transfer_after(new_b.get_node_ptr(), before_i.pointed_node(), last.pointed_node());
               }
            }
         }
         else{
            const std::size_t new_n = IndexOps::hash_to_bucket(n, new_cnt, split);
            //Empty buckets must not update the first used bucket
            if(new_n < new_first_bucket_num && !slist_node_algorithms::is_empty(old_bucket.get_node_ptr()))
               new_first_bucket_num = new_n;
            bucket_type &new_b = new_buckets[std::ptrdiff_t(new_n)];
            siterator const last = priv_get_last(old_bucket, optimize_multikey_t());
            slist_node_algorithms::transfer_after(new_b.get_node_ptr(), old_bucket.get_node_ptr(), last.pointed_node());
         }
      }
      return new_first_bucket_num;
   }

   //Incremental rehash: moves the nodes of bucket "bucket_to_rehash" whose hash ("node_hash")
   //maps to another bucket. "usable_cnt" and "split" are the values after the split count increment.
   template<bool OptimizeMultikey, class IndexOps, class NodeHash>
   static void priv_split_bucket
      (bucket_ptr buckets, std::size_t usable_cnt, std::size_t split, std::size_t bucket_to_rehash, NodeHash node_hash)
   {
      typedef detail::bool_<OptimizeMultikey> optimize_multikey_t;
      bucket_type &old_bucket = buckets[std::ptrdiff_t(bucket_to_rehash)];
      siterator before_i(old_bucket.get_node_ptr());
      siterator i(before_i); ++i;
      siterator const end_sit = LinearBuckets ? siterator() : before_i;
      for( ; i != end_sit; i = before_i, ++i){
         node_ptr const first_n = dcast_bucket_ptr<node>(i.pointed_node());
         const std::size_t new_n = IndexOps::hash_to_bucket(node_hash(first_n), usable_cnt, split);
         siterator const last(group_functions_t::get_last_in_group(first_n, optimize_multikey_t()));
         if(new_n == bucket_to_rehash){
            before_i = last;
         }
         else{
            bucket_type &new_b = buckets[std::ptrdiff_t(new_n)];
            slist_node_algorithms::transfer_after(new_b.get_node_ptr(), before_i.pointed_node(), last.pointed_node());
         }
      }
   }
};

//bucket_plus_vtraits stores ValueTraits + BucketTraits
template<class ValueTraits, class BucketTraits, bool LinearBuckets>
struct bucket_plus_vtraits
{
   private:
   BOOST_MOVABLE_BUT_NOT_COPYABLE(bucket_plus_vtraits)


   struct BOOST_INTRUSIVE_EMPTY_BASES data_type
      : public ValueTraits, BucketTraits
   {
      private:
      BOOST_MOVABLE_BUT_NOT_COPYABLE(data_type)

      public:
      BOOST_INTRUSIVE_FORCEINLINE data_type(const ValueTraits& val_traits, const BucketTraits& b_traits)
         : ValueTraits(val_traits), BucketTraits(b_traits)
      {}

      BOOST_INTRUSIVE_FORCEINLINE data_type(BOOST_RV_REF(data_type) other)
         : ValueTraits (BOOST_MOVE_BASE(ValueTraits,  other))
         , BucketTraits(BOOST_MOVE_BASE(BucketTraits, other))
      {}
   } m_data;

   public:
   typedef BucketTraits bucket_traits;
   typedef ValueTraits  value_traits;

   static const bool safemode_or_autounlink = is_safe_autounlink<value_traits::link_mode>::value;

   typedef typename unordered_bucket_impl
      <value_traits>::type                               bucket_type;
   typedef typename unordered_bucket_ptr_impl
      <value_traits>::type                               bucket_ptr;
   typedef typename value_traits::node_traits            node_traits;
   typedef typename bucket_type::node_traits             slist_node_traits;
   typedef unordered_group_adapter<node_traits>          group_traits;
   typedef group_functions<node_traits>                  group_functions_t;
   typedef typename detail::if_c
      < LinearBuckets
      , linear_slist_algorithms<slist_node_traits>
      , circular_slist_algorithms<slist_node_traits>
      >::type                                            slist_node_algorithms;

   typedef typename slist_node_traits::node_ptr          slist_node_ptr;
   typedef trivial_value_traits
      <slist_node_traits, normal_link>                   slist_value_traits;
   typedef slist_iterator<slist_value_traits, false>     siterator;
   typedef slist_iterator<slist_value_traits, true>      const_siterator;

   typedef typename node_traits::node_ptr                node_ptr;
   typedef typename node_traits::const_node_ptr          const_node_ptr;
   typedef typename node_traits::node                    node;
   typedef typename value_traits::value_type             value_type;
   typedef typename value_traits::pointer                pointer;
   typedef typename value_traits::const_pointer          const_pointer;
   typedef typename pointer_traits<pointer>::reference   reference;
   typedef typename pointer_traits
      <const_pointer>::reference                         const_reference;
   typedef circular_slist_algorithms<group_traits>       group_algorithms;
   typedef typename pointer_traits
      <typename value_traits::pointer>::
         template rebind_pointer
            <const value_traits>::type                   const_value_traits_ptr;
   typedef typename pointer_traits
      <typename value_traits::pointer>::
         template rebind_pointer
            <const bucket_plus_vtraits>::type            const_bucket_value_traits_ptr;
   typedef detail::bool_<LinearBuckets>                  linear_buckets_t;
   typedef bucket_plus_vtraits&                          this_ref;

   static const std::size_t bucket_overhead = LinearBuckets ? 1u : 0u;

   typedef hashtable_node_ops<node_traits, LinearBuckets> node_ops_t;

   BOOST_INTRUSIVE_FORCEINLINE bucket_plus_vtraits(const ValueTraits &val_traits, const bucket_traits &b_traits)
      : m_data(val_traits, b_traits)
   {}

   BOOST_INTRUSIVE_FORCEINLINE bucket_plus_vtraits(BOOST_RV_REF(bucket_plus_vtraits) other)
      : m_data(boost::move(((bucket_plus_vtraits&)other).m_data))
   {}

   BOOST_INTRUSIVE_FORCEINLINE const_value_traits_ptr priv_value_traits_ptr() const
   {  return pointer_traits<const_value_traits_ptr>::pointer_to(this->priv_value_traits());  }

   //bucket_value_traits
   //
   BOOST_INTRUSIVE_FORCEINLINE const bucket_plus_vtraits &get_bucket_value_traits() const
   {  return *this;  }

   BOOST_INTRUSIVE_FORCEINLINE bucket_plus_vtraits &get_bucket_value_traits()
   {  return *this;  }

   BOOST_INTRUSIVE_FORCEINLINE const_bucket_value_traits_ptr bucket_value_traits_ptr() const
   {  return pointer_traits<const_bucket_value_traits_ptr>::pointer_to(this->get_bucket_value_traits());  }

   //value traits
   //
   BOOST_INTRUSIVE_FORCEINLINE const value_traits &priv_value_traits() const
   {  return static_cast<const value_traits &>(this->m_data);  }

   BOOST_INTRUSIVE_FORCEINLINE value_traits &priv_value_traits()
   {  return static_cast<value_traits &>(this->m_data);  }

   //value traits
   //
   BOOST_INTRUSIVE_FORCEINLINE const bucket_traits &priv_bucket_traits() const
   {  return static_cast<const bucket_traits &>(this->m_data);  }

   BOOST_INTRUSIVE_FORCEINLINE bucket_traits& priv_bucket_traits()
   {  return static_cast<bucket_traits&>(this->m_data);  }

   //bucket operations
   BOOST_INTRUSIVE_FORCEINLINE bucket_ptr priv_bucket_pointer() const BOOST_NOEXCEPT
   {  return this->priv_bucket_traits().bucket_begin();  }

   inline std::size_t priv_usable_bucket_count() const BOOST_NOEXCEPT
   {
      BOOST_IF_CONSTEXPR(bucket_overhead){
         const std::size_t n = this->priv_bucket_traits().bucket_count();
         return n - std::size_t(n != 0)*bucket_overhead;
      }
      else{
         return this->priv_bucket_traits().bucket_count();
      }
   }

   inline bucket_type &priv_bucket(std::size_t n) const BOOST_NOEXCEPT
   {
      BOOST_INTRUSIVE_INVARIANT_ASSERT(n < this->priv_usable_bucket_count());
      return this->priv_bucket_pointer()[std::ptrdiff_t(n)];
   }

   BOOST_INTRUSIVE_FORCEINLINE bucket_ptr priv_bucket_ptr(std::size_t n) const BOOST_NOEXCEPT
   {  return pointer_traits<bucket_ptr>::pointer_to(this->priv_bucket(n)); }

   BOOST_INTRUSIVE_FORCEINLINE bucket_ptr priv_past_usable_bucket_ptr() const
   {  return this->priv_bucket_pointer() + std::ptrdiff_t(priv_usable_bucket_count()); }

   BOOST_INTRUSIVE_FORCEINLINE bucket_ptr priv_invalid_bucket_ptr() const
   {
      BOOST_IF_CONSTEXPR(LinearBuckets) {
         return bucket_ptr();
      }
      else{
         return this->priv_past_usable_bucket_ptr();
      }
   }

   BOOST_INTRUSIVE_FORCEINLINE void priv_set_sentinel_bucket() const
   {
      BOOST_IF_CONSTEXPR(LinearBuckets) {
         node_ops_t::priv_set_sentinel_bucket(this->priv_bucket_pointer(), this->priv_bucket_traits().bucket_count());
      }
   }

   BOOST_INTRUSIVE_FORCEINLINE void priv_unset_sentinel_bucket() const
   {
      BOOST_IF_CONSTEXPR(LinearBuckets) {
         node_ops_t::priv_unset_sentinel_bucket(this->priv_bucket_pointer(), this->priv_bucket_traits().bucket_count());
      }
   }

   BOOST_INTRUSIVE_FORCEINLINE siterator priv_end_sit() const
   {  return priv_end_sit(linear_buckets_t());  }

   BOOST_INTRUSIVE_FORCEINLINE siterator priv_end_sit(detail::true_) const
   {  return siterator(this->priv_bucket_pointer() + std::ptrdiff_t(this->priv_usable_bucket_count())); }

   //The bucket pointer is null in a moved-from container: convert it without dereferencing it
   BOOST_INTRUSIVE_FORCEINLINE siterator priv_end_sit(detail::false_) const
   {  return siterator(pointer_traits<slist_node_ptr>::static_cast_from(this->priv_bucket_pointer()));  }

   BOOST_INTRUSIVE_FORCEINLINE siterator priv_bucket_lbegin(std::size_t n) const
   {  siterator s(this->priv_bucket_lbbegin(n)); return ++s; }

   BOOST_INTRUSIVE_FORCEINLINE siterator priv_bucket_lbbegin(std::size_t n) const
   {  return node_ops_t::sit_bbegin(this->priv_bucket(n));  }

   BOOST_INTRUSIVE_FORCEINLINE siterator priv_bucket_lend(std::size_t n) const
   {  return node_ops_t::sit_end(this->priv_bucket(n));  }

   BOOST_INTRUSIVE_FORCEINLINE std::size_t priv_bucket_size(std::size_t n) const
   {  return slist_node_algorithms::count(this->priv_bucket(n).get_node_ptr())-1u;  }

   BOOST_INTRUSIVE_FORCEINLINE bool priv_bucket_empty(std::size_t n) const
   {  return slist_node_algorithms::is_empty(this->priv_bucket(n).get_node_ptr());  }

   BOOST_INTRUSIVE_FORCEINLINE bool priv_bucket_empty(bucket_ptr p) const
   {  return slist_node_algorithms::is_empty(p->get_node_ptr());  }

   template<class Disposer>
   struct typeof_node_disposer
   {
      typedef node_cast_adaptor
         < typename detail::get_node_disposer<Disposer, value_traits, CommonSListAlgorithms>::type
         , slist_node_ptr, node_ptr > type;
   };

   template<class Disposer>
   inline typename typeof_node_disposer<Disposer>::type
      make_node_disposer(const Disposer &disposer) const
   {
      typedef typename typeof_node_disposer<Disposer>::type return_t;
      return return_t(disposer, &this->priv_value_traits());
   }

   BOOST_INTRUSIVE_FORCEINLINE node &priv_value_to_node(reference v)
   {  return *this->priv_value_traits().to_node_ptr(v);  }

   BOOST_INTRUSIVE_FORCEINLINE const node &priv_value_to_node(const_reference v) const
   {  return *this->priv_value_traits().to_node_ptr(v);  }

   BOOST_INTRUSIVE_FORCEINLINE node_ptr priv_value_to_node_ptr(reference v)
   {  return this->priv_value_traits().to_node_ptr(v);  }

   BOOST_INTRUSIVE_FORCEINLINE const_node_ptr priv_value_to_node_ptr(const_reference v) const
   {  return this->priv_value_traits().to_node_ptr(v);  }

   BOOST_INTRUSIVE_FORCEINLINE reference priv_value_from_siterator(siterator s)
   {  return *this->priv_value_traits().to_value_ptr(dcast_bucket_ptr<node>(s.pointed_node())); }

   BOOST_INTRUSIVE_FORCEINLINE const_reference priv_value_from_siterator(siterator s) const
   {  return *this->priv_value_traits().to_value_ptr(dcast_bucket_ptr<node>(s.pointed_node())); }

   BOOST_INTRUSIVE_FORCEINLINE std::size_t priv_stored_or_compute_hash(const value_type &v, detail::true_) const   //For store_hash == true
   {  return node_traits::get_hash(this->priv_value_traits().to_node_ptr(v));  }

   typedef hashtable_iterator<bucket_plus_vtraits, LinearBuckets, false>          iterator;
   typedef hashtable_iterator<bucket_plus_vtraits, LinearBuckets, true>           const_iterator;

   BOOST_INTRUSIVE_FORCEINLINE iterator end() BOOST_NOEXCEPT
   {  return this->build_iterator(this->priv_end_sit(), bucket_ptr());   }

   BOOST_INTRUSIVE_FORCEINLINE const_iterator end() const BOOST_NOEXCEPT
   {  return this->cend();   }

   BOOST_INTRUSIVE_FORCEINLINE const_iterator cend() const BOOST_NOEXCEPT
   {  return this->build_const_iterator(this->priv_end_sit(), bucket_ptr());   }

   BOOST_INTRUSIVE_FORCEINLINE iterator build_iterator(siterator s, bucket_ptr p)
   {  return this->build_iterator(s, p, linear_buckets_t());  }

   BOOST_INTRUSIVE_FORCEINLINE iterator build_iterator(siterator s, bucket_ptr p, detail::true_)   //linear buckets
   {  return iterator(s, p, this->priv_value_traits_ptr());  }

   BOOST_INTRUSIVE_FORCEINLINE iterator build_iterator(siterator s, bucket_ptr, detail::false_)    //!linear buckets
   {  return iterator(s, &this->get_bucket_value_traits());  }

   BOOST_INTRUSIVE_FORCEINLINE const_iterator build_const_iterator(siterator s, bucket_ptr p) const
   {  return this->build_const_iterator(s, p, linear_buckets_t());  }

   BOOST_INTRUSIVE_FORCEINLINE const_iterator build_const_iterator(siterator s, bucket_ptr p, detail::true_) const   //linear buckets
   {  return const_iterator(s, p, this->priv_value_traits_ptr());  }

   BOOST_INTRUSIVE_FORCEINLINE const_iterator build_const_iterator(siterator s, bucket_ptr, detail::false_) const   //!linear buckets
   {  return const_iterator(s, &this->get_bucket_value_traits());  }
};

template<class Hash, class>
struct get_hash
{
   typedef Hash type;
};

template<class T>
struct get_hash<void, T>
{
   typedef detail::internal_hash_functor<T> type;
};

template<class EqualTo, class>
struct get_equal_to
{
   typedef EqualTo type;
};

template<class T>
struct get_equal_to<void, T>
{
   typedef value_equal<T> type;
};

template<class KeyOfValue, class T>
struct get_hash_key_of_value
{
   typedef KeyOfValue type;
};

template<class T>
struct get_hash_key_of_value<void, T>
{
   typedef ::boost::intrusive::detail::identity<T> type;
};

template<class T, class VoidOrKeyOfValue>
struct hash_key_types_base
{
   typedef typename get_hash_key_of_value
      < VoidOrKeyOfValue, T>::type           key_of_value;
   typedef typename key_of_value::type   key_type;
};

template<class T, class VoidOrKeyOfValue, class VoidOrKeyHash>
struct hash_key_hash
   : get_hash
      < VoidOrKeyHash
      , typename hash_key_types_base<T, VoidOrKeyOfValue>::key_type
      >
{};

template<class T, class VoidOrKeyOfValue, class VoidOrKeyEqual>
struct hash_key_equal
   : get_equal_to
      < VoidOrKeyEqual
      , typename hash_key_types_base<T, VoidOrKeyOfValue>::key_type
      >

{};

struct hashtable_hasher_tag;
struct hashtable_equal_tag;

//bucket_hash_equal_t
//Stores the hash function, bucket_plus_vtraits and the equality function.
//The hash function is the first base and the equality function the last one
//so that compilers without __declspec(empty_bases) don't add padding.
template<class ValueTraits, class VoidOrKeyOfValue, class VoidOrKeyHash, class VoidOrKeyEqual, class BucketTraits, bool LinearBuckets>
struct BOOST_INTRUSIVE_EMPTY_BASES bucket_hash_equal_t
   //Use public inheritance to avoid MSVC bugs with closures
   : public detail::ebo_functor_holder
      < typename hash_key_hash<typename ValueTraits::value_type, VoidOrKeyOfValue, VoidOrKeyHash>::type
      , hashtable_hasher_tag>
   , public bucket_plus_vtraits<ValueTraits, BucketTraits, LinearBuckets>
   , public detail::ebo_functor_holder
      < typename hash_key_equal<typename ValueTraits::value_type, VoidOrKeyOfValue, VoidOrKeyEqual>::type
      , hashtable_equal_tag>
{
   private:
   BOOST_MOVABLE_BUT_NOT_COPYABLE(bucket_hash_equal_t)

   public:
   typedef ValueTraits                                                              value_traits;
   typedef typename value_traits::value_type                                        value_type;
   typedef typename hash_key_hash
      <value_type, VoidOrKeyOfValue, VoidOrKeyHash>::type                           hasher;
   typedef typename hash_key_equal
      <value_type, VoidOrKeyOfValue, VoidOrKeyEqual>::type                          key_equal;
   typedef typename hash_key_types_base<value_type, VoidOrKeyOfValue>::key_of_value key_of_value;
   typedef detail::ebo_functor_holder<hasher, hashtable_hasher_tag>                 hasher_holder_t;
   typedef detail::ebo_functor_holder<key_equal, hashtable_equal_tag>               equal_holder_t;
   typedef BucketTraits                                                             bucket_traits;
   typedef bucket_plus_vtraits<ValueTraits, BucketTraits, LinearBuckets>            bucket_plus_vtraits_t;

   BOOST_INTRUSIVE_FORCEINLINE bucket_hash_equal_t
      (const ValueTraits &val_traits, const bucket_traits &b_traits, const hasher & h, const key_equal &e)
      : hasher_holder_t(h)
      , bucket_plus_vtraits_t(val_traits, b_traits)
      , equal_holder_t(e)
   {}

   BOOST_INTRUSIVE_FORCEINLINE bucket_hash_equal_t(BOOST_RV_REF(bucket_hash_equal_t) other)
      : hasher_holder_t(BOOST_MOVE_BASE(hasher_holder_t, other))
      , bucket_plus_vtraits_t(BOOST_MOVE_BASE(bucket_plus_vtraits_t, other))
      , equal_holder_t(BOOST_MOVE_BASE(equal_holder_t, other))
   {}

   template<class K>
   BOOST_INTRUSIVE_FORCEINLINE std::size_t priv_hash(const K &k) const
   {  return this->hasher_holder_t::operator()(k);  }

   BOOST_INTRUSIVE_FORCEINLINE const hasher &priv_hasher() const
   {  return this->hasher_holder_t::get();  }

   BOOST_INTRUSIVE_FORCEINLINE hasher &priv_hasher()
   {  return this->hasher_holder_t::get();  }

   BOOST_INTRUSIVE_FORCEINLINE const key_equal &priv_equal() const
   {  return this->equal_holder_t::get();  }

   BOOST_INTRUSIVE_FORCEINLINE key_equal &priv_equal()
   {  return this->equal_holder_t::get();  }

   using bucket_plus_vtraits_t::priv_stored_or_compute_hash;   //For store_hash == true

   BOOST_INTRUSIVE_FORCEINLINE std::size_t priv_stored_or_compute_hash(const value_type &v, detail::false_) const  //For store_hash == false
   {  return this->priv_hasher()(key_of_value()(v));   }
};

//Optional members of the hashtable: the first non-empty bucket (only if cache_begin<true>)
//and the split count (only if incremental<true> or fastmod_buckets<true>).
//If the first non-empty bucket is not cached, get_cache returns the first bucket.
template<class BucketPtr, class SizeType, bool CacheBegin, bool HasSplit>
struct hashtable_state_members
{
   typedef detail::size_holder<true, SizeType>  split_traits;
   typedef split_traits &                       split_traits_t;
   typedef const split_traits &                 split_traits_const_t;

   BOOST_INTRUSIVE_FORCEINLINE BucketPtr get_cache(BucketPtr) const
   {  return cached_begin_;  }

   BOOST_INTRUSIVE_FORCEINLINE void set_cache(BucketPtr p)
   {  cached_begin_ = p;  }

   BOOST_INTRUSIVE_FORCEINLINE split_traits_t get_split_traits()
   {  return split_;  }

   BOOST_INTRUSIVE_FORCEINLINE split_traits_const_t get_split_traits() const
   {  return split_;  }

   BucketPtr      cached_begin_;
   split_traits   split_;
};

template<class BucketPtr, class SizeType>
struct hashtable_state_members<BucketPtr, SizeType, true, false>
{
   typedef detail::size_holder<false, SizeType> split_traits;
   typedef split_traits                         split_traits_t;
   typedef split_traits                         split_traits_const_t;

   BOOST_INTRUSIVE_FORCEINLINE BucketPtr get_cache(BucketPtr) const
   {  return cached_begin_;  }

   BOOST_INTRUSIVE_FORCEINLINE void set_cache(BucketPtr p)
   {  cached_begin_ = p;  }

   BOOST_INTRUSIVE_FORCEINLINE split_traits_t get_split_traits() const
   {  return split_traits();  }

   BucketPtr      cached_begin_;
};

template<class BucketPtr, class SizeType>
struct hashtable_state_members<BucketPtr, SizeType, false, true>
{
   typedef detail::size_holder<true, SizeType>  split_traits;
   typedef split_traits &                       split_traits_t;
   typedef const split_traits &                 split_traits_const_t;

   BOOST_INTRUSIVE_FORCEINLINE BucketPtr get_cache(BucketPtr first_bucket) const
   {  return first_bucket;  }

   BOOST_INTRUSIVE_FORCEINLINE void set_cache(BucketPtr)
   {}

   BOOST_INTRUSIVE_FORCEINLINE split_traits_t get_split_traits()
   {  return split_;  }

   BOOST_INTRUSIVE_FORCEINLINE split_traits_const_t get_split_traits() const
   {  return split_;  }

   split_traits   split_;
};

template<class BucketPtr, class SizeType>
struct hashtable_state_members<BucketPtr, SizeType, false, false>
{
   typedef detail::size_holder<false, SizeType> split_traits;
   typedef split_traits                         split_traits_t;
   typedef split_traits                         split_traits_const_t;

   BOOST_INTRUSIVE_FORCEINLINE BucketPtr get_cache(BucketPtr first_bucket) const
   {  return first_bucket;  }

   BOOST_INTRUSIVE_FORCEINLINE void set_cache(BucketPtr)
   {}

   BOOST_INTRUSIVE_FORCEINLINE split_traits_t get_split_traits() const
   {  return split_traits();  }
};

//Adds hashtable_state_members to Base as a data member, only if it has data,
//to maintain minimal container size with compilers like MSVC
//that have problems with EBO and multiple empty base classes
template<class Base, class BucketPtr, class SizeType, bool CacheBegin, bool HasSplit>
struct hashtable_state_wrapper
   : public Base
{
   private:
   BOOST_MOVABLE_BUT_NOT_COPYABLE(hashtable_state_wrapper)

   public:
   typedef hashtable_state_members<BucketPtr, SizeType, CacheBegin, HasSplit>  state_members_t;

   template<class Arg0, class Arg1, class Arg2, class Arg3>
   BOOST_INTRUSIVE_FORCEINLINE hashtable_state_wrapper
      ( const Arg0 &arg0, const Arg1 &arg1, const Arg2 &arg2, const Arg3 &arg3)
      :  Base(arg0, arg1, arg2, arg3)
   {}

   BOOST_INTRUSIVE_FORCEINLINE hashtable_state_wrapper(BOOST_RV_REF(hashtable_state_wrapper) other)
      : Base(BOOST_MOVE_BASE(Base, other))
   {}

   BOOST_INTRUSIVE_FORCEINLINE state_members_t &priv_state()
   {  return state_;  }

   BOOST_INTRUSIVE_FORCEINLINE const state_members_t &priv_state() const
   {  return state_;  }

   private:
   state_members_t state_;
};

template<class Base, class BucketPtr, class SizeType>
struct hashtable_state_wrapper<Base, BucketPtr, SizeType, false, false>
   : public Base
{
   private:
   BOOST_MOVABLE_BUT_NOT_COPYABLE(hashtable_state_wrapper)

   public:
   typedef hashtable_state_members<BucketPtr, SizeType, false, false>  state_members_t;

   template<class Arg0, class Arg1, class Arg2, class Arg3>
   BOOST_INTRUSIVE_FORCEINLINE hashtable_state_wrapper
      ( const Arg0 &arg0, const Arg1 &arg1, const Arg2 &arg2, const Arg3 &arg3)
      :  Base(arg0, arg1, arg2, arg3)
   {}

   BOOST_INTRUSIVE_FORCEINLINE hashtable_state_wrapper(BOOST_RV_REF(hashtable_state_wrapper) other)
      : Base(BOOST_MOVE_BASE(Base, other))
   {}

   BOOST_INTRUSIVE_FORCEINLINE state_members_t priv_state() const
   {  return state_members_t();  }
};

//This wrapper around size_traits is used
//to maintain minimal container size with compilers like MSVC
//that have problems with EBO and multiple empty base classes
template<class DeriveFrom, class SizeType, bool>
struct hashtable_size_wrapper
   : public DeriveFrom
{
   private:
   BOOST_MOVABLE_BUT_NOT_COPYABLE(hashtable_size_wrapper)

   public:
   template<class Base, class Arg0, class Arg1, class Arg2>
   hashtable_size_wrapper( BOOST_FWD_REF(Base) base, BOOST_FWD_REF(Arg0) arg0
                         , BOOST_FWD_REF(Arg1) arg1, BOOST_FWD_REF(Arg2) arg2)
      :  DeriveFrom( ::boost::forward<Base>(base)
                   , ::boost::forward<Arg0>(arg0)
                   , ::boost::forward<Arg1>(arg1)
                   , ::boost::forward<Arg2>(arg2))
   {}
   typedef detail::size_holder < true, SizeType> size_traits;//size_traits

   BOOST_INTRUSIVE_FORCEINLINE hashtable_size_wrapper(BOOST_RV_REF(hashtable_size_wrapper) other)
      : DeriveFrom(BOOST_MOVE_BASE(DeriveFrom, other))
   {}

   size_traits size_traits_;

   typedef const size_traits & size_traits_const_t;
   typedef       size_traits & size_traits_t;

   BOOST_INTRUSIVE_FORCEINLINE size_traits_t priv_size_traits()
   {  return size_traits_; }

   BOOST_INTRUSIVE_FORCEINLINE size_traits_const_t priv_size_traits() const
   {  return size_traits_; }
};

template<class DeriveFrom, class SizeType>
struct hashtable_size_wrapper<DeriveFrom, SizeType, false>
   : public DeriveFrom
{
   private:
   BOOST_MOVABLE_BUT_NOT_COPYABLE(hashtable_size_wrapper)

   public:
   template<class Base, class Arg0, class Arg1, class Arg2>
   hashtable_size_wrapper( BOOST_FWD_REF(Base) base, BOOST_FWD_REF(Arg0) arg0
                         , BOOST_FWD_REF(Arg1) arg1, BOOST_FWD_REF(Arg2) arg2)
      :  DeriveFrom( ::boost::forward<Base>(base)
                   , ::boost::forward<Arg0>(arg0)
                   , ::boost::forward<Arg1>(arg1)
                   , ::boost::forward<Arg2>(arg2))
   {}

   BOOST_INTRUSIVE_FORCEINLINE hashtable_size_wrapper(BOOST_RV_REF(hashtable_size_wrapper) other)
      : DeriveFrom(BOOST_MOVE_BASE(DeriveFrom, other))
   {}

   typedef detail::size_holder< false, SizeType>   size_traits;

   typedef size_traits size_traits_const_t;
   typedef size_traits size_traits_t;

   BOOST_INTRUSIVE_FORCEINLINE size_traits priv_size_traits() const
   {  return size_traits(); }
};

template< class ValueTraits,    class VoidOrKeyOfValue, class VoidOrKeyHash
        , class VoidOrKeyEqual, class BucketTraits,     class SizeType
        , std::size_t BoolFlags>
struct get_hashtable_state_wrapper
{
   typedef hashtable_state_wrapper
      < bucket_hash_equal_t
         < ValueTraits, VoidOrKeyOfValue, VoidOrKeyHash, VoidOrKeyEqual
         , BucketTraits
         , 0 != (BoolFlags & hash_bool_flags::linear_buckets_pos)
         >
      , typename unordered_bucket_ptr_impl<ValueTraits>::type
      , SizeType
      , 0 != (BoolFlags & hash_bool_flags::cache_begin_pos)
      , (BoolFlags & hash_bool_flags::incremental_pos)     != 0 ||
        (BoolFlags & hash_bool_flags::fastmod_buckets_pos) != 0
      > type;
};

//hashdata_internal
//Stores bucket_hash_equal_t, the cached first non-empty bucket and the split count
template<class ValueTraits, class VoidOrKeyOfValue, class VoidOrKeyHash, class VoidOrKeyEqual, class BucketTraits, class SizeType, std::size_t BoolFlags>
struct hashdata_internal
   : public get_hashtable_state_wrapper
               <ValueTraits, VoidOrKeyOfValue, VoidOrKeyHash, VoidOrKeyEqual, BucketTraits, SizeType, BoolFlags>::type
{
   private:
   BOOST_MOVABLE_BUT_NOT_COPYABLE(hashdata_internal)

   public:
   static const bool linear_buckets = 0 != (BoolFlags & hash_bool_flags::linear_buckets_pos);
   static const bool cache_begin = 0 != (BoolFlags & hash_bool_flags::cache_begin_pos);
   typedef typename get_hashtable_state_wrapper
      <ValueTraits, VoidOrKeyOfValue, VoidOrKeyHash, VoidOrKeyEqual, BucketTraits, SizeType, BoolFlags>::type split_bucket_hash_equal_t;

   typedef typename split_bucket_hash_equal_t::key_equal                key_equal;
   typedef typename split_bucket_hash_equal_t::hasher                   hasher;
   typedef bucket_plus_vtraits
      <ValueTraits, BucketTraits, linear_buckets>           bucket_plus_vtraits_t;
   typedef typename bucket_plus_vtraits_t::node_ops_t       node_ops_t;
   typedef SizeType                                         size_type;
   typedef typename split_bucket_hash_equal_t::state_members_t          state_members_t;
   typedef typename state_members_t::split_traits                       split_traits;
   typedef typename bucket_plus_vtraits_t::bucket_ptr       bucket_ptr;
   typedef typename bucket_plus_vtraits_t::const_value_traits_ptr   const_value_traits_ptr;
   typedef typename bucket_plus_vtraits_t::siterator        siterator;
   typedef typename bucket_plus_vtraits_t::bucket_traits    bucket_traits;
   typedef typename bucket_plus_vtraits_t::value_traits     value_traits;
   typedef typename bucket_plus_vtraits_t::bucket_type      bucket_type;
   typedef typename value_traits::value_type                value_type;
   typedef typename value_traits::pointer                   pointer;
   typedef typename value_traits::const_pointer             const_pointer;
   typedef typename pointer_traits<pointer>::reference      reference;
   typedef typename pointer_traits
      <const_pointer>::reference                            const_reference;
   typedef typename value_traits::node_traits               node_traits;
   typedef typename node_traits::node                       node;
   typedef typename node_traits::node_ptr                   node_ptr;
   typedef typename node_traits::const_node_ptr             const_node_ptr;
   typedef typename bucket_plus_vtraits_t::slist_node_algorithms  slist_node_algorithms;
   typedef typename bucket_plus_vtraits_t::slist_node_ptr   slist_node_ptr;

   typedef hash_key_types_base
      < typename ValueTraits::value_type
      , VoidOrKeyOfValue
      >                                                              hash_types_base;
   typedef typename hash_types_base::key_of_value                    key_of_value;

   static const bool store_hash = store_hash_is_true<node_traits>::value;
   static const bool safemode_or_autounlink = is_safe_autounlink<value_traits::link_mode>::value;
   static const bool stateful_value_traits = detail::is_stateful_value_traits<value_traits>::value;

   typedef detail::bool_<store_hash>                                 store_hash_t;

   typedef detail::transform_iterator
      < siterator
      , downcast_node_to_value_t<value_traits, false> >              local_iterator;

   typedef detail::transform_iterator
      < siterator
      , downcast_node_to_value_t<value_traits, true> >               const_local_iterator;

   typedef detail::bool_<linear_buckets>                             linear_buckets_t;

   BOOST_INTRUSIVE_FORCEINLINE hashdata_internal( const ValueTraits &val_traits, const bucket_traits &b_traits
                    , const hasher & h, const key_equal &e)
      : split_bucket_hash_equal_t(val_traits, b_traits, h, e)
   {}

   BOOST_INTRUSIVE_FORCEINLINE hashdata_internal(BOOST_RV_REF(hashdata_internal) other)
      : split_bucket_hash_equal_t(BOOST_MOVE_BASE(split_bucket_hash_equal_t, other))
   {}

   BOOST_INTRUSIVE_FORCEINLINE typename state_members_t::split_traits_t priv_split_traits()
   {  return this->priv_state().get_split_traits();  }

   BOOST_INTRUSIVE_FORCEINLINE typename state_members_t::split_traits_const_t priv_split_traits() const
   {  return this->priv_state().get_split_traits();  }

   ~hashdata_internal()
   #if defined(BOOST_INTRUSIVE_CONCEPTS_BASED_OVERLOADING)
      requires (ValueTraits::link_mode != normal_link)
   #endif
   {
      BOOST_IF_CONSTEXPR(safemode_or_autounlink){
         this->priv_clear_buckets();
      }
   }

   #if !defined(BOOST_INTRUSIVE_DOXYGEN_INVOKED) && defined(BOOST_INTRUSIVE_CONCEPTS_BASED_OVERLOADING)
   //Default destructor for normal links (allows conditional triviality)
   ~hashdata_internal() requires (ValueTraits::link_mode == normal_link) = default;
   #endif

   void priv_clear_buckets()
   {
      const std::size_t cache_num = this->priv_get_cache_bucket_num();
      node_ops_t::template priv_clear_buckets<safemode_or_autounlink>
         (this->priv_get_cache(), this->priv_usable_bucket_count() - cache_num);
   }

   void priv_clear_buckets_and_cache()
   {
      this->priv_clear_buckets();
      this->priv_init_cache();
   }

   //Cached first non-empty bucket. If cache_begin<> is not active, the cache is
   //always the first bucket and update operations do nothing.
   BOOST_INTRUSIVE_FORCEINLINE bucket_ptr priv_get_cache() const
   {
      BOOST_IF_CONSTEXPR(cache_begin)
         return this->priv_state().get_cache(bucket_ptr());
      else
         return this->priv_bucket_pointer();
   }

   BOOST_INTRUSIVE_FORCEINLINE void priv_set_cache(bucket_ptr p)
   {  this->priv_state().set_cache(p);   }

   void priv_set_cache_bucket_num(std::size_t insertion_bucket)
   {
      (void)insertion_bucket;
      BOOST_IF_CONSTEXPR(cache_begin){
         BOOST_INTRUSIVE_INVARIANT_ASSERT(insertion_bucket <= this->priv_usable_bucket_count());
         this->priv_set_cache(this->priv_bucket_pointer() + std::ptrdiff_t(insertion_bucket));
      }
   }

   BOOST_INTRUSIVE_FORCEINLINE std::size_t priv_get_cache_bucket_num() const
   {
      BOOST_IF_CONSTEXPR(cache_begin)
         return std::size_t(this->priv_get_cache() - this->priv_bucket_pointer());
      else
         return 0u;
   }

   BOOST_INTRUSIVE_FORCEINLINE void priv_init_cache()
   {
      BOOST_IF_CONSTEXPR(cache_begin){
         this->priv_set_cache(this->priv_past_usable_bucket_ptr());
      }
   }

   void priv_swap_cache(hashdata_internal &other)
   {
      (void)other;
      BOOST_IF_CONSTEXPR(cache_begin){
         const bucket_ptr tmp(this->priv_get_cache());
         this->priv_set_cache(other.priv_get_cache());
         other.priv_set_cache(tmp);
      }
   }

   siterator priv_begin(bucket_ptr &pbucketptr) const
   {
      BOOST_IF_CONSTEXPR(cache_begin){
         pbucketptr = this->priv_get_cache();
         return pbucketptr == this->priv_past_usable_bucket_ptr()
            ? this->priv_end_sit() : siterator(pbucketptr->begin_ptr());
      }
      else{
         return node_ops_t::priv_begin_from(this->priv_bucket_pointer(), 0u, this->priv_usable_bucket_count(), pbucketptr);
      }
   }

   void priv_insertion_update_cache(std::size_t insertion_bucket)
   {
      (void)insertion_bucket;
      BOOST_IF_CONSTEXPR(cache_begin){
         BOOST_INTRUSIVE_INVARIANT_ASSERT(insertion_bucket < this->priv_usable_bucket_count());
         bucket_ptr p = this->priv_bucket_pointer() + std::ptrdiff_t(insertion_bucket);
         if(p < this->priv_get_cache()){
            this->priv_set_cache(p);
         }
      }
   }

   void priv_erasure_update_cache_range(std::size_t first_bucket_num, std::size_t last_bucket_num)
   {
      (void)first_bucket_num; (void)last_bucket_num;
      //If the last bucket is the end, the cache must be updated
      //to the last position if all
      BOOST_IF_CONSTEXPR(cache_begin){
         if(this->priv_get_cache_bucket_num() == first_bucket_num   &&
            this->priv_bucket_empty(first_bucket_num) ){
            this->priv_set_cache(this->priv_bucket_pointer() + std::ptrdiff_t(last_bucket_num));
            this->priv_erasure_update_cache();
         }
      }
   }

   void priv_erasure_update_cache(bucket_ptr first_bucket)
   {
      (void)first_bucket;
      //If the last bucket is the end, the cache must be updated
      //to the last position if all
      BOOST_IF_CONSTEXPR(cache_begin){
         if (this->priv_get_cache() == first_bucket &&
            this->priv_bucket_empty(first_bucket)) {
            this->priv_erasure_update_cache();
         }
      }
   }

   void priv_erasure_update_cache()
   {
      BOOST_IF_CONSTEXPR(cache_begin){
         bucket_ptr cache = this->priv_get_cache();
         node_ops_t::priv_skip_empty_buckets(cache, this->priv_past_usable_bucket_ptr());
         this->priv_set_cache(cache);
      }
   }
   
   typedef typename bucket_plus_vtraits_t::iterator         iterator;
   typedef typename bucket_plus_vtraits_t::const_iterator   const_iterator;

   //public functions
   BOOST_INTRUSIVE_FORCEINLINE SizeType split_count() const BOOST_NOEXCEPT
   {  return this->priv_split_traits().get_size();  }

   BOOST_INTRUSIVE_FORCEINLINE void split_count(SizeType s) BOOST_NOEXCEPT
   {  this->priv_split_traits().set_size(s);  }

   //public functions
   BOOST_INTRUSIVE_FORCEINLINE void inc_split_count() BOOST_NOEXCEPT
   {  this->priv_split_traits().increment();  }

   BOOST_INTRUSIVE_FORCEINLINE void dec_split_count() BOOST_NOEXCEPT
   {  this->priv_split_traits().decrement();  }

   BOOST_INTRUSIVE_FORCEINLINE static SizeType initial_split_from_bucket_count(SizeType bc) BOOST_NOEXCEPT
   {  return index_ops_t::initial_split_from_bucket_count(bc);  }

   BOOST_INTRUSIVE_FORCEINLINE static SizeType rehash_split_from_bucket_count(SizeType bc) BOOST_NOEXCEPT
   {  return index_ops_t::rehash_split_from_bucket_count(bc);  }

   BOOST_INTRUSIVE_FORCEINLINE iterator iterator_to(reference value) BOOST_NOEXCEPT_IF(!linear_buckets)
   {  return iterator_to(value, linear_buckets_t());  }

   BOOST_INTRUSIVE_FORCEINLINE const_iterator iterator_to(const_reference value) const BOOST_NOEXCEPT_IF(!linear_buckets)
   {  return iterator_to(value, linear_buckets_t());  }

   iterator iterator_to(reference value, detail::true_)  //linear_buckets
   {
      const std::size_t h = this->priv_stored_or_compute_hash(value, store_hash_t());
      siterator sit(this->priv_value_to_node_ptr(value));
      return this->build_iterator(sit, this->priv_hash_to_bucket_ptr(h));
   }

   const_iterator iterator_to(const_reference value, detail::true_) const //linear_buckets
   {
      const std::size_t h = this->priv_stored_or_compute_hash(value, store_hash_t());
      siterator const sit = siterator
         ( pointer_traits<node_ptr>::const_cast_from(this->priv_value_to_node_ptr(value))
         );
      return this->build_const_iterator(sit, this->priv_hash_to_bucket_ptr(h));
   }

   static const bool incremental = 0 != (BoolFlags & hash_bool_flags::incremental_pos);
   static const bool power_2_buckets = incremental || (0 != (BoolFlags & hash_bool_flags::power_2_buckets_pos));
   static const bool fastmod_buckets = 0 != (BoolFlags & hash_bool_flags::fastmod_buckets_pos);

   typedef detail::bool_<fastmod_buckets> fastmod_buckets_t;
   typedef hashtable_index_ops<power_2_buckets, incremental, fastmod_buckets> index_ops_t;

   BOOST_INTRUSIVE_FORCEINLINE bucket_type &priv_hash_to_bucket(std::size_t hash_value) const
   {  return this->priv_bucket(this->priv_hash_to_nbucket(hash_value));   }

   BOOST_INTRUSIVE_FORCEINLINE bucket_ptr priv_hash_to_bucket_ptr(std::size_t hash_value) const
   {  return this->priv_bucket_ptr(this->priv_hash_to_nbucket(hash_value));   }

   BOOST_INTRUSIVE_FORCEINLINE size_type priv_hash_to_nbucket(std::size_t hash_value) const
   {  return (priv_hash_to_nbucket)(hash_value, fastmod_buckets_t());  }

   BOOST_INTRUSIVE_FORCEINLINE size_type priv_hash_to_nbucket(std::size_t hash_value, detail::true_) const  //fastmod_buckets_t
   {  return static_cast<size_type>(prime_fmod_size::position(hash_value, this->split_count()));  }

   BOOST_INTRUSIVE_FORCEINLINE size_type priv_hash_to_nbucket(std::size_t hash_value, detail::false_) const //!fastmod_buckets_t
   {
      return static_cast<size_type>(index_ops_t::hash_to_bucket
         (hash_value, this->priv_usable_bucket_count(), this->split_count()));
   }

   BOOST_INTRUSIVE_FORCEINLINE iterator iterator_to(reference value, detail::false_) BOOST_NOEXCEPT
   {
      return iterator( siterator(this->priv_value_to_node_ptr(value))
                     , &this->get_bucket_value_traits());
   }

   const_iterator iterator_to(const_reference value, detail::false_) const BOOST_NOEXCEPT
   {
      siterator const sit = siterator
         ( pointer_traits<node_ptr>::const_cast_from(this->priv_value_to_node_ptr(value)) );
      return const_iterator(sit, &this->get_bucket_value_traits());
   }


   static local_iterator s_local_iterator_to(reference value) BOOST_NOEXCEPT
   {
      BOOST_INTRUSIVE_STATIC_ASSERT((!stateful_value_traits));
      siterator sit(value_traits::to_node_ptr(value));
      return local_iterator(sit, const_value_traits_ptr());
   }

   static const_local_iterator s_local_iterator_to(const_reference value) BOOST_NOEXCEPT
   {
      BOOST_INTRUSIVE_STATIC_ASSERT((!stateful_value_traits));
      siterator const sit = siterator
         ( pointer_traits<node_ptr>::const_cast_from
            (value_traits::to_node_ptr(value))
         );
      return const_local_iterator(sit, const_value_traits_ptr());
   }

   local_iterator local_iterator_to(reference value) BOOST_NOEXCEPT
   {
      siterator sit(this->priv_value_to_node_ptr(value));
      return local_iterator(sit, this->priv_value_traits_ptr());
   }

   const_local_iterator local_iterator_to(const_reference value) const BOOST_NOEXCEPT
   {
       siterator sit
         ( pointer_traits<node_ptr>::const_cast_from(this->priv_value_to_node_ptr(value)) );
      return const_local_iterator(sit, this->priv_value_traits_ptr());
   }

   BOOST_INTRUSIVE_FORCEINLINE size_type bucket_count() const BOOST_NOEXCEPT
   {  return size_type(this->priv_usable_bucket_count());   }

   BOOST_INTRUSIVE_FORCEINLINE size_type bucket_size(size_type n) const BOOST_NOEXCEPT
   {  return (size_type)this->priv_bucket_size(n);   }

   BOOST_INTRUSIVE_FORCEINLINE bucket_ptr bucket_pointer() const BOOST_NOEXCEPT
   {  return this->priv_bucket_pointer();   }

   BOOST_INTRUSIVE_FORCEINLINE local_iterator begin(size_type n) BOOST_NOEXCEPT
   {  return local_iterator(this->priv_bucket_lbegin(n), this->priv_value_traits_ptr());  }

   BOOST_INTRUSIVE_FORCEINLINE const_local_iterator begin(size_type n) const BOOST_NOEXCEPT
   {  return this->cbegin(n);  }

   BOOST_INTRUSIVE_FORCEINLINE static size_type suggested_upper_bucket_count(size_type n) BOOST_NOEXCEPT
   {  return index_ops_t::suggested_upper_bucket_count(n);  }

   BOOST_INTRUSIVE_FORCEINLINE static size_type suggested_lower_bucket_count(size_type n) BOOST_NOEXCEPT
   {  return index_ops_t::suggested_lower_bucket_count(n);  }

   BOOST_INTRUSIVE_FORCEINLINE const_local_iterator cbegin(size_type n) const BOOST_NOEXCEPT
   {
      return const_local_iterator
         (this->priv_bucket_lbegin(n)
         , this->priv_value_traits_ptr());
   }

   using split_bucket_hash_equal_t::end;
   using split_bucket_hash_equal_t::cend;

   BOOST_INTRUSIVE_FORCEINLINE local_iterator end(size_type n) BOOST_NOEXCEPT
   {  return local_iterator(this->priv_bucket_lend(n), this->priv_value_traits_ptr());  }

   BOOST_INTRUSIVE_FORCEINLINE const_local_iterator end(size_type n) const BOOST_NOEXCEPT
   {  return this->cend(n);  }

   BOOST_INTRUSIVE_FORCEINLINE const_local_iterator cend(size_type n) const BOOST_NOEXCEPT
   {
      return const_local_iterator
         ( this->priv_bucket_lend(n)
         , this->priv_value_traits_ptr());
   }

   //Public functions for hashtable_impl

   inline iterator begin() BOOST_NOEXCEPT
   {
      bucket_ptr p;
      siterator s = this->priv_begin(p);
      return this->build_iterator(s, p);
   }

   BOOST_INTRUSIVE_FORCEINLINE const_iterator begin() const BOOST_NOEXCEPT
   {  return this->cbegin();  }

   inline const_iterator cbegin() const BOOST_NOEXCEPT
   {
      bucket_ptr p;
      siterator s = this->priv_begin(p);
      return this->build_const_iterator(s, p);
   }

   BOOST_INTRUSIVE_FORCEINLINE hasher hash_function() const
   {  return this->priv_hasher();  }

   BOOST_INTRUSIVE_FORCEINLINE key_equal key_eq() const
   {  return this->priv_equal();   }
};

template< class ValueTraits,    class VoidOrKeyOfValue, class VoidOrKeyHash
        , class VoidOrKeyEqual, class BucketTraits,     class SizeType
        , std::size_t BoolFlags>
struct get_hashtable_size_wrapper_internal
{
   typedef hashtable_size_wrapper
      < hashdata_internal
         < ValueTraits
         , VoidOrKeyOfValue, VoidOrKeyHash, VoidOrKeyEqual
         , BucketTraits, SizeType
         , BoolFlags & ~(hash_bool_flags::constant_time_size_pos) //1
         >
      , SizeType
      , (BoolFlags& hash_bool_flags::constant_time_size_pos) != 0
      > type;
};

#endif   //#if !defined(BOOST_INTRUSIVE_DOXYGEN_INVOKED)

#ifdef BOOST_INTRUSIVE_DOXYGEN_INVOKED
//! This metafunction will obtain the type of a bucket
//! from the value_traits or hook option to be used with
//! a hash container.
template<class ValueTraitsOrHookOption>
struct unordered_bucket
{
   typedef implementation_defined type;
};

//! This metafunction will obtain the type of a bucket pointer
//! from the value_traits or hook option to be used with
//! a hash container.
template<class ValueTraitsOrHookOption>
struct unordered_bucket_ptr
{
   typedef implementation_defined type;
};

//! This metafunction will obtain the type of the default bucket traits
//! (when the user does not specify the bucket_traits<> option) from the
//! value_traits or hook option to be used with a hash container.
template<class ValueTraitsOrHookOption>
struct unordered_default_bucket_traits
{
   typedef implementation_defined type;
};
#endif

//! The class template hashtable is an intrusive hash table container, that
//! is used to construct intrusive unordered_set and unordered_multiset containers. The
//! no-throw guarantee holds only, if the VoidOrKeyEqual object and Hasher don't throw.
//!
//! hashtable is a semi-intrusive container: each object to be stored in the
//! container must contain a proper hook, but the container also needs
//! additional auxiliary memory to work: hashtable needs a pointer to an array
//! of type `bucket_type` to be passed in the constructor. This bucket array must
//! have at least the same lifetime as the container. This makes the use of
//! hashtable more complicated than purely intrusive containers.
//! `bucket_type` is default-constructible, copyable and assignable
//!
//! The template parameter \c T is the type to be managed by the container.
//! The user can specify additional options and if no options are provided
//! default options are used.
//!
//! The container supports the following options:
//! \c base_hook<>/member_hook<>/value_traits<>,
//! \c constant_time_size<>, \c size_type<>, \c hash<> and \c equal<>
//! \c bucket_traits<>, power_2_buckets<>, cache_begin<> and incremental<>.
//!
//! hashtable only provides forward iterators but it provides 4 iterator types:
//! iterator and const_iterator to navigate through the whole container and
//! local_iterator and const_local_iterator to navigate through the values
//! stored in a single bucket. Local iterators are faster and smaller.
//!
//! It's not recommended to use non constant-time size hashtables because several
//! key functions, like "empty()", become non-constant time functions. Non
//! constant_time size hashtables are mainly provided to support auto-unlink hooks.
//!
//! hashtables, does not make automatic rehashings nor
//! offers functions related to a load factor. Rehashing can be explicitly requested
//! and the user must provide a new bucket array that will be used from that moment.
//!
//! Since no automatic rehashing is done, iterators are never invalidated when
//! inserting or erasing elements. Iterators are only invalidated when rehashing.
#if defined(BOOST_INTRUSIVE_DOXYGEN_INVOKED)
template<class T, class ...Options>
#else
template<class ValueTraits, class VoidOrKeyOfValue, class VoidOrKeyHash, class VoidOrKeyEqual, class BucketTraits, class SizeType, std::size_t BoolFlags>
#endif
class hashtable_impl
   : private get_hashtable_size_wrapper_internal
         <ValueTraits, VoidOrKeyOfValue, VoidOrKeyHash, VoidOrKeyEqual, BucketTraits, SizeType, BoolFlags>::type
{
   static const bool linear_buckets_flag = (BoolFlags & hash_bool_flags::linear_buckets_pos) != 0;
   typedef typename get_hashtable_size_wrapper_internal
      <ValueTraits, VoidOrKeyOfValue, VoidOrKeyHash, VoidOrKeyEqual, BucketTraits, SizeType, BoolFlags>::type
      internal_type;
   typedef typename internal_type::size_traits                       size_traits;
   typedef hash_key_types_base
      < typename ValueTraits::value_type
      , VoidOrKeyOfValue
      >                                                              hash_types_base;
   public:
   typedef BOOST_INTRUSIVE_IMPDEF(ValueTraits)                       value_traits;
   typedef BOOST_INTRUSIVE_IMPDEF(BucketTraits)                      bucket_traits;

   /// @cond
   typedef bucket_plus_vtraits
         <ValueTraits, BucketTraits, linear_buckets_flag>            bucket_plus_vtraits_t;
   typedef typename bucket_plus_vtraits_t::node_ops_t       node_ops_t;
   typedef typename bucket_plus_vtraits_t::const_value_traits_ptr    const_value_traits_ptr;

   typedef detail::bool_<linear_buckets_flag>                        linear_buckets_t;

   typedef typename internal_type::siterator                         siterator;
   typedef typename internal_type::const_siterator                   const_siterator;
   using internal_type::begin;
   using internal_type::cbegin;
   using internal_type::end;
   using internal_type::cend;
   using internal_type::hash_function;
   using internal_type::key_eq;
   using internal_type::bucket_size;
   using internal_type::bucket_count;
   using internal_type::local_iterator_to;
   using internal_type::s_local_iterator_to;
   using internal_type::iterator_to;
   using internal_type::bucket_pointer;
   using internal_type::suggested_upper_bucket_count;
   using internal_type::suggested_lower_bucket_count;
   using internal_type::split_count;

   /// @endcond

   typedef BOOST_INTRUSIVE_IMPDEF(typename value_traits::pointer)                      pointer;
   typedef BOOST_INTRUSIVE_IMPDEF(typename value_traits::const_pointer)                const_pointer;
   typedef BOOST_INTRUSIVE_IMPDEF(typename value_traits::value_type)                   value_type;
   typedef BOOST_INTRUSIVE_IMPDEF(typename hash_types_base::key_type)                  key_type;
   typedef BOOST_INTRUSIVE_IMPDEF(typename hash_types_base::key_of_value)              key_of_value;
   typedef BOOST_INTRUSIVE_IMPDEF(typename pointer_traits<pointer>::reference)         reference;
   typedef BOOST_INTRUSIVE_IMPDEF(typename pointer_traits<const_pointer>::reference)   const_reference;
   typedef BOOST_INTRUSIVE_IMPDEF(typename pointer_traits<pointer>::difference_type)   difference_type;
   typedef BOOST_INTRUSIVE_IMPDEF(SizeType)                                            size_type;
   typedef BOOST_INTRUSIVE_IMPDEF(typename internal_type::key_equal)                   key_equal;
   typedef BOOST_INTRUSIVE_IMPDEF(typename internal_type::hasher)                      hasher;
   typedef BOOST_INTRUSIVE_IMPDEF(typename internal_type::bucket_type)                 bucket_type;
   typedef BOOST_INTRUSIVE_IMPDEF(typename internal_type::bucket_ptr)                  bucket_ptr;
   typedef BOOST_INTRUSIVE_IMPDEF(typename internal_type::iterator)                    iterator;
   typedef BOOST_INTRUSIVE_IMPDEF(typename internal_type::const_iterator)              const_iterator;
   typedef BOOST_INTRUSIVE_IMPDEF(typename internal_type::local_iterator)              local_iterator;
   typedef BOOST_INTRUSIVE_IMPDEF(typename internal_type::const_local_iterator)        const_local_iterator;
   typedef BOOST_INTRUSIVE_IMPDEF(typename value_traits::node_traits)                  node_traits;
   typedef BOOST_INTRUSIVE_IMPDEF(typename node_traits::node)                          node;
   typedef BOOST_INTRUSIVE_IMPDEF(typename pointer_traits
      <pointer>::template rebind_pointer
         < node >::type)                                                               node_ptr;
   typedef BOOST_INTRUSIVE_IMPDEF(typename pointer_traits
      <pointer>::template rebind_pointer
         < const node >::type)                                                         const_node_ptr;
   typedef typename pointer_traits
      <node_ptr>::reference                                          node_reference;
   typedef typename pointer_traits
      <const_node_ptr>::reference                                    const_node_reference;
   typedef typename internal_type::slist_node_algorithms             slist_node_algorithms;

   static const bool stateful_value_traits = internal_type::stateful_value_traits;
   static const bool store_hash = internal_type::store_hash;

   static const bool unique_keys          = 0 != (BoolFlags & hash_bool_flags::unique_keys_pos);
   static const bool constant_time_size   = 0 != (BoolFlags & hash_bool_flags::constant_time_size_pos);
   static const bool cache_begin          = 0 != (BoolFlags & hash_bool_flags::cache_begin_pos);
   static const bool compare_hash         = 0 != (BoolFlags & hash_bool_flags::compare_hash_pos);
   static const bool incremental          = 0 != (BoolFlags & hash_bool_flags::incremental_pos);
   static const bool power_2_buckets      = incremental || (0 != (BoolFlags & hash_bool_flags::power_2_buckets_pos));
   static const bool optimize_multikey    = optimize_multikey_is_true<node_traits>::value && !unique_keys;
   static const bool linear_buckets       = linear_buckets_flag;
   static const bool fastmod_buckets      = 0 != (BoolFlags & hash_bool_flags::fastmod_buckets_pos);
   static const std::size_t bucket_overhead = internal_type::bucket_overhead;

   /// @cond
   static const bool is_multikey = !unique_keys;
   private:

   //Configuration error: compare_hash<> can't be specified without store_hash<>
   //See documentation for more explanations
   BOOST_INTRUSIVE_STATIC_ASSERT((!compare_hash || store_hash));

   //Configuration error: fasmod_buckets<> can't be specified with incremental<> or power_2_buckets<>
   //See documentation for more explanations
   BOOST_INTRUSIVE_STATIC_ASSERT(!(fastmod_buckets && power_2_buckets));

   typedef typename internal_type::slist_node_ptr                    slist_node_ptr;
   typedef typename pointer_traits
      <slist_node_ptr>::template rebind_pointer
         < void >::type                                              void_pointer;
   //We'll define group traits, but these won't be instantiated if
   //optimize_multikey is not true
   typedef unordered_group_adapter<node_traits>                      group_traits;
   typedef circular_slist_algorithms<group_traits>                   group_algorithms;
   typedef typename internal_type::store_hash_t                      store_hash_t;
   typedef detail::bool_<optimize_multikey>                          optimize_multikey_t;
   //Only key_equal guarantees at most one equivalent element in a container with unique keys:
   //a KeyEqual can be coarser. key_type overloads use unique_keys_t, KeyType overloads false.
   typedef detail::bool_<unique_keys>                                unique_keys_t;
   typedef detail::bool_<cache_begin>                                cache_begin_t;
   typedef detail::bool_<power_2_buckets>                            power_2_buckets_t;
   typedef detail::bool_<compare_hash>                               compare_hash_t;
   typedef typename internal_type::split_traits                      split_traits;
   typedef typename internal_type::index_ops_t                       index_ops_t;
   typedef group_functions<node_traits>                              group_functions_t;
   typedef node_functions<node_traits>                               node_functions_t;

   private:
   //noncopyable, movable
   BOOST_MOVABLE_BUT_NOT_COPYABLE(hashtable_impl)

   static const bool safemode_or_autounlink = internal_type::safemode_or_autounlink;

   //Constant-time size is incompatible with auto-unlink hooks!
   BOOST_INTRUSIVE_STATIC_ASSERT(!(constant_time_size && ((int)value_traits::link_mode == (int)auto_unlink)));
   //Cache begin is incompatible with auto-unlink hooks!
   BOOST_INTRUSIVE_STATIC_ASSERT(!(cache_begin && ((int)value_traits::link_mode == (int)auto_unlink)));
   //Linear buckets are incompatible with auto-unlink hooks!
   BOOST_INTRUSIVE_STATIC_ASSERT(!(linear_buckets && ((int)value_traits::link_mode == (int)auto_unlink)));


   /// @endcond
   
   public:
   typedef BOOST_INTRUSIVE_IMPDEF(insert_commit_data_impl) insert_commit_data;

   private:
   void default_init_actions()
   {
      const size_type bucket_sz = this->bucket_count();
      node_ops_t::priv_init_bucket_array
         (this->priv_bucket_pointer(), this->priv_bucket_traits().bucket_count(), power_2_buckets);
      this->priv_init_cache();
      this->priv_size_traits().set_size(size_type(0));
      this->split_count(this->initial_split_from_bucket_count(bucket_sz));
   }

   public:

   //! <b>Requires</b>: buckets must not be being used by any other resource.
   //!
   //! <b>Effects</b>: Constructs an empty unordered_set, storing a reference
   //!   to the bucket array and copies of the key_hasher and equal_func functors.
   //!
   //! <b>Complexity</b>: Constant.
   //!
   //! <b>Throws</b>: If value_traits::node_traits::node
   //!   constructor throws (this does not happen with predefined Boost.Intrusive hooks)
   //!   or the copy constructor or invocation of hash_func or equal_func throws.
   //!
   //! <b>Notes</b>: buckets array must be disposed only after
   //!   *this is disposed.
   explicit hashtable_impl ( const bucket_traits &b_traits
                           , const hasher & hash_func = hasher()
                           , const key_equal &equal_func = key_equal()
                           , const value_traits &v_traits = value_traits())
      :  internal_type(v_traits, b_traits, hash_func, equal_func)
   {  this->default_init_actions(); }

   //! <b>Requires</b>: buckets must not be being used by any other resource
   //!   and dereferencing iterator must yield an lvalue of type value_type.
   //!
   //! <b>Effects</b>: Constructs an empty container and inserts elements from
   //!   [b, e).
   //!
   //! <b>Complexity</b>: If N is distance(b, e): Average case is O(N)
   //!   (with a good hash function and with buckets_len >= N),worst case O(N^2).
   //!
   //! <b>Throws</b>: If value_traits::node_traits::node
   //!   constructor throws (this does not happen with predefined Boost.Intrusive hooks)
   //!   or the copy constructor or invocation of hasher or key_equal throws.
   //!
   //! <b>Notes</b>: buckets array must be disposed only after
   //!   *this is disposed.
   template<class Iterator>
   hashtable_impl ( bool unique, Iterator b, Iterator e
                  , const bucket_traits &b_traits
                  , const hasher & hash_func = hasher()
                  , const key_equal &equal_func = key_equal()
                  , const value_traits &v_traits = value_traits())
      :  internal_type(v_traits, b_traits, hash_func, equal_func)
   {
      this->default_init_actions();

      //Now insert
      if(unique)
         this->insert_unique(b, e);
      else
         this->insert_equal(b, e);
   }

   //! <b>Effects</b>: Constructs a container moving resources from another container.
   //!   Internal value traits, bucket traits, hasher and comparison are move constructed and
   //!   nodes belonging to x are linked to *this.
   //!
   //! <b>Complexity</b>: Constant.
   //!
   //! <b>Throws</b>: If value_traits::node_traits::node's
   //!   move constructor throws (this does not happen with predefined Boost.Intrusive hooks)
   //!   or the move constructor of value traits, bucket traits, hasher or comparison throws.
   //!
   //! <b>Note</b>: The bucket array is not copied: after the move, *this uses the bucket array
   //!   of x, which holds the nodes. The move constructor of the bucket traits must therefore leave
   //!   the moved-from bucket traits without a bucket array.
   //!
   //!   x is left empty and without a bucket array (bucket_count() == 0). It can be destroyed,
   //!   cleared, iterated, swapped, move assigned, used in clone_from (as destination only if
   //!   the source is empty) and rehashed (full_rehash does nothing). Calling rehash with new
   //!   bucket traits gives it a new bucket array. Functions that need a bucket (insertions,
   //!   lookups and erasures by key, bucket functions, etc.) can't be used until then.
   hashtable_impl(BOOST_RV_REF(hashtable_impl) x)
      : internal_type(BOOST_MOVE_BASE(internal_type, x))
   {
      this->priv_swap_cache(x);
      x.priv_init_cache();
      this->priv_size_traits().set_size(x.priv_size_traits().get_size());
      x.priv_size_traits().set_size(size_type(0));
      this->split_count(x.split_count());
      x.split_count(size_type(0));
   }

   //! <b>Effects</b>: Equivalent to swap.
   //!
   hashtable_impl& operator=(BOOST_RV_REF(hashtable_impl) x)
   {  this->swap(x); return *this;  }

   #if defined(BOOST_INTRUSIVE_DOXYGEN_INVOKED)
   //! <b>Effects</b>: Detaches all elements from this. The objects in the unordered_set
   //!   are not deleted (i.e. no destructors are called).
   //!
   //! <b>Complexity</b>: Linear to the number of elements in the unordered_set, if
   //!   it's a safe-mode or auto-unlink value. Otherwise constant.
   //!
   //! <b>Throws</b>: Nothing.
   ~hashtable_impl();

   //! <b>Effects</b>: Returns an iterator pointing to the beginning of the unordered_set.
   //!
   //! <b>Complexity</b>: Amortized constant time.
   //!   Worst case (empty unordered_set): O(this->bucket_count())
   //!
   //! <b>Throws</b>: Nothing.
   iterator begin() BOOST_NOEXCEPT;

   //! <b>Effects</b>: Returns a const_iterator pointing to the beginning
   //!   of the unordered_set.
   //!
   //! <b>Complexity</b>: Amortized constant time.
   //!   Worst case (empty unordered_set): O(this->bucket_count())
   //!
   //! <b>Throws</b>: Nothing.
   const_iterator begin() const BOOST_NOEXCEPT;

   //! <b>Effects</b>: Returns a const_iterator pointing to the beginning
   //!   of the unordered_set.
   //!
   //! <b>Complexity</b>: Amortized constant time.
   //!   Worst case (empty unordered_set): O(this->bucket_count())
   //!
   //! <b>Throws</b>: Nothing.
   const_iterator cbegin() const BOOST_NOEXCEPT;

   //! <b>Effects</b>: Returns an iterator pointing to the end of the unordered_set.
   //!
   //! <b>Complexity</b>: Constant.
   //!
   //! <b>Throws</b>: Nothing.
   iterator end() BOOST_NOEXCEPT;

   //! <b>Effects</b>: Returns a const_iterator pointing to the end of the unordered_set.
   //!
   //! <b>Complexity</b>: Constant.
   //!
   //! <b>Throws</b>: Nothing.
   const_iterator end() const BOOST_NOEXCEPT;

   //! <b>Effects</b>: Returns a const_iterator pointing to the end of the unordered_set.
   //!
   //! <b>Complexity</b>: Constant.
   //!
   //! <b>Throws</b>: Nothing.
   const_iterator cend() const BOOST_NOEXCEPT;

   //! <b>Effects</b>: Returns the hasher object used by the unordered_set.
   //!
   //! <b>Complexity</b>: Constant.
   //!
   //! <b>Throws</b>: If hasher copy-constructor throws.
   hasher hash_function() const;

   //! <b>Effects</b>: Returns the key_equal object used by the unordered_set.
   //!
   //! <b>Complexity</b>: Constant.
   //!
   //! <b>Throws</b>: If key_equal copy-constructor throws.
   key_equal key_eq() const;

   #endif

   //! <b>Effects</b>: Returns true if the container is empty.
   //!
   //! <b>Complexity</b>: if constant-time size and cache_begin options are disabled,
   //!   average constant time (worst case, with empty() == true: O(this->bucket_count()).
   //!   Otherwise constant.
   //!
   //! <b>Throws</b>: Nothing.
   bool empty() const BOOST_NOEXCEPT
   {
      BOOST_IF_CONSTEXPR(constant_time_size){
         return !this->size();
      }
      else BOOST_IF_CONSTEXPR(cache_begin){
         return this->begin() == this->end();
      }
      else{
         return node_ops_t::priv_all_buckets_empty(this->priv_bucket_pointer(), this->priv_usable_bucket_count());
      }
   }

   //! <b>Effects</b>: Returns the number of elements stored in the unordered_set.
   //!
   //! <b>Complexity</b>: Linear to elements contained in *this if
   //!   constant_time_size is false. Constant-time otherwise.
   //!
   //! <b>Throws</b>: Nothing.
   size_type size() const BOOST_NOEXCEPT
   {
      BOOST_IF_CONSTEXPR(constant_time_size)
         return this->priv_size_traits().get_size();
      else{
         const std::size_t len = node_ops_t::priv_count_elements(this->priv_bucket_pointer(), this->priv_usable_bucket_count());
         BOOST_INTRUSIVE_INVARIANT_ASSERT((len <= SizeType(-1)));
         return size_type(len);
      }
   }

   //! <b>Requires</b>: the hasher and the equality function unqualified swap
   //!   call should not throw.
   //!
   //! <b>Effects</b>: Swaps the contents of two unordered_sets.
   //!   Swaps also the contained bucket array, the value traits and
   //!   the equality and hasher functors.
   //!
   //! <b>Complexity</b>: Constant.
   //!
   //! <b>Throws</b>: If the swap() call for the comparison or hash functors
   //!   found using ADL throw. Basic guarantee.
   //!
   //! <b>Note</b>: Does not affect the validity of iterators and references.
   //!   If value traits are stateful, iterators are invalidated.
   void swap(hashtable_impl& other)
   {
      //These can throw
      ::boost::adl_move_swap(this->priv_equal(),  other.priv_equal());
      ::boost::adl_move_swap(this->priv_hasher(), other.priv_hasher());
      //These can't throw
      ::boost::adl_move_swap(this->priv_bucket_traits(), other.priv_bucket_traits());
      ::boost::adl_move_swap(this->priv_value_traits(), other.priv_value_traits());
      this->priv_swap_cache(other);
      this->priv_size_traits().swap(other.priv_size_traits());
      this->priv_split_traits().swap(other.priv_split_traits());
   }

   //! <b>Requires</b>: Disposer::operator()(pointer) shouldn't throw
   //!   Cloner should yield to nodes that compare equal and produce the same
   //!   hash than the original node.
   //!
   //! <b>Effects</b>: Erases all the elements from *this
   //!   calling Disposer::operator()(pointer), clones all the
   //!   elements from src calling Cloner::operator()(const_reference )
   //!   and inserts them on *this. The hash function and the equality
   //!   predicate are copied from the source.
   //!
   //!   If store_hash option is true, this method does not use the hash function.
   //!
   //!   If any operation throws, all cloned elements are unlinked and disposed
   //!   calling Disposer::operator()(pointer).
   //!
   //! <b>Complexity</b>: Linear to erased plus inserted elements.
   //!
   //! <b>Throws</b>: If cloner or hasher throw or hash or equality predicate copying
   //!   throws. Basic guarantee.
   template <class Cloner, class Disposer>
   BOOST_INTRUSIVE_FORCEINLINE void clone_from(const hashtable_impl &src, Cloner cloner, Disposer disposer)
   {  this->priv_clone_from(src, cloner, disposer);   }

   //! <b>Requires</b>: Disposer::operator()(pointer) shouldn't throw
   //!   Cloner should yield to nodes that compare equal and produce the same
   //!   hash than the original node.
   //!
   //! <b>Effects</b>: Erases all the elements from *this
   //!   calling Disposer::operator()(pointer), clones all the
   //!   elements from src calling Cloner::operator()(reference)
   //!   and inserts them on *this. The hash function and the equality
   //!   predicate are copied from the source.
   //!
   //!   If store_hash option is true, this method does not use the hash function.
   //!
   //!   If any operation throws, all cloned elements are unlinked and disposed
   //!   calling Disposer::operator()(pointer).
   //!
   //! <b>Complexity</b>: Linear to erased plus inserted elements.
   //!
   //! <b>Throws</b>: If cloner or hasher throw or hash or equality predicate copying
   //!   throws. Basic guarantee.
   template <class Cloner, class Disposer>
   BOOST_INTRUSIVE_FORCEINLINE void clone_from(BOOST_RV_REF(hashtable_impl) src, Cloner cloner, Disposer disposer)
   {  this->priv_clone_from(static_cast<hashtable_impl&>(src), cloner, disposer);   }

   //! <b>Requires</b>: value must be an lvalue
   //!
   //! <b>Effects</b>: Inserts the value into the unordered_set.
   //!
   //! <b>Returns</b>: An iterator to the inserted value.
   //!
   //! <b>Complexity</b>: Average case O(1), worst case O(this->size()).
   //!
   //! <b>Throws</b>: If the internal hasher or the equality functor throws. Strong guarantee.
   //!
   //! <b>Note</b>: Does not affect the validity of iterators and references.
   //!   No copy-constructors are called.
   iterator insert_equal(reference value)
   {
      size_type bucket_num;
      std::size_t hash_value;
      siterator prev;
      siterator const it = this->priv_find
         (key_of_value()(value), this->priv_hasher(), this->priv_equal(), bucket_num, hash_value, prev);
      bool const next_is_in_group = optimize_multikey && it != this->priv_end_sit();
      return this->priv_insert_equal_after_find(value, bucket_num, hash_value, prev, next_is_in_group);
   }

   //! <b>Requires</b>: Dereferencing iterator must yield an lvalue
   //!   of type value_type.
   //!
   //! <b>Effects</b>: Equivalent to this->insert_equal(t) for each element in [b, e).
   //!
   //! <b>Complexity</b>: Average case O(N), where N is distance(b, e).
   //!   Worst case O(N*this->size()).
   //!
   //! <b>Throws</b>: If the internal hasher or the equality functor throws. Basic guarantee.
   //!
   //! <b>Note</b>: Does not affect the validity of iterators and references.
   //!   No copy-constructors are called.
   template<class Iterator>
   void insert_equal(Iterator b, Iterator e)
   {
      for (; b != e; ++b)
         this->insert_equal(*b);
   }

   //! <b>Requires</b>: value must be an lvalue
   //!
   //! <b>Effects</b>: Tries to inserts value into the unordered_set.
   //!
   //! <b>Returns</b>: If the value
   //!   is not already present inserts it and returns a pair containing the
   //!   iterator to the new value and true. If there is an equivalent value
   //!   returns a pair containing an iterator to the already present value
   //!   and false.
   //!
   //! <b>Complexity</b>: Average case O(1), worst case O(this->size()).
   //!
   //! <b>Throws</b>: If the internal hasher or the equality functor throws. Strong guarantee.
   //!
   //! <b>Note</b>: Does not affect the validity of iterators and references.
   //!   No copy-constructors are called.
   std::pair<iterator, bool> insert_unique(reference value)
   {
      insert_commit_data commit_data;
      std::pair<iterator, bool> ret = this->insert_unique_check(key_of_value()(value), commit_data);
      if(ret.second){
         ret.first = this->insert_unique_fast_commit(value, commit_data);
      }
      return ret;
   }

   //! <b>Requires</b>: Dereferencing iterator must yield an lvalue
   //!   of type value_type.
   //!
   //! <b>Effects</b>: Equivalent to this->insert_unique(t) for each element in [b, e).
   //!
   //! <b>Complexity</b>: Average case O(N), where N is distance(b, e).
   //!   Worst case O(N*this->size()).
   //!
   //! <b>Throws</b>: If the internal hasher or the equality functor throws. Basic guarantee.
   //!
   //! <b>Note</b>: Does not affect the validity of iterators and references.
   //!   No copy-constructors are called.
   template<class Iterator>
   void insert_unique(Iterator b, Iterator e)
   {
      for (; b != e; ++b)
         this->insert_unique(*b);
   }

   //! <b>Requires</b>: "hash_func" must be a hash function that induces
   //!   the same hash values as the stored hasher. The difference is that
   //!   "hash_func" hashes the given key instead of the value_type.
   //!
   //!   "equal_func" must be a equality function that induces
   //!   the same equality as key_equal. The difference is that
   //!   "equal_func" compares an arbitrary key with the contained values.
   //!
   //! <b>Effects</b>: Checks if a value can be inserted in the unordered_set, using
   //!   a user provided key instead of the value itself.
   //!
   //! <b>Returns</b>: If there is an equivalent value
   //!   returns a pair containing an iterator to the already present value
   //!   and false. If the value can be inserted returns true in the returned
   //!   pair boolean and fills "commit_data" that is meant to be used with
   //!   the "insert_commit" function.
   //!
   //! <b>Complexity</b>: Average case O(1), worst case O(this->size()).
   //!
   //! <b>Throws</b>: If hash_func or equal_func throw. Strong guarantee.
   //!
   //! <b>Notes</b>: This function is used to improve performance when constructing
   //!   a value_type is expensive: if there is an equivalent value
   //!   the constructed object must be discarded. Many times, the part of the
   //!   node that is used to impose the hash or the equality is much cheaper to
   //!   construct than the value_type and this function offers the possibility to
   //!   use that the part to check if the insertion will be successful.
   //!
   //!   If the check is successful, the user can construct the value_type and use
   //!   "insert_commit" to insert the object in constant-time.
   //!
   //!   "commit_data" remains valid for a subsequent "insert_commit" only if no more
   //!   objects are inserted or erased from the unordered_set.
   //!
   //!   After a successful rehashing insert_commit_data remains valid.
   template<class KeyType, class KeyHasher, class KeyEqual>
   std::pair<iterator, bool> insert_unique_check
      ( const KeyType &key
      , KeyHasher hash_func
      , KeyEqual equal_func
      , insert_commit_data &commit_data)
   {
      const std::size_t h = hash_func(key);
      const std::size_t bn = this->priv_hash_to_nbucket(h);

      commit_data.bucket_idx = bn;
      commit_data.set_hash(h);

      bucket_ptr bp = this->priv_bucket_ptr(bn);
      siterator const s = this->priv_find_in_bucket(*bp, key, equal_func, h);
      return std::pair<iterator, bool>(this->build_iterator(s, bp), s == this->priv_end_sit());
   }

   //! <b>Effects</b>: Checks if a value can be inserted in the unordered_set, using
   //!   a user provided key instead of the value itself.
   //!
   //! <b>Returns</b>: If there is an equivalent value
   //!   returns a pair containing an iterator to the already present value
   //!   and false. If the value can be inserted returns true in the returned
   //!   pair boolean and fills "commit_data" that is meant to be used with
   //!   the "insert_commit" function.
   //!
   //! <b>Complexity</b>: Average case O(1), worst case O(this->size()).
   //!
   //! <b>Throws</b>: If hasher or key_compare throw. Strong guarantee.
   //!
   //! <b>Notes</b>: This function is used to improve performance when constructing
   //!   a value_type is expensive: if there is an equivalent value
   //!   the constructed object must be discarded. Many times, the part of the
   //!   node that is used to impose the hash or the equality is much cheaper to
   //!   construct than the value_type and this function offers the possibility to
   //!   use that the part to check if the insertion will be successful.
   //!
   //!   If the check is successful, the user can construct the value_type and use
   //!   "insert_commit" to insert the object in constant-time.
   //!
   //!   "commit_data" remains valid for a subsequent "insert_commit" only if no more
   //!   objects are inserted or erased from the unordered_set.
   //!
   //!   After a successful rehashing insert_commit_data remains valid.
   inline std::pair<iterator, bool> insert_unique_check
      ( const key_type &key, insert_commit_data &commit_data)
   {  return this->insert_unique_check(key, this->priv_hasher(), this->priv_equal(), commit_data);  }

   //! <b>Requires</b>: value must be an lvalue of type value_type. commit_data
   //!   must have been obtained from a previous call to "insert_check".
   //!   No objects should have been inserted or erased from the unordered_set between
   //!   the "insert_check" that filled "commit_data" and the call to "insert_commit".
   //!
   //! <b>Effects</b>: Inserts the value in the unordered_set using the information obtained
   //!   from the "commit_data" that a previous "insert_check" filled.
   //!
   //! <b>Returns</b>: An iterator to the newly inserted object.
   //!
   //! <b>Complexity</b>: Constant time.
   //!
   //! <b>Throws</b>: Nothing.
   //!
   //! <b>Notes</b>: This function has only sense if a "insert_check" has been
   //!   previously executed to fill "commit_data". No value should be inserted or
   //!   erased between the "insert_check" and "insert_commit" calls.
   //!
   //!   After a successful rehashing insert_commit_data remains valid.
   iterator insert_unique_commit(reference value, const insert_commit_data& commit_data) BOOST_NOEXCEPT
   {
      size_type bucket_num = this->priv_hash_to_nbucket(commit_data.get_hash());
      bucket_type& b = this->priv_bucket(bucket_num);
      this->priv_size_traits().increment();
      node_ptr const n = pointer_traits<node_ptr>::pointer_to(this->priv_value_to_node(value));
      node_ops_t::template priv_link_after<optimize_multikey, safemode_or_autounlink>
         (b.get_node_ptr(), n, commit_data.get_hash(), false);
      this->priv_insertion_update_cache(bucket_num);
      return this->build_iterator(siterator(n), node_ops_t::to_ptr(b));
   }

   //! <b>Requires</b>: value must be an lvalue of type value_type. commit_data
   //!   must have been obtained from a previous call to "insert_check".
   //!   No objects should have been inserted or erased from the unordered_set between
   //!   the "insert_check" that filled "commit_data" and the call to "insert_commit".
   //!
   //!   No rehashing shall be performed between `insert_check` and `insert_fast_commit`.
   //! 
   //! <b>Effects</b>: Inserts the value in the unordered_set using the information obtained
   //!   from the "commit_data" that a previous "insert_check" filled.
   //!
   //! <b>Returns</b>: An iterator to the newly inserted object.
   //!
   //! <b>Complexity</b>: Constant time.
   //!
   //! <b>Throws</b>: Nothing.
   //!
   //! <b>Notes</b>: This function has only sense if a "insert_check" has been
   //!   previously executed to fill "commit_data". No value should be inserted or
   //!   erased between the "insert_check" and "insert_commit" calls.
   //!
   //!   Since this commit operation does not support rehashing between the check
   //!   and the commit, it's faster than `insert_commit`.
   iterator insert_unique_fast_commit(reference value, const insert_commit_data &commit_data) BOOST_NOEXCEPT
   {
      this->priv_size_traits().increment();
      node_ptr const n = this->priv_value_to_node_ptr(value);
      bucket_type& b = this->priv_bucket(commit_data.bucket_idx);
      node_ops_t::template priv_link_after<optimize_multikey, safemode_or_autounlink>
         (b.get_node_ptr(), n, commit_data.get_hash(), false);
      this->priv_insertion_update_cache(static_cast<size_type>(commit_data.bucket_idx));
      return this->build_iterator(siterator(n), node_ops_t::to_ptr(b));
   }

   //! <b>Effects</b>: Erases the element pointed to by i.
   //!
   //! <b>Complexity</b>: Average case O(1), worst case O(this->size()).
   //!
   //! <b>Throws</b>: Nothing.
   //!
   //! <b>Note</b>: Invalidates the iterators (but not the references)
   //!    to the erased element. No destructors are called.
   BOOST_INTRUSIVE_FORCEINLINE void erase(const_iterator i) BOOST_NOEXCEPT
   {  this->erase_and_dispose(i, detail::null_disposer());  }

   //! <b>Effects</b>: Erases the range pointed to by b end e.
   //!
   //! <b>Complexity</b>: Average case O(distance(b, e)),
   //!   worst case O(this->size()).
   //!
   //! <b>Throws</b>: Nothing.
   //!
   //! <b>Note</b>: Invalidates the iterators (but not the references)
   //!    to the erased elements. No destructors are called.
   BOOST_INTRUSIVE_FORCEINLINE void erase(const_iterator b, const_iterator e) BOOST_NOEXCEPT
   {  this->erase_and_dispose(b, e, detail::null_disposer());  }

   //! <b>Effects</b>: Erases all the elements with the given value.
   //!
   //! <b>Returns</b>: The number of erased elements.
   //!
   //! <b>Complexity</b>: Average case O(this->count(value)).
   //!   Worst case O(this->size()).
   //!
   //! <b>Throws</b>: If the internal hasher or the equality functor throws.
   //!   Basic guarantee.
   //!
   //! <b>Note</b>: Invalidates the iterators (but not the references)
   //!    to the erased elements. No destructors are called.
   BOOST_INTRUSIVE_FORCEINLINE size_type erase(const key_type &key)
   {
      return this->priv_erase_and_dispose
         (key, this->priv_hasher(), this->priv_equal(), detail::null_disposer(), unique_keys_t());
   }

   //! <b>Requires</b>: "hash_func" must be a hash function that induces
   //!   the same hash values as the stored hasher. The difference is that
   //!   "hash_func" hashes the given key instead of the value_type.
   //!
   //!   "equal_func" must be a equality function that induces
   //!   the same equality as key_equal. The difference is that
   //!   "equal_func" compares an arbitrary key with the contained values.
   //!
   //! <b>Effects</b>: Erases all the elements that have the same hash and
   //!   compare equal with the given key.
   //!
   //! <b>Returns</b>: The number of erased elements.
   //!
   //! <b>Complexity</b>: Average case O(this->count(value)).
   //!   Worst case O(this->size()).
   //!
   //! <b>Throws</b>: If hash_func or equal_func throw. Basic guarantee.
   //!
   //! <b>Note</b>: Invalidates the iterators (but not the references)
   //!    to the erased elements. No destructors are called.
   template<class KeyType, class KeyHasher, class KeyEqual>
   BOOST_INTRUSIVE_FORCEINLINE size_type erase(const KeyType& key, KeyHasher hash_func, KeyEqual equal_func)
   {  return this->erase_and_dispose(key, hash_func, equal_func, detail::null_disposer()); }

   //! <b>Requires</b>: Disposer::operator()(pointer) shouldn't throw.
   //!
   //! <b>Effects</b>: Erases the element pointed to by i.
   //!   Disposer::operator()(pointer) is called for the removed element.
   //!
   //! <b>Complexity</b>: Average case O(1), worst case O(this->size()).
   //!
   //! <b>Throws</b>: Nothing.
   //!
   //! <b>Note</b>: Invalidates the iterators
   //!    to the erased elements.
   template<class Disposer>
   BOOST_INTRUSIVE_DOC1ST(void
      , typename detail::disable_if_convertible<Disposer BOOST_INTRUSIVE_I const_iterator>::type)
    erase_and_dispose(const_iterator i, Disposer disposer) BOOST_NOEXCEPT
   {
      //Get the bucket number and local iterator for both iterators
      const bucket_ptr bp = this->priv_get_bucket_ptr(i);
      node_ops_t::priv_erase_node(*bp, i.slist_it(), this->make_node_disposer(disposer), optimize_multikey_t());
      this->priv_size_traits().decrement();
      this->priv_erasure_update_cache(bp);
   }

   //! <b>Requires</b>: Disposer::operator()(pointer) shouldn't throw.
   //!
   //! <b>Effects</b>: Erases the range pointed to by b end e.
   //!   Disposer::operator()(pointer) is called for the removed elements.
   //!
   //! <b>Complexity</b>: Average case O(distance(b, e)),
   //!   worst case O(this->size()).
   //!
   //! <b>Throws</b>: Nothing.
   //!
   //! <b>Note</b>: Invalidates the iterators
   //!    to the erased elements.
   template<class Disposer>
   void erase_and_dispose(const_iterator b, const_iterator e, Disposer disposer) BOOST_NOEXCEPT
   {
      if(b != e){
         //Get the bucket number of both iterators. For the end iterator,
         //the range ends at the end of the last bucket
         size_type const first_bucket_num = this->priv_get_bucket_num(b);
         bool const last_is_end = e == this->end();
         size_type const last_bucket_num = last_is_end
            ? size_type(this->bucket_count() - 1u) : this->priv_get_bucket_num(e);
         size_type const num_erased = (size_type)node_ops_t::priv_erase_range
            ( this->priv_bucket_pointer(), b.slist_it(), first_bucket_num, e.slist_it(), last_bucket_num, last_is_end
            , this->make_node_disposer(disposer), optimize_multikey_t());
         this->priv_size_traits().set_size(size_type(this->priv_size_traits().get_size()-num_erased));
         this->priv_erasure_update_cache_range(first_bucket_num, last_bucket_num);
      }
   }

   //! <b>Requires</b>: Disposer::operator()(pointer) shouldn't throw.
   //!
   //! <b>Effects</b>: Erases all the elements with the given value.
   //!   Disposer::operator()(pointer) is called for the removed elements.
   //!
   //! <b>Returns</b>: The number of erased elements.
   //!
   //! <b>Complexity</b>: Average case O(this->count(value)).
   //!   Worst case O(this->size()).
   //!
   //! <b>Throws</b>: If the internal hasher or the equality functor throws.
   //!   Basic guarantee.
   //!
   //! <b>Note</b>: Invalidates the iterators (but not the references)
   //!    to the erased elements. No destructors are called.
   template<class Disposer>
   BOOST_INTRUSIVE_FORCEINLINE size_type erase_and_dispose(const key_type &key, Disposer disposer)
   {
      return this->priv_erase_and_dispose
         (key, this->priv_hasher(), this->priv_equal(), disposer, unique_keys_t());
   }

   //! <b>Requires</b>: Disposer::operator()(pointer) shouldn't throw.
   //!
   //! <b>Effects</b>: Erases all the elements with the given key.
   //!   according to the comparison functor "equal_func".
   //!   Disposer::operator()(pointer) is called for the removed elements.
   //!
   //! <b>Returns</b>: The number of erased elements.
   //!
   //! <b>Complexity</b>: Average case O(this->count(value)).
   //!   Worst case O(this->size()).
   //!
   //! <b>Throws</b>: If hash_func or equal_func throw. Basic guarantee.
   //!
   //! <b>Note</b>: Invalidates the iterators
   //!    to the erased elements.
   template<class KeyType, class KeyHasher, class KeyEqual, class Disposer>
   inline size_type erase_and_dispose(const KeyType& key, KeyHasher hash_func
                              ,KeyEqual equal_func, Disposer disposer)
   {  return this->priv_erase_and_dispose(key, hash_func, equal_func, disposer, detail::bool_<false>());  }

   //! <b>Effects</b>: Erases all of the elements.
   //!
   //! <b>Complexity</b>: Linear to the number of elements on the container.
   //!   if it's a safe-mode or auto-unlink value_type. Constant time otherwise.
   //!
   //! <b>Throws</b>: Nothing.
   //!
   //! <b>Note</b>: Invalidates the iterators (but not the references)
   //!    to the erased elements. No destructors are called.
   void clear() BOOST_NOEXCEPT
   {
      this->priv_clear_buckets_and_cache();
      this->priv_size_traits().set_size(size_type(0));
   }

   //! <b>Requires</b>: Disposer::operator()(pointer) shouldn't throw.
   //!
   //! <b>Effects</b>: Erases all of the elements.
   //!
   //! <b>Complexity</b>: Linear to the number of elements on the container.
   //!   Disposer::operator()(pointer) is called for the removed elements.
   //!
   //! <b>Throws</b>: Nothing.
   //!
   //! <b>Note</b>: Invalidates the iterators (but not the references)
   //!    to the erased elements. No destructors are called.
   template<class Disposer>
   void clear_and_dispose(Disposer disposer) BOOST_NOEXCEPT
   {
      if(!constant_time_size || !this->empty()){
         size_type num_buckets = this->bucket_count();
         bucket_ptr b = this->priv_bucket_pointer();
         typename internal_type::template typeof_node_disposer<Disposer>::type d(disposer, &this->priv_value_traits());
         for(; num_buckets; ++b){
            --num_buckets;
            slist_node_algorithms::detach_and_dispose(b->get_node_ptr(), d);
         }
         this->priv_size_traits().set_size(size_type(0));
      }
      this->priv_init_cache();
   }

   //! <b>Effects</b>: Returns the number of contained elements with the given value
   //!
   //! <b>Complexity</b>: Average case O(1), worst case O(this->size()).
   //!
   //! <b>Throws</b>: If the internal hasher or the equality functor throws.
   inline size_type count(const key_type &key) const
   {
      size_type cnt;
      size_type n_bucket;
      this->priv_local_equal_range(key, this->priv_hasher(), this->priv_equal(), n_bucket, cnt, unique_keys_t());
      return cnt;
   }

   //! <b>Requires</b>: "hash_func" must be a hash function that induces
   //!   the same hash values as the stored hasher. The difference is that
   //!   "hash_func" hashes the given key instead of the value_type.
   //!
   //!   "equal_func" must be a equality function that induces
   //!   the same equality as key_equal. The difference is that
   //!   "equal_func" compares an arbitrary key with the contained values.
   //!
   //! <b>Effects</b>: Returns the number of contained elements with the given key
   //!
   //! <b>Complexity</b>: Average case O(1), worst case O(this->size()).
   //!
   //! <b>Throws</b>: If hash_func or equal throw.
   template<class KeyType, class KeyHasher, class KeyEqual>
   size_type count(const KeyType &key, KeyHasher hash_func, KeyEqual equal_func) const
   {
      size_type cnt;
      size_type n_bucket;
      this->priv_local_equal_range(key, hash_func, equal_func, n_bucket, cnt, detail::bool_<false>());
      return cnt;
   }

   //! <b>Effects</b>: Finds an iterator to the first element is equal to
   //!   "value" or end() if that element does not exist.
   //!
   //! <b>Complexity</b>: Average case O(1), worst case O(this->size()).
   //!
   //! <b>Throws</b>: If the internal hasher or the equality functor throws.
   BOOST_INTRUSIVE_FORCEINLINE iterator find(const key_type &key)
   {  return this->find(key, this->priv_hasher(), this->priv_equal());   }

   //! <b>Requires</b>: "hash_func" must be a hash function that induces
   //!   the same hash values as the stored hasher. The difference is that
   //!   "hash_func" hashes the given key instead of the value_type.
   //!
   //!   "equal_func" must be a equality function that induces
   //!   the same equality as key_equal. The difference is that
   //!   "equal_func" compares an arbitrary key with the contained values.
   //!
   //! <b>Effects</b>: Finds an iterator to the first element whose key is
   //!   "key" according to the given hash and equality functor or end() if
   //!   that element does not exist.
   //!
   //! <b>Complexity</b>: Average case O(1), worst case O(this->size()).
   //!
   //! <b>Throws</b>: If hash_func or equal_func throw.
   //!
   //! <b>Note</b>: This function is used when constructing a value_type
   //!   is expensive and the value_type can be compared with a cheaper
   //!   key type. Usually this key is part of the value_type.
   template<class KeyType, class KeyHasher, class KeyEqual>
   iterator find(const KeyType &key, KeyHasher hash_func, KeyEqual equal_func)
   {
      std::size_t h = hash_func(key);
      bucket_ptr bp = this->priv_hash_to_bucket_ptr(h);
      siterator s = this->priv_find_in_bucket(*bp, key, equal_func, h);
      return this->build_iterator(s, bp);
   }

   //! <b>Effects</b>: Finds a const_iterator to the first element whose key is
   //!   "key" or end() if that element does not exist.
   //!
   //! <b>Complexity</b>: Average case O(1), worst case O(this->size()).
   //!
   //! <b>Throws</b>: If the internal hasher or the equality functor throws.
   BOOST_INTRUSIVE_FORCEINLINE const_iterator find(const key_type &key) const
   {  return this->find(key, this->priv_hasher(), this->priv_equal());   }

   //! <b>Requires</b>: "hash_func" must be a hash function that induces
   //!   the same hash values as the stored hasher. The difference is that
   //!   "hash_func" hashes the given key instead of the value_type.
   //!
   //!   "equal_func" must be a equality function that induces
   //!   the same equality as key_equal. The difference is that
   //!   "equal_func" compares an arbitrary key with the contained values.
   //!
   //! <b>Effects</b>: Finds an iterator to the first element whose key is
   //!   "key" according to the given hasher and equality functor or end() if
   //!   that element does not exist.
   //!
   //! <b>Complexity</b>: Average case O(1), worst case O(this->size()).
   //!
   //! <b>Throws</b>: If hash_func or equal_func throw.
   //!
   //! <b>Note</b>: This function is used when constructing a value_type
   //!   is expensive and the value_type can be compared with a cheaper
   //!   key type. Usually this key is part of the value_type.
   template<class KeyType, class KeyHasher, class KeyEqual>
   const_iterator find
      (const KeyType &key, KeyHasher hash_func, KeyEqual equal_func) const
   {
      std::size_t h = hash_func(key);
      bucket_ptr bp = this->priv_hash_to_bucket_ptr(h);
      siterator s = this->priv_find_in_bucket(*bp, key, equal_func, h);
      return this->build_const_iterator(s, bp);
   }

   //! <b>Effects</b>: Returns a range containing all elements with values equivalent
   //!   to value. Returns std::make_pair(this->end(), this->end()) if no such
   //!   elements exist.
   //!
   //! <b>Complexity</b>: Average case O(this->count(value)). Worst case O(this->size()).
   //!
   //! <b>Throws</b>: If the internal hasher or the equality functor throws.
   inline std::pair<iterator,iterator> equal_range(const key_type &key)
   {
      priv_equal_range_result ret =
         this->priv_equal_range(key, this->priv_hasher(), this->priv_equal(), unique_keys_t());
      return std::pair<iterator, iterator>
         ( this->build_iterator(ret.first, ret.bucket_first)
         , this->build_iterator(ret.second, ret.bucket_second));
   }

   //! <b>Requires</b>: "hash_func" must be a hash function that induces
   //!   the same hash values as the stored hasher. The difference is that
   //!   "hash_func" hashes the given key instead of the value_type.
   //!
   //!   "equal_func" must be a equality function that induces
   //!   the same equality as key_equal. The difference is that
   //!   "equal_func" compares an arbitrary key with the contained values.
   //!
   //! <b>Effects</b>: Returns a range containing all elements with equivalent
   //!   keys. Returns std::make_pair(this->end(), this->end()) if no such
   //!   elements exist.
   //!
   //! <b>Complexity</b>: Average case O(this->count(key, hash_func, equal_func)).
   //!   Worst case O(this->size()).
   //!
   //! <b>Throws</b>: If hash_func or the equal_func throw.
   //!
   //! <b>Note</b>: This function is used when constructing a value_type
   //!   is expensive and the value_type can be compared with a cheaper
   //!   key type. Usually this key is part of the value_type.
   template<class KeyType, class KeyHasher, class KeyEqual>
   std::pair<iterator,iterator> equal_range
      (const KeyType &key, KeyHasher hash_func, KeyEqual equal_func)
   {
      priv_equal_range_result ret =
         this->priv_equal_range(key, hash_func, equal_func, detail::bool_<false>());
      return std::pair<iterator, iterator>
         ( this->build_iterator(ret.first, ret.bucket_first)
         , this->build_iterator(ret.second, ret.bucket_second));
   }

   //! <b>Effects</b>: Returns a range containing all elements with values equivalent
   //!   to value. Returns std::make_pair(this->end(), this->end()) if no such
   //!   elements exist.
   //!
   //! <b>Complexity</b>: Average case O(this->count(value)). Worst case O(this->size()).
   //!
   //! <b>Throws</b>: If the internal hasher or the equality functor throws.
   inline std::pair<const_iterator, const_iterator>
      equal_range(const key_type &key) const
   {
      priv_equal_range_result ret =
         this->priv_equal_range(key, this->priv_hasher(), this->priv_equal(), unique_keys_t());
      return std::pair<const_iterator, const_iterator>
         ( this->build_const_iterator(ret.first,  ret.bucket_first)
         , this->build_const_iterator(ret.second, ret.bucket_second));
   }

   //! <b>Requires</b>: "hash_func" must be a hash function that induces
   //!   the same hash values as the stored hasher. The difference is that
   //!   "hash_func" hashes the given key instead of the value_type.
   //!
   //!   "equal_func" must be a equality function that induces
   //!   the same equality as key_equal. The difference is that
   //!   "equal_func" compares an arbitrary key with the contained values.
   //!
   //! <b>Effects</b>: Returns a range containing all elements with equivalent
   //!   keys. Returns std::make_pair(this->end(), this->end()) if no such
   //!   elements exist.
   //!
   //! <b>Complexity</b>: Average case O(this->count(key, hash_func, equal_func)).
   //!   Worst case O(this->size()).
   //!
   //! <b>Throws</b>: If the hasher or equal_func throw.
   //!
   //! <b>Note</b>: This function is used when constructing a value_type
   //!   is expensive and the value_type can be compared with a cheaper
   //!   key type. Usually this key is part of the value_type.
   template<class KeyType, class KeyHasher, class KeyEqual>
   std::pair<const_iterator,const_iterator> equal_range
      (const KeyType &key, KeyHasher hash_func, KeyEqual equal_func) const
   {
      priv_equal_range_result ret =
         this->priv_equal_range(key, hash_func, equal_func, detail::bool_<false>());
      return std::pair<const_iterator, const_iterator>
         ( this->build_const_iterator(ret.first,  ret.bucket_first)
         , this->build_const_iterator(ret.second, ret.bucket_second));
   }

   #if defined(BOOST_INTRUSIVE_DOXYGEN_INVOKED)

   //! <b>Requires</b>: value must be an lvalue and shall be in a unordered_set of
   //!   appropriate type. Otherwise the behavior is undefined.
   //!
   //! <b>Effects</b>: Returns: a valid iterator belonging to the unordered_set
   //!   that points to the value
   //!
   //! <b>Complexity</b>: Constant.
   //!
   //! <b>Throws</b>: If linear_buckets<true> is set and store_hash<true> is not set,
   //!   if the internal hasher throws. Otherwise, nothing.
   iterator iterator_to(reference value);

   //! <b>Requires</b>: value must be an lvalue and shall be in a unordered_set of
   //!   appropriate type. Otherwise the behavior is undefined.
   //!
   //! <b>Effects</b>: Returns: a valid const_iterator belonging to the
   //!   unordered_set that points to the value
   //!
   //! <b>Complexity</b>: Constant.
   //!
   //! <b>Throws</b>: If linear_buckets<true> is set and store_hash<true> is not set,
   //!   if the internal hasher throws. Otherwise, nothing.
   const_iterator iterator_to(const_reference value) const;

   //! <b>Requires</b>: value must be an lvalue and shall be in a unordered_set of
   //!   appropriate type. Otherwise the behavior is undefined.
   //!
   //! <b>Effects</b>: Returns: a valid local_iterator belonging to the unordered_set
   //!   that points to the value
   //!
   //! <b>Complexity</b>: Constant.
   //!
   //! <b>Throws</b>: Nothing.
   //!
   //! <b>Note</b>: This static function is available only if the <i>value traits</i>
   //!   is stateless.
   static local_iterator s_local_iterator_to(reference value) BOOST_NOEXCEPT;

   //! <b>Requires</b>: value must be an lvalue and shall be in a unordered_set of
   //!   appropriate type. Otherwise the behavior is undefined.
   //!
   //! <b>Effects</b>: Returns: a valid const_local_iterator belonging to
   //!   the unordered_set that points to the value
   //!
   //! <b>Complexity</b>: Constant.
   //!
   //! <b>Throws</b>: Nothing.
   //!
   //! <b>Note</b>: This static function is available only if the <i>value traits</i>
   //!   is stateless.
   static const_local_iterator s_local_iterator_to(const_reference value) BOOST_NOEXCEPT;

   //! <b>Requires</b>: value must be an lvalue and shall be in a unordered_set of
   //!   appropriate type. Otherwise the behavior is undefined.
   //!
   //! <b>Effects</b>: Returns: a valid local_iterator belonging to the unordered_set
   //!   that points to the value
   //!
   //! <b>Complexity</b>: Constant.
   //!
   //! <b>Throws</b>: Nothing.
   local_iterator local_iterator_to(reference value) BOOST_NOEXCEPT;

   //! <b>Requires</b>: value must be an lvalue and shall be in a unordered_set of
   //!   appropriate type. Otherwise the behavior is undefined.
   //!
   //! <b>Effects</b>: Returns: a valid const_local_iterator belonging to
   //!   the unordered_set that points to the value
   //!
   //! <b>Complexity</b>: Constant.
   //!
   //! <b>Throws</b>: Nothing.
   const_local_iterator local_iterator_to(const_reference value) const BOOST_NOEXCEPT;

   //! <b>Effects</b>: Returns the number of buckets passed in the constructor
   //!   or the last rehash function.
   //!
   //! <b>Complexity</b>: Constant.
   //!
   //! <b>Throws</b>: Nothing.
   size_type bucket_count() const BOOST_NOEXCEPT;

   //! <b>Requires</b>: n is in the range [0, this->bucket_count()).
   //!
   //! <b>Effects</b>: Returns the number of elements in the nth bucket.
   //!
   //! <b>Complexity</b>: Constant.
   //!
   //! <b>Throws</b>: Nothing.
   size_type bucket_size(size_type n) const BOOST_NOEXCEPT;
   #endif  //#if defined(BOOST_INTRUSIVE_DOXYGEN_INVOKED)

   //! <b>Effects</b>: Returns the index of the bucket in which elements
   //!   with keys equivalent to k would be found, if any such element existed.
   //!
   //! <b>Complexity</b>: Constant.
   //!
   //! <b>Throws</b>: If the hash functor throws.
   //!
   //! <b>Note</b>: the return value is in the range [0, this->bucket_count()).
   BOOST_INTRUSIVE_FORCEINLINE size_type bucket(const key_type& k) const
   {  return this->priv_hash_to_nbucket(this->priv_hash(k));   }

   //! <b>Requires</b>: "hash_func" must be a hash function that induces
   //!   the same hash values as the stored hasher. The difference is that
   //!   "hash_func" hashes the given key instead of the value_type.
   //!
   //! <b>Effects</b>: Returns the index of the bucket in which elements
   //!   with keys equivalent to k would be found, if any such element existed.
   //!
   //! <b>Complexity</b>: Constant.
   //!
   //! <b>Throws</b>: If hash_func throws.
   //!
   //! <b>Note</b>: the return value is in the range [0, this->bucket_count()).
   template<class KeyType, class KeyHasher>
   BOOST_INTRUSIVE_FORCEINLINE size_type bucket(const KeyType& k, KeyHasher hash_func)  const
   {  return this->priv_hash_to_nbucket(hash_func(k));   }

   #if defined(BOOST_INTRUSIVE_DOXYGEN_INVOKED)
   //! <b>Effects</b>: Returns the bucket array pointer passed in the constructor
   //!   or the last rehash function.
   //!
   //! <b>Complexity</b>: Constant.
   //!
   //! <b>Throws</b>: Nothing.
   bucket_ptr bucket_pointer() const BOOST_NOEXCEPT;

   //! <b>Requires</b>: n is in the range [0, this->bucket_count()).
   //!
   //! <b>Effects</b>: Returns a local_iterator pointing to the beginning
   //!   of the sequence stored in the bucket n.
   //!
   //! <b>Complexity</b>: Constant.
   //!
   //! <b>Throws</b>: Nothing.
   //!
   //! <b>Note</b>:  [this->begin(n), this->end(n)) is a valid range
   //!   containing all of the elements in the nth bucket.
   local_iterator begin(size_type n) BOOST_NOEXCEPT;

   //! <b>Requires</b>: n is in the range [0, this->bucket_count()).
   //!
   //! <b>Effects</b>: Returns a const_local_iterator pointing to the beginning
   //!   of the sequence stored in the bucket n.
   //!
   //! <b>Complexity</b>: Constant.
   //!
   //! <b>Throws</b>: Nothing.
   //!
   //! <b>Note</b>:  [this->begin(n), this->end(n)) is a valid range
   //!   containing all of the elements in the nth bucket.
   const_local_iterator begin(size_type n) const BOOST_NOEXCEPT;

   //! <b>Requires</b>: n is in the range [0, this->bucket_count()).
   //!
   //! <b>Effects</b>: Returns a const_local_iterator pointing to the beginning
   //!   of the sequence stored in the bucket n.
   //!
   //! <b>Complexity</b>: Constant.
   //!
   //! <b>Throws</b>: Nothing.
   //!
   //! <b>Note</b>:  [this->begin(n), this->end(n)) is a valid range
   //!   containing all of the elements in the nth bucket.
   const_local_iterator cbegin(size_type n) const BOOST_NOEXCEPT;

   //! <b>Requires</b>: n is in the range [0, this->bucket_count()).
   //!
   //! <b>Effects</b>: Returns a local_iterator pointing to the end
   //!   of the sequence stored in the bucket n.
   //!
   //! <b>Complexity</b>: Constant.
   //!
   //! <b>Throws</b>: Nothing.
   //!
   //! <b>Note</b>:  [this->begin(n), this->end(n)) is a valid range
   //!   containing all of the elements in the nth bucket.
   local_iterator end(size_type n) BOOST_NOEXCEPT;

   //! <b>Requires</b>: n is in the range [0, this->bucket_count()).
   //!
   //! <b>Effects</b>: Returns a const_local_iterator pointing to the end
   //!   of the sequence stored in the bucket n.
   //!
   //! <b>Complexity</b>: Constant.
   //!
   //! <b>Throws</b>: Nothing.
   //!
   //! <b>Note</b>:  [this->begin(n), this->end(n)) is a valid range
   //!   containing all of the elements in the nth bucket.
   const_local_iterator end(size_type n) const BOOST_NOEXCEPT;

   //! <b>Requires</b>: n is in the range [0, this->bucket_count()).
   //!
   //! <b>Effects</b>: Returns a const_local_iterator pointing to the end
   //!   of the sequence stored in the bucket n.
   //!
   //! <b>Complexity</b>: Constant.
   //!
   //! <b>Throws</b>: Nothing.
   //!
   //! <b>Note</b>:  [this->begin(n), this->end(n)) is a valid range
   //!   containing all of the elements in the nth bucket.
   const_local_iterator cend(size_type n) const BOOST_NOEXCEPT;
   #endif   //#if defined(BOOST_INTRUSIVE_DOXYGEN_INVOKED)

   //! <b>Requires</b>: new_bucket_traits can hold a pointer to a new bucket array
   //!   or the same as the old bucket array with a different length. new_size is the length of the
   //!   the array pointed by new_buckets. If new_bucket_traits.bucket_begin() == this->bucket_pointer()
   //!   new_bucket_traits.bucket_count() can be bigger or smaller than this->bucket_count().
   //!   'new_bucket_traits' copy constructor should not throw.
   //!
   //! <b>Effects</b>:
   //!   If `new_bucket_traits.bucket_begin() == this->bucket_pointer()` is false,
   //!   unlinks values from the old bucket and inserts then in the new one according
   //!   to the hash value of values.
   //!
   //!   If `new_bucket_traits.bucket_begin() == this->bucket_pointer()` is true,
   //!   the implementations avoids moving values as much as possible.
   //!
   //!   Bucket traits hold by *this is assigned from new_bucket_traits.
   //!   If the container is configured as incremental<>, the split bucket is set
   //!   to the new bucket_count().
   //!
   //!   If store_hash option is true, this method does not use the hash function.
   //!   If false, the implementation tries to minimize calls to the hash function
   //!	 (e.g. once for equivalent values if optimize_multikey<true> is true).
   //!
   //!   If rehash is successful updates the internal bucket_traits with new_bucket_traits.
   //!
   //! <b>Complexity</b>: Average case linear in this->size(), worst case quadratic.
   //!
   //! <b>Throws</b>: If the hasher functor throws. Basic guarantee: all the elements
   //!   are unlinked (no destructors are called) and the container is left empty.
   BOOST_INTRUSIVE_FORCEINLINE void rehash(const bucket_traits &new_bucket_traits)
   {  this->priv_rehash_impl(new_bucket_traits, false, detail::bool_<store_hash>()); }

   //! <b>Note</b>: This function is used when keys from inserted elements are changed 
   //!  (e.g. a language change when key is a string) but uniqueness and hash properties are
   //!  preserved so a fast full rehash recovers invariants for *this without extracting and
   //!  reinserting all elements again.
   //!
   //! <b>Requires</b>: Calls produced to the hash function should not alter the value uniqueness
   //!  properties of already inserted elements. If hasher(key1) == hasher(key2) was true when
   //!  elements were inserted, it shall be true during calls produced in the execution of this function.
   //!
   //!  key_equal is not called inside this function so it is assumed that key_equal(value1, value2)
   //!  should produce the same results as before for inserted elements.
   //!
   //! <b>Effects</b>: Reprocesses all values hold by *this, recalculating their hash values
   //!   and redistributing them though the buckets.
   //!
   //!   If store_hash option is true, this method uses the hash function and updates the stored hash value.
   //!
   //! <b>Complexity</b>: Average case linear in this->size(), worst case quadratic.
   //!
   //! <b>Throws</b>: If the hasher functor throws. Basic guarantee: all the elements
   //!   are unlinked (no destructors are called) and the container is left empty.
   inline void full_rehash()
   {
      //A moved-from container has no bucket array and nothing to rehash
      if(this->priv_bucket_traits().bucket_count())
         this->priv_rehash_impl(this->priv_bucket_traits(), true, detail::false_());
   }

   //! <b>Effects</b>: Let s be this->split_count() and n be this->bucket_count().
   //!
   //!   If grow is true and s < n, increments the split count and moves the elements
   //!   of bucket (s - n/2) that hash to bucket s to bucket s.
   //!
   //!   If grow is false and s > n/2, moves all the elements of bucket (s - 1)
   //!   to bucket (s - 1 - n/2) and decrements the split count.
   //!
   //!   Otherwise, does nothing.
   //!
   //! <b>Returns</b>: true if the split count was changed, false otherwise.
   //!
   //! <b>Complexity</b>: Linear to the number of elements in the bucket that is split or merged.
   //!
   //! <b>Throws</b>: If grow is true and store_hash<true> is not set, if the internal
   //!   hasher throws. Strong guarantee.
   //!
   //! <b>Note</b>: this method is only available if incremental<true> option is activated.
   bool incremental_rehash(bool grow = true)
   {
      //This function is only available for containers with incremental hashing
      BOOST_INTRUSIVE_STATIC_ASSERT(( incremental && power_2_buckets ));
      const std::size_t split_idx  = this->split_count();
      const std::size_t bucket_cnt = this->bucket_count();
      bool ret = false;

      if(grow){
         //Test if the split variable can be changed
         ret = split_idx < bucket_cnt;
         if(ret){
            const std::size_t bucket_to_rehash = split_idx - bucket_cnt/2u;
            bucket_type &old_bucket = this->priv_bucket(bucket_to_rehash);
            this->inc_split_count();

            //Anti-exception stuff: if an exception is thrown while
            //moving elements from old_bucket to the target bucket, all moved
            //elements are moved back to the original one.
            incremental_rehash_rollback<bucket_type, split_traits, slist_node_algorithms> rollback
               ( this->priv_bucket(split_idx), old_bucket, this->priv_split_traits());
            node_ops_t::template priv_split_bucket<optimize_multikey, index_ops_t>
               ( this->priv_bucket_pointer(), this->priv_usable_bucket_count(), this->split_count()
               , bucket_to_rehash, this->priv_node_hasher(detail::bool_<store_hash>()));
            rollback.release();
            this->priv_erasure_update_cache();
         }
      }
      else{   //!grow
         ret = split_idx > bucket_cnt/2u;
         if(ret){
            const std::size_t target_bucket_num = split_idx - 1u - bucket_cnt/2u;
            bucket_type &target_bucket = this->priv_bucket(target_bucket_num);
            bucket_type &source_bucket = this->priv_bucket(split_idx-1u);
            //The cached first bucket is only updated if elements are moved to the target bucket
            const bool source_is_empty = slist_node_algorithms::is_empty(source_bucket.get_node_ptr());
            slist_node_algorithms::transfer_after(target_bucket.get_node_ptr(), source_bucket.get_node_ptr());
            this->dec_split_count();
            if(!source_is_empty)
               this->priv_insertion_update_cache(target_bucket_num);
         }
      }
      return ret;
   }

   //! <b>Requires</b>: 'new_bucket_traits' copy assignment should not throw.
   //!
   //! <b>Effects</b>: If new_bucket_traits.bucket_count() is not
   //!   this->bucket_count()/2 or this->bucket_count()*2, or
   //!   this->split_bucket() != new_bucket_traits.bucket_count() returns false
   //!   and does nothing.
   //!
   //!   Otherwise, copy assigns new_bucket_traits to the internal bucket_traits
   //!   and transfers all the objects from old buckets to the new ones.
   //!
   //! <b>Complexity</b>: Linear to size().
   //!
   //! <b>Throws</b>: Nothing
   //!
   //! <b>Note</b>: this method is only available if incremental<true> option is activated.
   bool incremental_rehash(const bucket_traits &new_bucket_traits) BOOST_NOEXCEPT
   {
      //This function is only available for containers with incremental hashing
      BOOST_INTRUSIVE_STATIC_ASSERT(( incremental && power_2_buckets ));
      const bucket_ptr new_buckets = new_bucket_traits.bucket_begin();
      const size_type new_bucket_count_stdszt = static_cast<SizeType>(new_bucket_traits.bucket_count() - bucket_overhead);
      BOOST_INTRUSIVE_INVARIANT_ASSERT(sizeof(size_type) >= sizeof(std::size_t) || new_bucket_count_stdszt <= size_type(-1));
      size_type new_bucket_count = static_cast<size_type>(new_bucket_count_stdszt);
      const size_type old_bucket_count = static_cast<size_type>(this->priv_usable_bucket_count());
      const size_type split_idx = this->split_count();

      //Test new bucket size is consistent with internal bucket size and split count
      if(new_bucket_count/2 == old_bucket_count){
         if(!(split_idx >= old_bucket_count))
            return false;
      }
      else if(new_bucket_count == old_bucket_count/2){
         if(!(split_idx <= new_bucket_count))
            return false;
      }
      else{
         return false;
      }

      const size_type ini_n = (size_type)this->priv_get_cache_bucket_num();
      const bucket_ptr old_buckets = this->priv_bucket_pointer();


      this->priv_unset_sentinel_bucket();
      this->priv_initialize_new_buckets(old_buckets, old_bucket_count, new_buckets, new_bucket_count);
      if (&new_bucket_traits != &this->priv_bucket_traits())
         this->priv_bucket_traits() = new_bucket_traits;

      if(old_buckets != new_buckets){
         for(size_type n = ini_n; n < split_idx; ++n){
            slist_node_ptr new_bucket_nodeptr = new_bucket_traits.bucket_begin()[difference_type(n)].get_node_ptr();
            slist_node_ptr old_bucket_node_ptr = old_buckets[difference_type(n)].get_node_ptr();
            slist_node_algorithms::transfer_after(new_bucket_nodeptr, old_bucket_node_ptr);
         }
      }
      if(ini_n >= old_bucket_count){
         //The container was empty, so the cache was the end of the old bucket
         //array, that is not the end of the new one.
         this->priv_init_cache();
      }
      else if(old_buckets != new_buckets){
         //Reset cache to safe position
         this->priv_set_cache_bucket_num(ini_n);
      }

      this->priv_set_sentinel_bucket();
      return true;
   }

   #if defined(BOOST_INTRUSIVE_DOXYGEN_INVOKED)

   //! <b>Requires</b>: incremental<> option must be set
   //!
   //! <b>Effects</b>: returns the current split count
   //!
   //! <b>Complexity</b>: Constant
   //!
   //! <b>Throws</b>: Nothing
   size_type split_count() const BOOST_NOEXCEPT;

   //! <b>Effects</b>: Returns the nearest new bucket count optimized for
   //!   the container that is bigger or equal than n. This suggestion can be
   //!   used to create bucket arrays with a size that will usually improve
   //!   container's performance. If such value does not exist, the
   //!   highest possible value that size_type can represent is returned.
   //!
   //! <b>Complexity</b>: Amortized constant time.
   //!
   //! <b>Throws</b>: Nothing.
   static size_type suggested_upper_bucket_count(size_type n) BOOST_NOEXCEPT;

   //! <b>Effects</b>: Returns the nearest new bucket count optimized for
   //!   the container that is smaller or equal than n. This suggestion can be
   //!   used to create bucket arrays with a size that will usually improve
   //!   container's performance. If such value does not exist, the
   //!   lowest possible value is returned.
   //!
   //! <b>Complexity</b>: Amortized constant time.
   //!
   //! <b>Throws</b>: Nothing.
   static size_type suggested_lower_bucket_count(size_type n) BOOST_NOEXCEPT;
   #endif   //#if defined(BOOST_INTRUSIVE_DOXYGEN_INVOKED)


   friend bool operator==(const hashtable_impl &x, const hashtable_impl &y)
   {
      //Taken from N3068
      //Sizes are compared at the end if they are not constant time,
      //as y might hold additional elements not found in x.
      BOOST_IF_CONSTEXPR(constant_time_size)
      if(x.size() != y.size()){
         return false;
      }

      size_type x_size = 0u;
      for (const_iterator ix = x.cbegin(), ex = x.cend(); ix != ex;){
         std::pair<const_iterator, const_iterator> eqx(x.equal_range(key_of_value()(*ix))),
                                                   eqy(y.equal_range(key_of_value()(*ix)));
         const std::size_t x_group_size = boost::intrusive::iterator_udistance(eqx.first, eqx.second);
         if (x_group_size != boost::intrusive::iterator_udistance(eqy.first, eqy.second) ||
               !(priv_algo_is_permutation)(eqx.first, eqx.second, eqy.first)      ){
            return false;
         }
         x_size += size_type(x_group_size);
         ix = eqx.second;
      }
      return constant_time_size || x_size == y.size();
   }

   BOOST_INTRUSIVE_FORCEINLINE friend bool operator!=(const hashtable_impl &x, const hashtable_impl &y)
   {  return !(x == y); }

   BOOST_INTRUSIVE_FORCEINLINE friend bool operator<(const hashtable_impl &x, const hashtable_impl &y)
   {  return ::boost::intrusive::algo_lexicographical_compare(x.begin(), x.end(), y.begin(), y.end());  }

   BOOST_INTRUSIVE_FORCEINLINE friend bool operator>(const hashtable_impl &x, const hashtable_impl &y)
   {  return y < x;  }

   BOOST_INTRUSIVE_FORCEINLINE friend bool operator<=(const hashtable_impl &x, const hashtable_impl &y)
   {  return !(y < x);  }

   BOOST_INTRUSIVE_FORCEINLINE friend bool operator>=(const hashtable_impl &x, const hashtable_impl &y)
   {  return !(x < y);  }

   /// @cond
   BOOST_INTRUSIVE_FORCEINLINE void check() const {}
   private:

   BOOST_INTRUSIVE_FORCEINLINE static void priv_initialize_new_buckets
      ( bucket_ptr old_buckets, size_type old_bucket_count
      , bucket_ptr new_buckets, size_type new_bucket_count)
   {  node_ops_t::priv_initialize_new_buckets(old_buckets, old_bucket_count, new_buckets, new_bucket_count);  }

   //Hash functor for the node-level rehash operations: the stored hash value
   //(shared by all value types) or the hash value calculated with the hasher
   typedef hashtable_value_hasher<value_traits, hasher, key_of_value>   value_hasher_t;

   BOOST_INTRUSIVE_FORCEINLINE hashtable_stored_hash<node_traits> priv_node_hasher(detail::true_)
   {  return hashtable_stored_hash<node_traits>();  }

   BOOST_INTRUSIVE_FORCEINLINE value_hasher_t priv_node_hasher(detail::false_)
   {  return value_hasher_t(this->priv_value_traits(), this->priv_hasher());  }

   //If "do_full_rehash" is true, hash values are recalculated and stored again in nodes that store
   //the hash. If UseStoredHash is true (only if !do_full_rehash), stored hash values are used.
   template<bool UseStoredHash>
   void priv_rehash_impl(const bucket_traits &new_bucket_traits, bool do_full_rehash, detail::bool_<UseStoredHash> use_stored_hash)
   {
      const std::size_t nbc             = new_bucket_traits.bucket_count() - bucket_overhead;
      BOOST_INTRUSIVE_INVARIANT_ASSERT(sizeof(SizeType) >= sizeof(std::size_t) || nbc <= SizeType(-1));

      const bucket_ptr new_buckets      = new_bucket_traits.bucket_begin();
      const size_type  new_bucket_count = static_cast<SizeType>(nbc);
      const bucket_ptr old_buckets      = this->priv_bucket_pointer();
      const size_type  old_bucket_count = this->bucket_count();

      //Check power of two bucket array if the option is activated
      BOOST_INTRUSIVE_INVARIANT_ASSERT
         (!power_2_buckets || (0 == (new_bucket_count & (new_bucket_count-1u))));

      const bool same_buffer = old_buckets == new_buckets;
      //If the new bucket length is a common factor
      //of the old one we can avoid hash calculations.
      const bool fast_shrink = (!do_full_rehash) && (!incremental) && (old_bucket_count >= new_bucket_count) &&
         (power_2_buckets || (old_bucket_count % new_bucket_count) == 0);
      //If we are shrinking the same bucket array and it's
      //is a fast shrink, just rehash the last nodes
      size_type new_first_bucket_num = new_bucket_count;
      size_type old_bucket_cache = (size_type)this->priv_get_cache_bucket_num();
      if(same_buffer && fast_shrink && (old_bucket_cache < new_bucket_count)){
         new_first_bucket_num = old_bucket_cache;
         old_bucket_cache = new_bucket_count;
      }

      if (!do_full_rehash)
         this->priv_initialize_new_buckets(old_buckets, old_bucket_count, new_buckets, new_bucket_count);

      //Anti-exception stuff: they destroy the elements if something goes wrong.
      //If the source and destination buckets are the same, the second rollback function
      //is harmless, because all elements have been already unlinked and destroyed

      typedef typename internal_type::template typeof_node_disposer<detail::null_disposer>::type NodeDisposer;
      typedef exception_bucket_disposer<bucket_type, slist_node_algorithms, NodeDisposer, size_type> ArrayDisposer;
      NodeDisposer nd(this->make_node_disposer(detail::null_disposer()));
      //The sentinel bucket of the old array is unset before the hasher is called. It must be
      //destroyed after the rollback functions, as with a bigger new bucket array in the same
      //buffer the old sentinel bucket is a bucket of the new array.
      exception_sentinel_bucket_restorer<internal_type> rollback_sentinel(*this);
      ArrayDisposer rollback1(boost::movelib::to_raw_pointer(new_buckets), nd, new_bucket_count);
      ArrayDisposer rollback2(boost::movelib::to_raw_pointer(old_buckets), nd, old_bucket_count);

      //Put size in a safe value for rollback exception
      size_type const size_backup = this->priv_size_traits().get_size();
      this->priv_size_traits().set_size(0);
      //Put cache to safe position
      this->priv_init_cache();
      this->priv_unset_sentinel_bucket();

      const size_type split = this->rehash_split_from_bucket_count(new_bucket_count);

      //Move the nodes to the new buckets
      new_first_bucket_num = (size_type)node_ops_t::template priv_rehash_nodes<optimize_multikey, index_ops_t>
         ( old_buckets, old_bucket_cache, old_bucket_count, new_buckets, new_bucket_count, split
         , fast_shrink, do_full_rehash && store_hash, new_first_bucket_num, this->priv_node_hasher(use_stored_hash));

      this->priv_size_traits().set_size(size_backup);
      this->split_count(split);
      if(&new_bucket_traits != &this->priv_bucket_traits())
         this->priv_bucket_traits() = new_bucket_traits;
      this->priv_set_sentinel_bucket();
      this->priv_set_cache_bucket_num(new_first_bucket_num);
      rollback1.release();
      rollback2.release();
      rollback_sentinel.release();
   }

   template <class MaybeConstHashtableImpl, class Cloner, class Disposer>
   void priv_clone_from(MaybeConstHashtableImpl &src, Cloner cloner, Disposer disposer)
   {
      this->clear_and_dispose(disposer);
      this->priv_hasher() = src.priv_hasher();
      this->priv_equal()  = src.priv_equal();
      //An empty source can be a moved-from container without bucket array
      if(!src.empty()){
         const size_type src_bucket_count = src.bucket_count();
         const size_type dst_bucket_count = this->bucket_count();
         //Check power of two bucket array if the option is activated
         BOOST_INTRUSIVE_INVARIANT_ASSERT
            (!power_2_buckets || (0 == (src_bucket_count & (src_bucket_count-1))));
         BOOST_INTRUSIVE_INVARIANT_ASSERT
            (!power_2_buckets || (0 == (dst_bucket_count & (dst_bucket_count-1))));
         //If src bucket count is bigger or equal, structural copy is possible
         const bool structural_copy = (!incremental) && (src_bucket_count >= dst_bucket_count) &&
            (power_2_buckets || (src_bucket_count % dst_bucket_count) == 0);
         if(structural_copy){
            this->priv_structural_clone_from(src, cloner, disposer);
         }
         else{
            //Unlike previous cloning algorithm, this can throw
            //if cloner, hasher or comparison functor throw
            typedef typename detail::if_c< detail::is_const<MaybeConstHashtableImpl>::value
                                         , typename MaybeConstHashtableImpl::const_iterator
                                         , typename MaybeConstHashtableImpl::iterator
                                         >::type clone_iterator;
            clone_iterator b(src.begin()), e(src.end());
            detail::exception_disposer<hashtable_impl, Disposer> rollback(*this, disposer);
            for(; b != e; ++b){
               //No need to check for duplicates and insert it in the first position
               //as this is an unordered container. So use minimal insertion code
               std::size_t const hash_to_store = this->priv_stored_or_compute_hash(*b, store_hash_t());
               size_type const bucket_number = this->priv_hash_to_nbucket(hash_to_store);
               typedef typename detail::if_c
                  <detail::is_const<MaybeConstHashtableImpl>::value, const_reference, reference>::type reference_type;
               reference_type r = *b;
               this->priv_clone_front_in_bucket<reference_type>(bucket_number, r, hash_to_store, cloner);
            }
            rollback.release();
         }
      }
   }

   template<class ValueReference, class Cloner>
   void priv_clone_front_in_bucket( size_type const bucket_number
                                  , typename detail::identity<ValueReference>::type src_ref
                                  , std::size_t const hash_to_store, Cloner cloner)
   {
      //No need to check for duplicates and insert it in the first position
      //as this is an unordered container. So use minimal insertion code
      bucket_type &cur_bucket = this->priv_bucket(bucket_number);
      siterator const prev(cur_bucket.get_node_ptr());
      //Just check if the cloned node is equal to the first inserted value in the new bucket
      //as equal src values were contiguous and they should be already inserted in the
      //destination bucket.
      bool const next_is_in_group = optimize_multikey && !this->priv_bucket_empty(bucket_number) &&
         this->priv_equal()( key_of_value()(src_ref)
                           , key_of_value()(this->priv_value_from_siterator(++siterator(prev))));
      this->priv_insert_equal_after_find(*cloner(src_ref), bucket_number, hash_to_store, prev, next_is_in_group);
   }

   template <class MaybeConstHashtableImpl, class Cloner, class Disposer>
   void priv_structural_clone_from(MaybeConstHashtableImpl &src, Cloner cloner, Disposer disposer)
   {
      //First clone the first ones
      const size_type src_bucket_count = src.bucket_count();
      const size_type dst_bucket_count = this->bucket_count();
      size_type constructed = 0;
      typedef typename internal_type::template typeof_node_disposer<Disposer>::type NodeDisposer;
      NodeDisposer node_disp(disposer, &this->priv_value_traits());

      //If the cloner throws, the clones are in the buckets of the destination
      //array (smaller or bigger than the number of source buckets already processed)
      exception_bucket_disposer<bucket_type, slist_node_algorithms, NodeDisposer, size_type>
         rollback(boost::movelib::to_raw_pointer(this->priv_bucket_pointer()), node_disp, dst_bucket_count);
      BOOST_INTRUSIVE_TRY{
         //Now insert the remaining ones using the modulo trick
         for( //"constructed" already initialized
            ; constructed < src_bucket_count
            ; ++constructed){

            const size_type new_n = (size_type)index_ops_t::hash_to_bucket
               (constructed, dst_bucket_count, this->split_count());
            bucket_type &src_b = src.priv_bucket(constructed);
            for( siterator b(node_ops_t::priv_bucket_lbegin(src_b)), e(node_ops_t::priv_bucket_lend(src_b)); b != e; ++b){
               typedef typename detail::if_c
                  <detail::is_const<MaybeConstHashtableImpl>::value, const_reference, reference>::type reference_type;
               reference_type r = this->priv_value_from_siterator(b);
               this->priv_clone_front_in_bucket<reference_type>
                  (new_n, r, node_ops_t::priv_stored_hash(b, store_hash_t()), cloner);
            }
         }
      }
      BOOST_INTRUSIVE_CATCH(...){
         //The rollback disposes all the clones, the container will be empty
         //but we must update size and the cached first bucket
         this->priv_size_traits().set_size(size_type(0));
         this->priv_init_cache();
         BOOST_INTRUSIVE_RETHROW;
      }
      BOOST_INTRUSIVE_CATCH_END
      rollback.release();
      this->priv_size_traits().set_size(src.priv_size_traits().get_size());
      //split_count is not modified as it depends on this bucket array
      this->priv_set_cache_bucket_num(0u);
      this->priv_erasure_update_cache();
   }

   iterator priv_insert_equal_after_find(reference value, size_type bucket_num, std::size_t hash_value, siterator prev, bool const next_is_in_group)
   {
      node_ptr n = this->priv_value_to_node_ptr(value);
      node_ops_t::template priv_link_after<optimize_multikey, safemode_or_autounlink>
         (prev.pointed_node(), n, hash_value, next_is_in_group);
      //Update cache and increment size if needed
      this->priv_insertion_update_cache(bucket_num);
      this->priv_size_traits().increment();
      return this->build_iterator(siterator(n), this->priv_bucket_ptr(bucket_num));
   }

   template<class KeyType, class KeyHasher, class KeyEqual>
   siterator priv_find  //In case it is not found previt is priv_end_sit()
      ( const KeyType &key,  KeyHasher hash_func
      , KeyEqual equal_func, size_type &bucket_number, std::size_t &h, siterator &previt) const
   {
      h = hash_func(key);

      bucket_number = this->priv_hash_to_nbucket(h);
      bucket_type& b = this->priv_bucket(bucket_number);
      siterator prev = node_ops_t::sit_bbegin(b);
      siterator it = prev;
      siterator const endit = node_ops_t::sit_end(b);

      while (++it != endit) {
         if (this->priv_is_value_equal_to_key
               (this->priv_value_from_siterator(it), h, key, equal_func, compare_hash_t())) {
            previt = prev;
            return it;
         }
         (priv_go_to_last_in_group)(it, optimize_multikey_t());
         prev = it;
      }
      previt = b.get_node_ptr();
      return this->priv_end_sit();
   }


   template<class KeyType, class KeyEqual>
   siterator priv_find_in_bucket  //In case it is not found previt is priv_end_sit()
      (bucket_type &b, const KeyType& key, KeyEqual equal_func, const std::size_t h) const
   {
      siterator it(node_ops_t::sit_begin(b));
      siterator const endit(node_ops_t::sit_end(b));

      for (; it != endit; (priv_go_to_last_in_group)(it, optimize_multikey_t()), ++it) {
         if (BOOST_LIKELY(this->priv_is_value_equal_to_key
               (this->priv_value_from_siterator(it), h, key, equal_func, compare_hash_t()))) {
            return it;
         }
      }
      return this->priv_end_sit();
   }

   template<class KeyType, class KeyEqual>
   inline bool priv_is_value_equal_to_key
      (const value_type &v, const std::size_t h, const KeyType &key, KeyEqual equal_func, detail::true_) const //compare_hash
   {  return this->priv_stored_or_compute_hash(v, store_hash_t()) == h && equal_func(key, key_of_value()(v));  }

   template<class KeyType, class KeyEqual>
   inline bool priv_is_value_equal_to_key
      (const value_type& v, const std::size_t , const KeyType& key, KeyEqual equal_func, detail::false_) const //compare_hash
   {  return equal_func(key, key_of_value()(v));   }

   //return previous iterator to the next equal range group in case
   inline static void priv_go_to_last_in_group
      (siterator &it_first_in_group, detail::true_) BOOST_NOEXCEPT  //optimize_multikey
   {
      it_first_in_group =
         (group_functions_t::get_last_in_group
            (dcast_bucket_ptr<node>(it_first_in_group.pointed_node()), optimize_multikey_t()));
   }

   //return previous iterator to the next equal range group in case
   inline static void priv_go_to_last_in_group        //!optimize_multikey
      (siterator /*&it_first_in_group*/, detail::false_) BOOST_NOEXCEPT
   { }

   //Searches the elements equivalent to key. If found, returns the first one, "previt" is the
   //node before it and "last" is the end of the equivalent elements in the bucket (the end of
   //the bucket if they are the last ones). If not found, returns priv_end_sit().
   //If "pcnt" is not null, *pcnt is the number of equivalent elements (zero if not found).
   //If Unique, at most one element is equivalent to key.
   template<class KeyType, class KeyHasher, class KeyEqual, bool Unique>
   siterator priv_find_range
      ( const KeyType &key, KeyHasher hash_func, KeyEqual equal_func, detail::bool_<Unique>
      , size_type &bucket_number, siterator &previt, siterator &last, std::size_t *pcnt) const
   {
      std::size_t h;
      siterator const first = this->priv_find(key, hash_func, equal_func, bucket_number, h, previt);
      std::size_t cnt = 0;
      if(first != this->priv_end_sit()){
         last = first;
         BOOST_IF_CONSTEXPR(optimize_multikey){
            (priv_go_to_last_in_group)(last, optimize_multikey_t());
            ++last;
            if(pcnt)
               cnt = boost::intrusive::iterator_udistance(first, last);
         }
         else{
            siterator const bend = this->priv_bucket_lend(bucket_number);
            do{
               ++cnt;   //At least one is found
               ++last;
            }while(!Unique && last != bend &&
                  this->priv_is_value_equal_to_key
                  (this->priv_value_from_siterator(last), h, key, equal_func, compare_hash_t()));
         }
      }
      if(pcnt)
         *pcnt = cnt;
      return first;
   }

   //If Unique, at most one element is equivalent to key
   template<class KeyType, class KeyHasher, class KeyEqual, class Disposer, bool Unique>
   size_type priv_erase_and_dispose(const KeyType& key, KeyHasher hash_func
                              ,KeyEqual equal_func, Disposer disposer, detail::bool_<Unique> unique_tag)
   {
      size_type bucket_num;
      siterator prev, last;
      std::size_t cnt = 0;
      //With optimize_multikey, priv_erase_from_single_bucket counts the erased elements
      siterator const first = this->priv_find_range
         (key, hash_func, equal_func, unique_tag, bucket_num, prev, last, optimize_multikey ? 0 : &cnt);

      if(first != this->priv_end_sit()){
         BOOST_IF_CONSTEXPR(optimize_multikey){
            cnt = node_ops_t::priv_erase_from_single_bucket
               ( this->priv_bucket(bucket_num), prev, last
               , this->make_node_disposer(disposer), optimize_multikey_t());
         }
         else{
            slist_node_algorithms::unlink_after_and_dispose(prev.pointed_node(), last.pointed_node(), this->make_node_disposer(disposer));
         }
         this->priv_size_traits().set_size(size_type(this->priv_size_traits().get_size()-cnt));
         this->priv_erasure_update_cache();
      }

      return static_cast<size_type>(cnt);
   }

   template<class KeyType, class KeyHasher, class KeyEqual, bool Unique>
   std::pair<siterator, siterator> priv_local_equal_range
      ( const KeyType &key
      , KeyHasher hash_func
      , KeyEqual equal_func
      , size_type &found_bucket
      , size_type &cnt
      , detail::bool_<Unique> unique_tag) const  //If Unique, at most one element is equivalent to key
   {
      std::size_t internal_cnt;
      siterator prev, last;
      size_type n_bucket;
      std::pair<siterator, siterator> to_return
         ( this->priv_find_range(key, hash_func, equal_func, unique_tag, n_bucket, prev, last, &internal_cnt)
         , this->priv_end_sit());

      if(to_return.first != to_return.second){
         found_bucket = n_bucket;
         if (last != this->priv_bucket_lend(n_bucket))
            to_return.second = last;
      }
      cnt = size_type(internal_cnt);
      return to_return;
   }

   struct priv_equal_range_result
   {
      siterator first;
      siterator second;
      bucket_ptr bucket_first;
      bucket_ptr bucket_second;
   };

   template<class KeyType, class KeyHasher, class KeyEqual, bool Unique>
   priv_equal_range_result priv_equal_range
      ( const KeyType &key
      , KeyHasher hash_func
      , KeyEqual equal_func
      , detail::bool_<Unique> unique_tag) const
   {
      //Avoid false GCC -Wmaybe-uninitialized warning
      size_type n_bucket = 0;
      size_type cnt;

      //Let's see if the element is present
      const std::pair<siterator, siterator> to_return
         (this->priv_local_equal_range(key, hash_func, equal_func, n_bucket, cnt, unique_tag));
      priv_equal_range_result r;
      r.first = to_return.first;
      r.second = to_return.second;

      //If not, find the next element as ".second" if ".second" local iterator
      //is not pointing to an element.
      if(to_return.first == to_return.second) {
         r.bucket_first = r.bucket_second = this->priv_invalid_bucket_ptr();
      }
      else if (to_return.second != this->priv_end_sit()) {
         r.bucket_first = this->priv_bucket_ptr(n_bucket);
      }
      else{
         r.bucket_first = this->priv_bucket_ptr(n_bucket);
         const size_type max_bucket = this->bucket_count();
         do{
            ++n_bucket;
         } while (n_bucket != max_bucket && this->priv_bucket_empty(n_bucket));

         if (n_bucket == max_bucket){
            r.bucket_second = this->priv_invalid_bucket_ptr();
         }
         else{
            r.bucket_second = this->priv_bucket_ptr(n_bucket);
            r.second = siterator(r.bucket_second->begin_ptr());
         }
      }

      return r;
   }

   BOOST_INTRUSIVE_FORCEINLINE size_type priv_get_bucket_num(const_iterator it) BOOST_NOEXCEPT
   {  return this->priv_get_bucket_num(it, linear_buckets_t());  }

   BOOST_INTRUSIVE_FORCEINLINE size_type priv_get_bucket_num(const_iterator it, detail::true_) BOOST_NOEXCEPT //linear
   {  return size_type(it.get_bucket_ptr() - this->priv_bucket_pointer());   }

   BOOST_INTRUSIVE_FORCEINLINE size_type priv_get_bucket_num(const_iterator it, detail::false_) BOOST_NOEXCEPT //!linear
   {  return this->priv_get_bucket_num_hash_dispatch(it.slist_it(), store_hash_t());  }

   BOOST_INTRUSIVE_FORCEINLINE size_type priv_get_bucket_num_hash_dispatch(siterator it, detail::true_) BOOST_NOEXCEPT    //store_hash
   {  return (size_type)this->priv_hash_to_nbucket(node_ops_t::priv_stored_hash(it, store_hash_t()));  }

   BOOST_INTRUSIVE_FORCEINLINE size_type priv_get_bucket_num_hash_dispatch(siterator it, detail::false_) BOOST_NOEXCEPT   //NO store_hash
   {
      return static_cast<size_type>(node_ops_t::template priv_bucket_num_from_node<optimize_multikey>
         (this->priv_bucket_pointer(), this->priv_usable_bucket_count(), it.pointed_node()));
   }


   BOOST_INTRUSIVE_FORCEINLINE bucket_ptr priv_get_bucket_ptr(const_iterator it) BOOST_NOEXCEPT
   {  return this->priv_get_bucket_ptr(it, linear_buckets_t());  }

   BOOST_INTRUSIVE_FORCEINLINE bucket_ptr priv_get_bucket_ptr(const_iterator it, detail::true_) BOOST_NOEXCEPT //linear
   {  return it.get_bucket_ptr();   }

   BOOST_INTRUSIVE_FORCEINLINE bucket_ptr priv_get_bucket_ptr(const_iterator it, detail::false_) BOOST_NOEXCEPT //!linear
   {  return this->priv_bucket_ptr(this->priv_get_bucket_num_hash_dispatch(it.slist_it(), store_hash_t()));  }

   /// @endcond
};

/// @cond
template < class T
         , class PackedOptions
         >
struct make_bucket_traits
{
   //Real value traits must be calculated from options
   typedef typename detail::get_value_traits
      <T, typename PackedOptions::proto_value_traits>::type value_traits;

   typedef typename PackedOptions::bucket_traits            specified_bucket_traits;

   //Real bucket traits must be calculated from options and calculated value_traits
   typedef bucket_traits_impl
      < typename unordered_bucket_ptr_impl
         <value_traits>::type
      , std::size_t>                                        bucket_traits_t;

   typedef typename
      detail::if_c< detail::is_same
                     < specified_bucket_traits
                     , default_bucket_traits
                     >::value
                  , bucket_traits_t
                  , specified_bucket_traits
                  >::type                                type;
};
/// @endcond

//! Helper metafunction to define a \c hashtable that yields to the same type when the
//! same options (either explicitly or implicitly) are used.
#if defined(BOOST_INTRUSIVE_DOXYGEN_INVOKED) || defined(BOOST_INTRUSIVE_VARIADIC_TEMPLATES)
template<class T, class ...Options>
#else
template<class T, class O1 = void, class O2 = void
                , class O3 = void, class O4 = void
                , class O5 = void, class O6 = void
                , class O7 = void, class O8 = void
                , class O9 = void, class O10= void
                , class O11= void
                >
#endif
struct make_hashtable
{
   /// @cond
   typedef typename pack_options
      < hashtable_defaults,
         #if !defined(BOOST_INTRUSIVE_VARIADIC_TEMPLATES)
         O1, O2, O3, O4, O5, O6, O7, O8, O9, O10, O11
         #else
         Options...
         #endif
      >::type packed_options;

   typedef typename detail::get_value_traits
      <T, typename packed_options::proto_value_traits>::type value_traits;

   typedef typename make_bucket_traits
            <T, packed_options>::type bucket_traits;

   typedef hashtable_impl
      < value_traits
      , typename packed_options::key_of_value
      , typename packed_options::hash
      , typename packed_options::equal
      , bucket_traits
      , typename packed_options::size_type
      ,  (std::size_t(false)*hash_bool_flags::unique_keys_pos)
        |(std::size_t(packed_options::constant_time_size)*hash_bool_flags::constant_time_size_pos)
        |(std::size_t(packed_options::power_2_buckets)*hash_bool_flags::power_2_buckets_pos)
        |(std::size_t(packed_options::cache_begin)*hash_bool_flags::cache_begin_pos)
        |(std::size_t(packed_options::compare_hash)*hash_bool_flags::compare_hash_pos)
        |(std::size_t(packed_options::incremental)*hash_bool_flags::incremental_pos)
        |(std::size_t(packed_options::linear_buckets)*hash_bool_flags::linear_buckets_pos)
        |(std::size_t(packed_options::fastmod_buckets)*hash_bool_flags::fastmod_buckets_pos)
      > implementation_defined;

   /// @endcond
   typedef implementation_defined type;
};

#if !defined(BOOST_INTRUSIVE_DOXYGEN_INVOKED)

#if defined(BOOST_INTRUSIVE_VARIADIC_TEMPLATES)
template<class T, class ...Options>
#else
template<class T, class O1, class O2, class O3, class O4, class O5, class O6, class O7, class O8, class O9, class O10>
#endif
class hashtable
   :  public make_hashtable<T,
         #if !defined(BOOST_INTRUSIVE_VARIADIC_TEMPLATES)
         O1, O2, O3, O4, O5, O6, O7, O8, O9, O10
         #else
         Options...
         #endif
         >::type
{
   typedef typename make_hashtable<T,
      #if !defined(BOOST_INTRUSIVE_VARIADIC_TEMPLATES)
      O1, O2, O3, O4, O5, O6, O7, O8, O9, O10
      #else
      Options...
      #endif
      >::type   Base;
   BOOST_MOVABLE_BUT_NOT_COPYABLE(hashtable)

   public:
   typedef typename Base::value_traits       value_traits;
   typedef typename Base::iterator           iterator;
   typedef typename Base::const_iterator     const_iterator;
   typedef typename Base::bucket_ptr         bucket_ptr;
   typedef typename Base::size_type          size_type;
   typedef typename Base::hasher             hasher;
   typedef typename Base::bucket_traits      bucket_traits;
   typedef typename Base::key_equal          key_equal;

   //Assert if passed value traits are compatible with the type
   BOOST_INTRUSIVE_STATIC_ASSERT((detail::is_same<typename value_traits::value_type, T>::value));

   inline explicit hashtable ( const bucket_traits &b_traits
             , const hasher & hash_func = hasher()
             , const key_equal &equal_func = key_equal()
             , const value_traits &v_traits = value_traits())
      :  Base(b_traits, hash_func, equal_func, v_traits)
   {}

   BOOST_INTRUSIVE_FORCEINLINE hashtable(BOOST_RV_REF(hashtable) x)
      :  Base(BOOST_MOVE_BASE(Base, x))
   {}

   BOOST_INTRUSIVE_FORCEINLINE hashtable& operator=(BOOST_RV_REF(hashtable) x)
   {  return static_cast<hashtable&>(this->Base::operator=(BOOST_MOVE_BASE(Base, x)));  }

   template <class Cloner, class Disposer>
   BOOST_INTRUSIVE_FORCEINLINE void clone_from(const hashtable &src, Cloner cloner, Disposer disposer)
   {  Base::clone_from(src, cloner, disposer);  }

   template <class Cloner, class Disposer>
   BOOST_INTRUSIVE_FORCEINLINE void clone_from(BOOST_RV_REF(hashtable) src, Cloner cloner, Disposer disposer)
   {  Base::clone_from(BOOST_MOVE_BASE(Base, src), cloner, disposer);  }
};

#endif

} //namespace intrusive
} //namespace boost

#include <boost/intrusive/detail/config_end.hpp>

#endif //BOOST_INTRUSIVE_HASHTABLE_HPP
