/////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Ion Gaztanaga  2006-2013
//
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/intrusive for documentation.
//
/////////////////////////////////////////////////////////////////////////////

#ifndef BOOST_INTRUSIVE_TEST_COMMON_FUNCTORS_HPP
#define BOOST_INTRUSIVE_TEST_COMMON_FUNCTORS_HPP

#include<boost/intrusive/detail/iterator.hpp>
#include<boost/intrusive/detail/mpl.hpp>
#include<boost/static_assert.hpp>
#include<boost/move/detail/to_raw_pointer.hpp>
#include<boost/move/utility_core.hpp>
#include<boost/move/core.hpp>
#include<boost/intrusive/priority_compare.hpp>
#include <cstddef>

namespace boost      {
namespace intrusive  {
namespace test       {

template<class T>
class delete_disposer
{
   public:
   template <class Pointer>
      void operator()(Pointer p)
   {
      typedef typename boost::intrusive::pointer_traits<Pointer>::element_type value_type;
      BOOST_INTRUSIVE_STATIC_ASSERT(( detail::is_same<T, value_type>::value ));
      delete boost::movelib::to_raw_pointer(p);
   }
};

template<class T>
class delete_noexcept_disposer
{
   public:
   template <class Pointer>
      void operator()(Pointer p) BOOST_NOEXCEPT
   {
      typedef typename boost::intrusive::pointer_traits<Pointer>::element_type value_type;
      BOOST_INTRUSIVE_STATIC_ASSERT(( detail::is_same<T, value_type>::value ));
      delete boost::movelib::to_raw_pointer(p);
   }
};

template<class T>
class new_cloner
{
   public:
      T *operator()(const T &t)
   {  return new T(t);  }
};

template<class T>
class new_nonconst_cloner
{
   public:
      T *operator()(T &t)
   {  return new T(t);  }
};

template<class T>
class new_default_factory
{
   public:
      T *operator()()
   {  return new T();  }
};

class empty_disposer
{
   public:
   template<class T>
   void operator()(const T &)
   {}
};

class empty_noexcept_disposer
{
   public:
   template<class T>
   void operator()(const T &) BOOST_NOEXCEPT
   {}
};

struct any_less
{
   template<class T, class U>
   bool operator()(const T &t, const U &u) const
   {  return t < u;  }
};

//Like any_less, but counts the comparisons in *count_
struct counting_less
{
   std::size_t *count_;

   template<class T, class U>
   bool operator()(const T &t, const U &u) const
   {  ++*count_; return t < u;  }
};

struct any_equal
{
   template<class T, class U>
   bool operator()(const T &t, const U &u) const
   {  return t == u;  }
};

//Wraps a predicate and only offers a non-const operator()
//to test that containers support such predicates
template<class Pred>
struct nonconst_pred
{
   Pred pred_;

   template<class T>
   bool operator()(const T &t)
   {  return pred_(t);  }

   template<class T, class U>
   bool operator()(const T &t, const U &u)
   {  return pred_(t, u);  }
};

template<class Pred>
nonconst_pred<Pred> make_nonconst_pred(const Pred &p)
{
   nonconst_pred<Pred> r = { p };
   return r;
}

struct any_greater
{
   template<class T, class U>
   bool operator()(const T &t, const U &u) const
   {  return t > u;  }
};

//Comparison functor with a state
template<class Key>
struct tagged_less
{
   int tag_;
   explicit tagged_less(int tag = 0) : tag_(tag) {}
   bool operator()(const Key &a, const Key &b) const
   {  return a < b;  }
};

//Hash function and equality predicate with a state
struct instance_seed_hash
{
   std::size_t seed_;
   explicit instance_seed_hash(std::size_t seed = 0u) : seed_(seed) {}

   template<class T>
   std::size_t operator()(const T &t) const
   {  return hash_value(t) ^ seed_;  }
};

struct tagged_equal
{
   int tag_;
   explicit tagged_equal(int tag = 0) : tag_(tag) {}

   template<class T, class U>
   bool operator()(const T &t, const U &u) const
   {  return t == u;  }
};

//Counts the copies, moves and swaps of the tracking_xxx predicates below
template<class Dummy = void>
struct predicate_counters_t
{
   static int copies;
   static int moves;
   static int swaps;

   static void reset()
   {  copies = moves = swaps = 0;  }
};

template<class Dummy> int predicate_counters_t<Dummy>::copies = 0;
template<class Dummy> int predicate_counters_t<Dummy>::moves  = 0;
template<class Dummy> int predicate_counters_t<Dummy>::swaps  = 0;

typedef predicate_counters_t<> predicate_counters;

//The state of a predicate that counts its copies, moves and swaps. A moved-from
//predicate has tag -1, so a container that kept a moved-from predicate is
//detected.
template<class Derived>
class tracking_predicate
{
   protected:
   explicit tracking_predicate(int tag) : tag_(tag) {}

   static void copied()
   {  ++predicate_counters::copies;  }

   static void moved(tracking_predicate &from)
   {  from.tag_ = -1; ++predicate_counters::moves;  }

   public:
   friend void swap(Derived &a, Derived &b) BOOST_NOEXCEPT_OR_NOTHROW
   {
      const int tmp = a.tag_; a.tag_ = b.tag_; b.tag_ = tmp;
      ++predicate_counters::swaps;
   }

   int tag_;
};

//The copy and move operations of a tracking predicate. Each predicate declares
//them, not tracking_predicate: with the Boost.Move emulation, a base class with
//move operations gives the derived class a copy assignment that can't take a
//const argument. The copy constructor is not noexcept (it does not throw), so
//the containers that copy it are not noexcept either.
#define BOOST_INTRUSIVE_TEST_TRACKING_PREDICATE(TYPE)\
   BOOST_COPYABLE_AND_MOVABLE(TYPE)\
   typedef tracking_predicate<TYPE> base_t;\
   public:\
   explicit TYPE(int tag = 0) : base_t(tag) {}\
   TYPE(const TYPE &o) BOOST_NOEXCEPT_IF(false) : base_t(o.tag_)\
   {  this->copied();  }\
   TYPE(BOOST_RV_REF(TYPE) o) BOOST_NOEXCEPT_OR_NOTHROW : base_t(o.tag_)\
   {  this->moved(o);  }\
   TYPE &operator=(BOOST_COPY_ASSIGN_REF(TYPE) o)\
   {  this->tag_ = o.tag_; return *this;  }\
   TYPE &operator=(BOOST_RV_REF(TYPE) o) BOOST_NOEXCEPT_OR_NOTHROW\
   {  this->tag_ = o.tag_; this->moved(o); return *this;  }\
//

template<class Key>
class tracking_less
   : public tracking_predicate< tracking_less<Key> >
{
   BOOST_INTRUSIVE_TEST_TRACKING_PREDICATE(tracking_less)

   bool operator()(const Key &a, const Key &b) const
   {  return a < b;  }
};

class tracking_priority
   : public tracking_predicate<tracking_priority>
{
   BOOST_INTRUSIVE_TEST_TRACKING_PREDICATE(tracking_priority)

   template<class T, class U>
   bool operator()(const T &t, const U &u) const
   {  return priority_compare<>()(t, u);  }
};

class tracking_hash
   : public tracking_predicate<tracking_hash>
{
   BOOST_INTRUSIVE_TEST_TRACKING_PREDICATE(tracking_hash)

   template<class T>
   std::size_t operator()(const T &t) const
   {  return hash_value(t);  }
};

class tracking_equal
   : public tracking_predicate<tracking_equal>
{
   BOOST_INTRUSIVE_TEST_TRACKING_PREDICATE(tracking_equal)

   template<class T, class U>
   bool operator()(const T &t, const U &u) const
   {  return t == u;  }
};

//Compares the integer values divided by 2: a comparison coarser than less.
//Several elements with unique keys can be equivalent to a key
struct coarse_less
{
   static int int_of(int i)
   {  return i;  }

   template<class T>
   static int int_of(const T &t)
   {  return t.int_value();  }

   template<class T, class U>
   bool operator()(const T &t, const U &u) const
   {  return int_of(t)/2 < int_of(u)/2;  }
};

}  //namespace test       {
}  //namespace intrusive  {
}  //namespace boost      {

#endif
