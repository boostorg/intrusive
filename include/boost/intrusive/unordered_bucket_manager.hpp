//////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Ion Gaztanaga 2026-2026.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/move for documentation.
//
//////////////////////////////////////////////////////////////////////////////

#ifndef BOOST_INTRUSIVE_UNORDERED_BUCKET_MANAGER_HPP
#define BOOST_INTRUSIVE_UNORDERED_BUCKET_MANAGER_HPP

#include <boost/intrusive/detail/config_begin.hpp>
#include <boost/intrusive/detail/workaround.hpp>
#include <boost/intrusive/detail/ebo_functor_holder.hpp>
#include <boost/intrusive/detail/mpl.hpp>
#include <boost/intrusive/intrusive_fwd.hpp>
#include <boost/intrusive/hashtable.hpp>   //prime_list_holder
#include <boost/intrusive/pointer_traits.hpp>

#include <boost/config.hpp>
#include <boost/assert.hpp>

//boost.move (the only non-Intrusive Boost dependency)
#include <boost/move/core.hpp>
#include <boost/move/utility_core.hpp>
#include <boost/move/adl_move_swap.hpp>
#include <boost/move/detail/to_raw_pointer.hpp>

#include <cstddef>   //std::size_t
#include <new>       //placement new

#if defined(BOOST_HAS_PRAGMA_ONCE)
#  pragma once
#endif

namespace boost {
namespace intrusive {
namespace detail {

//The allocator used when the manager is instantiated with `void`: it takes
//the memory for the bucket array from the global operator new. It holds no
//state, so the manager keeps the empty base optimization.
template<class T>
struct operator_new_allocator
{
   typedef T                     value_type;
   typedef T *                   pointer;
   typedef std::size_t           size_type;

   pointer allocate(size_type n)
   {  return static_cast<pointer>(::operator new(n * sizeof(T)));  }

   void deallocate(pointer p, size_type)
   {  ::operator delete(static_cast<void *>(p));  }
};

//Nested type detection, so that no std facility (and no <memory>) is needed
template<class A>
struct bucket_alloc_has_size_type
{
   template<class X> static char test(int, typename X::size_type*);
   template<class X> static int  test(...);
   static const bool value = (1 == sizeof(test<A>(0, 0)));
};

template<class A> struct bucket_alloc_nested_size_type
{  typedef typename A::size_type                type;  };

struct bucket_alloc_std_size_type
{  typedef std::size_t                          type;  };

//The size type of an allocator, with the default that
//std::allocator_traits would supply
template<class Allocator>
struct bucket_alloc_size_type
   :  eval_if_c< bucket_alloc_has_size_type<Allocator>::value
               , bucket_alloc_nested_size_type<Allocator>
               , bucket_alloc_std_size_type >
{};

//Selects the allocator that the manager holds: `void` means the internal
//operator new allocator, and any other type is used as it is, because it
//must already be an allocator of T
template<class Allocator, class T>
struct bucket_alloc_select
{  typedef Allocator                            type;  };

template<class T>
struct bucket_alloc_select<void, T>
{  typedef operator_new_allocator<T>            type;  };

//Members and operations of the bucket manager that need neither the
//container nor the allocator. It only depends on the parameters that
//change its behaviour:
//
//  - Power2Buckets: bucket counts are powers of two, otherwise they are
//    obtained from the prime table (also used by fastmod_buckets).
//  - BucketOverhead: additional buckets that the container needs after the
//    usable ones (the sentinel bucket of linear_buckets). bucket_count()
//    does not include them.
template<class BucketPtr, class SizeType, bool Power2Buckets, std::size_t BucketOverhead>
class bucket_array_base
{
   public:
   typedef BucketPtr                                                 bucket_ptr;
   typedef typename pointer_traits<BucketPtr>::element_type          bucket_type;
   typedef SizeType                                                  size_type;

   static const size_type bucket_overhead = size_type(BucketOverhead);

   bucket_ptr buckets() const BOOST_NOEXCEPT
   {  return m_buckets;  }

   size_type bucket_count() const BOOST_NOEXCEPT
   {  return m_bucket_count;  }

   float max_load_factor() const BOOST_NOEXCEPT
   {  return m_max_load_factor;  }

   void max_load_factor(float mlf)
   {
      BOOST_ASSERT(mlf > 0.0f);
      if(mlf > 0.0f){
         m_max_load_factor = mlf;
         m_max_elements = this->priv_max_elements(m_bucket_count);
      }
   }

   float load_factor(size_type element_count) const BOOST_NOEXCEPT
   {  return float(element_count) / float(m_bucket_count);  }

   //Returns true and the new bucket count if the array is too small to hold
   //element_capacity elements without exceeding the maximum load factor.
   //This is the hot path of the manager, called before every insertion,
   //so the common case is a single integer comparison.
   bool grow_count(size_type element_capacity, size_type &new_count) const
   {
      if(element_capacity <= m_max_elements)
         return false;
      const size_type target = this->priv_buckets_for(element_capacity);
      if(target <= m_bucket_count)
         return false;
      new_count = suggested_count(target);
      return new_count > m_bucket_count;
   }

   //Returns the bucket count for element_count elements and a minimum of
   //bucket_count_hint buckets.
   size_type rehash_count(size_type element_count, size_type bucket_count_hint) const
   {
      size_type target = this->priv_buckets_for(element_count);
      if(bucket_count_hint > target)
         target = bucket_count_hint;
      return suggested_count(target);
   }

   //Rounds `n` up to a bucket count the container accepts, limited
   //to max_count().
   static size_type suggested_count(size_type n)
   {
      return priv_suggested_count(n ? n : size_type(1u), bool_<Power2Buckets>());
   }

   protected:
   bucket_array_base()
      :  m_buckets()
      ,  m_bucket_count(0u)
      ,  m_max_load_factor(1.0f)
      ,  m_max_elements(0u)
   {}

   //Used to implement move operations: the source must be reset afterwards
   //with set_buckets(bucket_ptr(), 0u)
   bucket_array_base(const bucket_array_base &x)
      :  m_buckets(x.m_buckets)
      ,  m_bucket_count(x.m_bucket_count)
      ,  m_max_load_factor(x.m_max_load_factor)
      ,  m_max_elements(x.m_max_elements)
   {}

   bucket_array_base &operator=(const bucket_array_base &x)
   {
      m_buckets         = x.m_buckets;
      m_bucket_count    = x.m_bucket_count;
      m_max_load_factor = x.m_max_load_factor;
      m_max_elements    = x.m_max_elements;
      return *this;
   }

   void swap(bucket_array_base &x)
   {
      ::boost::adl_move_swap(m_buckets,         x.m_buckets);
      ::boost::adl_move_swap(m_bucket_count,    x.m_bucket_count);
      ::boost::adl_move_swap(m_max_load_factor, x.m_max_load_factor);
      ::boost::adl_move_swap(m_max_elements,    x.m_max_elements);
   }

   void set_buckets(bucket_ptr p, size_type n)
   {
      m_buckets      = p;
      m_bucket_count = n;
      m_max_elements = this->priv_max_elements(n);
   }

   private:
   //Maximum number of usable buckets: the array, including the overhead
   //buckets, must be representable by size_type and its size in bytes
   //can't exceed the maximum object size (PTRDIFF_MAX).
   static size_type priv_max_count()
   {
      const std::size_t max_alloc = (std::size_t(-1) >> 1u) / sizeof(bucket_type);
      const std::size_t max_size  = std::size_t(size_type(-1));
      return size_type((max_alloc < max_size ? max_alloc : max_size) - BucketOverhead);
   }

   static size_type priv_suggested_count(size_type n, bool_<true>)
   {
      const size_type max_count = priv_max_count();
      size_type p = 1u;
      while(p < n && p <= size_type(max_count >> 1u)){
         p = size_type(p << 1u);
      }
      return p;
   }

   static size_type priv_suggested_count(size_type n, bool_<false>)
   {
      const size_type max_count = priv_max_count();
      const size_type r = prime_list_holder<>::suggested_upper_bucket_count(n);
      return r > max_count ? max_count : r;
   }

   //Smallest bucket count `b` such that element_count <= max_load_factor*b.
   //The result is limited to priv_max_count(). Calculations use double,
   //as float can't represent all integers greater than 2^24.
   size_type priv_buckets_for(size_type element_count) const
   {
      const size_type max_count = priv_max_count();
      const double d = double(element_count) / double(m_max_load_factor);
      if(d >= double(max_count))
         return max_count;
      size_type b = size_type(d);
      if(double(b) < d)
         ++b;
      return b;
   }

   //Maximum number of elements that `n` buckets hold without exceeding
   //the maximum load factor, limited to size_type's maximum value.
   size_type priv_max_elements(size_type n) const
   {
      const double d = double(n) * double(m_max_load_factor);
      return d >= double(size_type(-1)) ? size_type(-1) : size_type(d);
   }

   bucket_ptr        m_buckets;
   size_type         m_bucket_count;
   float             m_max_load_factor;
   size_type         m_max_elements;   //Cached floor(m_bucket_count*m_max_load_factor)
};

//Adds the allocator to bucket_array_base: owns the bucket array, allocating
//and deallocating it. The allocator is stored as an empty base when it has
//no state.
template<class BucketPtr, class SizeType, class Allocator, bool Power2Buckets, std::size_t BucketOverhead>
class BOOST_INTRUSIVE_EMPTY_BASES bucket_array_manager
   :  public  bucket_array_base<BucketPtr, SizeType, Power2Buckets, BucketOverhead>
   ,  private ebo_functor_holder<Allocator>
{
   BOOST_MOVABLE_BUT_NOT_COPYABLE(bucket_array_manager)

   typedef ebo_functor_holder<Allocator>                                   alloc_holder_t;
   typedef bucket_array_base<BucketPtr, SizeType, Power2Buckets, BucketOverhead> base_t;
   typedef typename bucket_alloc_size_type<Allocator>::type                alloc_size_type;

   public:
   typedef typename base_t::bucket_ptr                                     bucket_ptr;
   typedef typename base_t::bucket_type                                    bucket_type;
   typedef typename base_t::size_type                                      size_type;
   typedef Allocator                                                       allocator_type;

   //The allocator allocates the container's buckets
   BOOST_INTRUSIVE_STATIC_ASSERT
      ((is_same<typename allocator_type::value_type, bucket_type>::value));

   bucket_array_manager(size_type bucket_count_hint, const allocator_type &a)
      :  base_t(), alloc_holder_t(a)
   {
      const size_type n = base_t::suggested_count(bucket_count_hint);
      this->set_buckets(this->create_buckets(n), n);
   }

   bucket_array_manager(BOOST_RV_REF(bucket_array_manager) x)
      :  base_t(static_cast<const base_t &>(x))
      ,  alloc_holder_t(static_cast<const alloc_holder_t &>(x))
   {
      x.set_buckets(bucket_ptr(), 0u);
   }

   bucket_array_manager & operator=(BOOST_RV_REF(bucket_array_manager) x)
   {
      bucket_array_manager &mx = x;
      if(this != &mx){
         this->destroy_buckets(this->buckets(), this->bucket_count());
         this->priv_alloc() = mx.priv_alloc();
         base_t::operator=(static_cast<const base_t &>(mx));
         mx.set_buckets(bucket_ptr(), 0u);
      }
      return *this;
   }

   ~bucket_array_manager()
   {  this->destroy_buckets(this->buckets(), this->bucket_count());  }

   void swap(bucket_array_manager &x)
   {
      ::boost::adl_move_swap(this->priv_alloc(), x.priv_alloc());
      base_t::swap(x);
   }

   allocator_type get_allocator() const
   {  return allocator_type(this->priv_alloc());  }

   //Allocates and default-constructs `n` usable buckets plus the overhead buckets.
   //bucket_type's default constructor is a no-throw operation.
   bucket_ptr create_buckets(size_type n)
   {
      const size_type len = size_type(n + BucketOverhead);
      bucket_ptr p = this->priv_alloc().allocate(alloc_size_type(len));
      bucket_type *raw = ::boost::movelib::to_raw_pointer(p);
      for(size_type i = 0; i != len; ++i){
         ::new(static_cast<void*>(raw + i)) bucket_type();
      }
      return p;
   }

   //Destroys a bucket array obtained from create_buckets. Buckets must be empty.
   void destroy_buckets(bucket_ptr p, size_type n)
   {
      if(p){
         const size_type len = size_type(n + BucketOverhead);
         bucket_type *raw = ::boost::movelib::to_raw_pointer(p);
         for(size_type i = len; i-- != 0u; ){
            (raw + i)->~bucket_type();
         }
         this->priv_alloc().deallocate(p, alloc_size_type(len));
      }
   }

   //Destroys the owned (now empty) array and takes ownership of `p`
   void replace_buckets(bucket_ptr p, size_type n)
   {
      this->destroy_buckets(this->buckets(), this->bucket_count());
      this->set_buckets(p, n);
   }

   private:
   BOOST_INTRUSIVE_FORCEINLINE allocator_type &         priv_alloc()
   {  return alloc_holder_t::get();  }

   BOOST_INTRUSIVE_FORCEINLINE const allocator_type &   priv_alloc() const
   {  return alloc_holder_t::get();  }
};

}  //namespace detail

//! unordered_bucket_manager is a utility that owns and manages the dynamic
//! bucket array required by Boost.Intrusive unordered associative containers
//! (unordered_set, unordered_multiset and hashtable). It implements operations
//! that require modifying the bucket array: increasing the bucket array
//! to maintain the load factor before an insertion, shrink_to_fit, reserve,
//! full-rehashing, etc.
//!
//! The memory for the bucket array is obtained from an allocator held by the manager.
//!
//! This class does not allocates the nodes (values) inserted in the container,
//! these are created and managed by the user of the semi-intrusive container.
//!
//!
//! <b>Template parameters</b>:
//!  - \c Hashtable: the Boost.Intrusive unordered container type (an
//!    instantiation of unordered_set, unordered_multiset or hashtable) that
//!    must use its default bucket traits, i.e. the \c bucket_traits option
//!    must not be customized.
//!  - \c Allocator: \c void (the default) means that the bucket array is
//!    taken from the global <tt>operator new</tt>.
//!    Any other type is used as the allocator of the bucket array, and it
//!    shall be an allocator of buckets: <tt>Allocator::value_type</tt> shall
//!    be <tt>Hashtable::bucket_type</tt> and <tt>Allocator::pointer</tt>
//!    must be convertible to <tt>Hashtable::bucket_ptr</tt>.
//!
//! <b>Usage rules</b>:
//!  - One manager manages the buckets of exactly one container. Declare the
//!    manager <b>before</b> the container (so that it's constructed first and
//!    destroyed last) and construct the container from <tt>traits()</tt>.
//!  - Insertions are done on the container. Before each one, call
//!    <tt>reserve_additional(c, n)</tt> (or <tt>reserve(c, capacity)</tt>)
//!    so the load factor invariant is maintained. For a batch of \c n
//!    elements, one <tt>reserve_additional(c, n)</tt> call grows the array
//!    at most once; a call per element grows it step by step.
//!  - The manager holds only the operations that need the bucket array or
//!    the load factor. Every other operation is done on the container
//!    itself: insertions (insert, insert_check/insert_commit), lookups
//!    (find, count, equal_range, iteration...) and erasures (erase,
//!    erase_and_dispose, clear, clear_and_dispose).
//!  - Erasures never touch the bucket array, as in the standard unordered
//!    containers: bucket_count() is unchanged (even when the container
//!    becomes empty), erasing invalidates only iterators and references to
//!    the erased elements, and a single erasure stays O(1) on average. To
//!    release buckets, call shrink_to_fit() (or rehash()) explicitly when
//!    convenient.
//!  - Any operation that reports a rehash (or may perform one) invalidates
//!    all iterators into the container. Pointers and references to the
//!    elements are never invalidated (nodes are relinked, not moved).
//!  - The container must be empty (or already destroyed) when the manager
//!    releases a bucket array it still uses, i.e. at manager destruction,
//!    move-assignment over it, or swap with an unrelated manager.
//!
//! <b>Example</b> (values owned by the caller, as usual in Boost.Intrusive):
//! \code
//! typedef boost::intrusive::unordered_set<MyType>              Uset;
//! typedef boost::intrusive::unordered_bucket_manager<Uset>     Manager;
//!
//! Manager mgr;                    //allocates the initial bucket array
//! Uset    set(mgr.traits());      //container uses the managed buckets
//!
//! MyType *p = new MyType(...);
//! mgr.reserve_additional(set);         //room for one more element
//! set.insert(*p);                      //insertions: on the container
//! Uset::iterator it = set.find(key);   //lookups go straight through
//! set.erase(key);                      //erasures too: no bucket work
//! mgr.shrink_to_fit(set);              //the only way to lose buckets
//! set.clear_and_dispose(Deleter());    //leave the buckets empty
//! \endcode
template < class Hashtable
         , class Allocator = void >
class unordered_bucket_manager
   #ifndef BOOST_INTRUSIVE_DOXYGEN_INVOKED
   //The implementation only depends on the parameters that change the
   //behaviour, not on the whole container type
   :  private detail::bucket_array_manager
         < typename Hashtable::bucket_ptr
         , typename Hashtable::size_type
         , typename detail::bucket_alloc_select
              <Allocator, typename Hashtable::bucket_type>::type
         , Hashtable::power_2_buckets
         , Hashtable::bucket_overhead
         >
   #endif   //BOOST_INTRUSIVE_DOXYGEN_INVOKED
{
   //Movable-only: the bucket array has a single owner
   BOOST_MOVABLE_BUT_NOT_COPYABLE(unordered_bucket_manager)

   typedef detail::bucket_array_manager
      < typename Hashtable::bucket_ptr
      , typename Hashtable::size_type
      , typename detail::bucket_alloc_select
           <Allocator, typename Hashtable::bucket_type>::type
      , Hashtable::power_2_buckets
      , Hashtable::bucket_overhead
      >                                                  base_t;

   public:
   typedef Hashtable                                     hashtable_type;
   typedef typename hashtable_type::bucket_type          bucket_type;
   typedef typename hashtable_type::bucket_traits        bucket_traits_type;
   typedef typename hashtable_type::size_type            size_type;
   //With \c void, an internal allocator using operator new, Allocator otherwise
   typedef BOOST_INTRUSIVE_IMPDEF(typename base_t::allocator_type) allocator_type;

   #ifndef BOOST_INTRUSIVE_DOXYGEN_INVOKED
   private:
   //The allocator allocates the container's buckets
   BOOST_INTRUSIVE_STATIC_ASSERT
      ((detail::is_same< typename allocator_type::value_type
                       , bucket_type>::value));

   typedef typename hashtable_type::bucket_ptr           bucket_ptr;
   #endif   //BOOST_INTRUSIVE_DOXYGEN_INVOKED

   public:

   //////////////////////////////////////////////
   //
   //  Construction, destruction, assignment
   //
   //////////////////////////////////////////////

   //! <b>Effects</b>: Constructs the manager, allocating an initial bucket
   //!   array of at least \c bucket_count_hint buckets (rounded up to a
   //!   count the container accepts; when zero, the smallest such count is
   //!   used). A copy of \c a is stored and used for every bucket
   //!   allocation.
   //!
   //! <b>Throws</b>: If the allocator throws.
   explicit unordered_bucket_manager
      ( size_type bucket_count_hint = 0u
      , const allocator_type &a = allocator_type())
      :  base_t(bucket_count_hint, a)
   {}

   //! <b>Effects</b>: Move constructor. Ownership of the bucket array is
   //!   transferred; the container associated with \c x (if any) keeps
   //!   working and becomes associated with *this. The moved-from manager
   //!   owns no buckets and shall only be destroyed, assigned to or swapped.
   //!
   //! <b>Throws</b>: If the allocator's copy constructor throws.
   unordered_bucket_manager(BOOST_RV_REF(unordered_bucket_manager) x)
      :  base_t(::boost::move(static_cast<base_t&>(x)))
   {}

   //! <b>Requires</b>: No container is using the bucket array currently
   //!   owned by *this (buckets must be empty).
   //!
   //! <b>Effects</b>: Destroys the owned array and takes ownership of x's
   //!   array (see the move constructor).
   //!
   //! <b>Throws</b>: If the allocator's copy assignment throws.
   unordered_bucket_manager & operator=(BOOST_RV_REF(unordered_bucket_manager) x)
   {
      base_t::operator=(::boost::move(static_cast<base_t&>(x)));
      return *this;
   }

   #if defined(BOOST_INTRUSIVE_DOXYGEN_INVOKED)
   //! <b>Requires</b>: No container is using the owned bucket array anymore:
   //!   the associated container has been destroyed, or cleared (e.g. via
   //!   clear_and_dispose), before the manager is destroyed.
   //!
   //! <b>Effects</b>: Destroys the buckets and deallocates the array.
   ~unordered_bucket_manager();
   #endif

   //! <b>Effects</b>: Swaps ownership of the bucket arrays, the allocators,
   //!   and the maximum load factors. The containers associated with each
   //!   manager become associated with the other one.
   //!
   //! <b>Throws</b>: Nothing (assuming the allocator's swap doesn't throw).
   void swap(unordered_bucket_manager &x)
   {  base_t::swap(x);  }

   friend void swap(unordered_bucket_manager &l, unordered_bucket_manager &r) BOOST_NOEXCEPT
   {  l.swap(r);  }

   //////////////////////////////////////////////
   //
   //  Observers
   //
   //////////////////////////////////////////////

   #if defined(BOOST_INTRUSIVE_DOXYGEN_INVOKED)
   //! <b>Effects</b>: Returns the number of usable buckets of the owned array,
   //!   that is, the bucket count of the associated container. With
   //!   linear_buckets, the array holds an additional sentinel bucket.
   //!
   //! <b>Throws</b>: Nothing.
   size_type bucket_count() const BOOST_NOEXCEPT;
   #else
   using base_t::bucket_count;
   #endif

   //! <b>Effects</b>: Returns a value-semantics bucket traits object
   //!   (of the container's own bucket traits type) describing the currently
   //!   owned array, suitable to construct the associated container.
   //!
   //! <b>Throws</b>: Nothing.
   bucket_traits_type traits() const BOOST_NOEXCEPT
   {
      return bucket_traits_type
         (this->buckets(), size_type(base_t::bucket_count() + base_t::bucket_overhead));
   }

   #if defined(BOOST_INTRUSIVE_DOXYGEN_INVOKED)
   //! <b>Effects</b>: Returns a copy of the stored allocator, converted back
   //!   to the original \c Allocator type.
   allocator_type get_allocator() const;

   //! <b>Effects</b>: Returns the current maximum load factor.
   float max_load_factor() const BOOST_NOEXCEPT;

   //! <b>Requires</b>: mlf > 0.0f
   //!
   //! <b>Effects</b>: Sets the maximum load factor. Does not rehash: the
   //!   bucket array is not resized immediately, and the new factor is
   //!   applied by the next operation that changes the bucket count
   //!   (reserve(), reserve_additional(), rehash(), shrink_to_fit()).
   void max_load_factor(float mlf);
   #else
   using base_t::get_allocator;
   using base_t::max_load_factor;
   #endif

   //! <b>Effects</b>: Returns the current load factor of the container.
   float load_factor(const hashtable_type &c) const BOOST_NOEXCEPT
   {  return base_t::load_factor(c.size());  }

   //////////////////////////////////////////////
   //
   //  Capacity handling (load factor / rehash)
   //
   //////////////////////////////////////////////

   //! <b>Requires</b>: \c c was constructed with this manager's buckets.
   //!
   //! <b>Effects</b>: If the owned array is too small to hold
   //!   \c element_capacity elements without exceeding the maximum load
   //!   factor, allocates a bigger array and <b>rehashes \c c into it</b>
   //!   (the array is never shrunk). Does nothing when the array is already
   //!   big enough. Call it
   //!   before inserting directly into the container: to make room for
   //!   \c n more elements, ask for <tt>c.size() + n</tt>.
   //!
   //! <b>Returns</b>: true if a rehash took place (all iterators into \c c
   //!   are invalidated; pointers and references remain valid).
   //!
   //! <b>Throws</b>: If the allocator throws, or if the container's hasher
   //!   throws during rehashing (see rehash() notes below).
   bool reserve(hashtable_type &c, size_type element_capacity)
   {
      BOOST_ASSERT(c.bucket_pointer() == this->buckets());
      size_type new_count;
      if(!this->grow_count(element_capacity, new_count))
         return false;
      this->priv_do_rehash(c, new_count);
      return true;
   }

   //! <b>Requires</b>: \c c was constructed with this manager's buckets.
   //!
   //! <b>Effects</b>: Equivalent to
   //!   <tt>reserve(c, c.size() + extra_elements)</tt>: if the owned array
   //!   is too small for \c extra_elements more elements at the maximum
   //!   load factor, allocates a bigger array and <b>rehashes \c c into
   //!   it</b>. Call it before inserting into the container.
   //!
   //! <b>Returns</b>: true if a rehash took place (all iterators into \c c
   //!   are invalidated; pointers and references remain valid).
   //!
   //! <b>Throws</b>: If the allocator throws, or if the container's hasher
   //!   throws during rehashing (see rehash() notes below).
   bool reserve_additional(hashtable_type &c, size_type extra_elements = 1u)
   {  return this->reserve(c, size_type(c.size() + extra_elements));  }

   //! <b>Effects</b>: Sets the bucket count to the suggested count nearest
   //!   to <tt>max(bucket_count_hint, c.size()/max_load_factor())</tt>,
   //!   growing or shrinking as needed. When that count differs from the
   //!   current one, allocates a new array and <b>rehashes \c c into it</b>;
   //!    otherwise does nothing.
   //!
   //! <b>Returns</b>: true if a rehash took place (all iterators into \c c
   //!   are invalidated; pointers and references remain valid).
   //!
   //! <b>Throws</b>: If the allocator throws, or if the container's hasher
   //!   throws during rehashing.
   //!
   //! <b>Note</b>: If the container's hasher throws while relinking,
   //!   Boost.Intrusive restores every element into the previous bucket
   //!   array, this manager deallocates the new (empty) one and the
   //!   exception is propagated, so the strong guarantee is provided as long
   //!   as the hasher computes equal values for equal keys.
   bool rehash(hashtable_type &c, size_type bucket_count_hint)
   {
      const size_type new_count = this->rehash_count(c.size(), bucket_count_hint);
      if(new_count == base_t::bucket_count())
         return false;
      this->priv_do_rehash(c, new_count);
      return true;
   }

   //! <b>Effects</b>: Shrinks the bucket array to the smallest suggested
   //!   count that respects the maximum load factor for the current size,
   //!   which allocates that smaller array and <b>rehashes \c c into
   //!   it</b>. Erasures never do this implicitly (see the class
   //!   documentation), so this is the operation to call when releasing
   //!   buckets is desired.
   //!
   //! <b>Returns</b>: true if a rehash took place (all iterators into \c c
   //!   are invalidated; pointers and references remain valid).
   //!
   //! <b>Throws</b>: If the allocator throws, or if the container's hasher
   //!   throws during rehashing.
   bool shrink_to_fit(hashtable_type &c)
   {  return this->rehash(c, 0u);  }

   #ifndef BOOST_INTRUSIVE_DOXYGEN_INVOKED
   private:

   //Allocates a new array, rehashes the container into it and releases the
   //old (now empty) array. Provides the strong guarantee: if the hasher
   //throws, Boost.Intrusive's internal rollback relinks every already
   //transferred node back into the old buckets, so the new array is empty
   //again and can be destroyed before rethrowing.
   void priv_do_rehash(hashtable_type &c, size_type new_count)
   {
      BOOST_ASSERT(new_count != 0u);
      const bucket_ptr nb = this->create_buckets(new_count);
      BOOST_INTRUSIVE_TRY{
         c.rehash(bucket_traits_type(nb, size_type(new_count + base_t::bucket_overhead)));
      }
      BOOST_INTRUSIVE_CATCH(...){
         this->destroy_buckets(nb, new_count);
         BOOST_INTRUSIVE_RETHROW;
      }
      BOOST_INTRUSIVE_CATCH_END
      //The old buckets are empty now, all nodes were relinked
      this->replace_buckets(nb, new_count);
   }
   #endif   //BOOST_INTRUSIVE_DOXYGEN_INVOKED
};

}  //namespace intrusive
}  //namespace boost

#include <boost/intrusive/detail/config_end.hpp>

#endif   //BOOST_INTRUSIVE_UNORDERED_BUCKET_MANAGER_HPP
