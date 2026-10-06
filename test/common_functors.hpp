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
