/////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Ion Gaztanaga  2026-2026.
//
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/intrusive for documentation.
//
/////////////////////////////////////////////////////////////////////////////

//Tests the move assignment when BOOST_INTRUSIVE_MOVE_ASSIGNMENT_SWAPS is defined:
//it swaps the containers, as before Boost 1.93
#define BOOST_INTRUSIVE_MOVE_ASSIGNMENT_SWAPS

#include <boost/intrusive/list.hpp>
#include <boost/intrusive/slist.hpp>
#include <boost/intrusive/set.hpp>
#include <boost/intrusive/avl_set.hpp>
#include <boost/intrusive/bs_set.hpp>
#include <boost/intrusive/sg_set.hpp>
#include <boost/intrusive/splay_set.hpp>
#include <boost/intrusive/treap_set.hpp>
#include <boost/intrusive/unordered_set.hpp>
#include <boost/move/utility_core.hpp>
#include <boost/move/detail/type_traits.hpp>
#include <boost/core/lightweight_test.hpp>
#include <cstddef>

using namespace boost::intrusive;

class value
   : public list_base_hook<>
   , public slist_base_hook<>
   , public set_base_hook<>
   , public avl_set_base_hook<>
   , public bs_set_base_hook<>
   , public unordered_set_base_hook<>
{
   public:
   explicit value(int i = 0) : v(i) {}
   int v;

   friend bool operator==(const value &a, const value &b)
   {  return a.v == b.v;  }

   friend bool priority_order(const value &a, const value &b)
   {  return a.v < b.v;  }
};

//Predicates with a tag, to check where they go
struct tagged_less
{
   explicit tagged_less(int t = 0) : tag(t) {}
   bool operator()(const value &a, const value &b) const
   {  return a.v < b.v;  }
   int tag;
};

struct tagged_hash
{
   explicit tagged_hash(int t = 0) : tag(t) {}
   std::size_t operator()(const value &a) const
   {  return std::size_t(a.v);  }
   int tag;
};

struct tagged_equal
{
   explicit tagged_equal(int t = 0) : tag(t) {}
   bool operator()(const value &a, const value &b) const
   {  return a.v == b.v;  }
   int tag;
};

static const std::size_t NumValues = 4u;

//The move assignment is not noexcept, as it can call the swap of the predicates
template<class Cont>
void test_not_noexcept()
{
   #if !defined(BOOST_NO_CXX11_NOEXCEPT)
   using boost::move_detail::declval;
   BOOST_INTRUSIVE_STATIC_ASSERT(( !noexcept(declval<Cont&>() = declval<Cont>()) ));
   #endif
}

//Checks that c contains exactly the values of vals
template<class Cont>
bool contains(const Cont &c, value (&vals)[NumValues])
{
   if(c.size() != NumValues)
      return false;
   for(std::size_t i = 0; i != NumValues; ++i){
      bool found = false;
      for(typename Cont::const_iterator it = c.begin(), itend = c.end(); it != itend; ++it){
         if(&*it == &vals[i])
            found = true;
      }
      if(!found)
         return false;
   }
   return true;
}

template<class Seq>
void test_sequence()
{
   value va[NumValues] = { value(0), value(1), value(2), value(3) };
   value vb[NumValues] = { value(4), value(5), value(6), value(7) };
   test_not_noexcept<Seq>();
   Seq a, b;
   for(std::size_t i = 0; i != NumValues; ++i){
      a.push_front(va[i]);
      b.push_front(vb[i]);
   }
   //The containers are swapped: a keeps the nodes of b and b the nodes of a
   a = boost::move(b);
   BOOST_TEST(contains(a, vb));
   BOOST_TEST(contains(b, va));
   a.clear();
   b.clear();
}

template<class Assoc>
void test_associative()
{
   value va[NumValues] = { value(0), value(1), value(2), value(3) };
   value vb[NumValues] = { value(4), value(5), value(6), value(7) };
   test_not_noexcept<Assoc>();
   Assoc a((tagged_less(1))), b((tagged_less(2)));
   a.insert(va, va + NumValues);
   b.insert(vb, vb + NumValues);
   //The containers are swapped, with their comparison objects
   a = boost::move(b);
   BOOST_TEST(contains(a, vb));
   BOOST_TEST(contains(b, va));
   BOOST_TEST(a.key_comp().tag == 2);
   BOOST_TEST(b.key_comp().tag == 1);
   a.clear();
   b.clear();
}

void test_unordered()
{
   typedef unordered_set<value, hash<tagged_hash>, equal<tagged_equal> > unordered_t;
   typedef unordered_t::bucket_type   bucket_type;
   typedef unordered_t::bucket_traits bucket_traits;
   test_not_noexcept<unordered_t>();
   bucket_type buckets_a[8], buckets_b[8];
   value va[NumValues] = { value(0), value(1), value(2), value(3) };
   value vb[NumValues] = { value(4), value(5), value(6), value(7) };
   unordered_t a(bucket_traits(buckets_a, 8), tagged_hash(1), tagged_equal(1));
   unordered_t b(bucket_traits(buckets_b, 8), tagged_hash(2), tagged_equal(2));
   a.insert(va, va + NumValues);
   b.insert(vb, vb + NumValues);
   //The containers are swapped, with their predicates and bucket arrays
   a = boost::move(b);
   BOOST_TEST(contains(a, vb));
   BOOST_TEST(contains(b, va));
   BOOST_TEST(a.hash_function().tag == 2);
   BOOST_TEST(a.key_eq().tag == 2);
   BOOST_TEST(b.hash_function().tag == 1);
   BOOST_TEST(b.key_eq().tag == 1);
   BOOST_TEST(a.bucket_pointer() == buckets_b);
   BOOST_TEST(b.bucket_pointer() == buckets_a);
   a.clear();
   b.clear();
}

int main()
{
   test_sequence< list<value> >();
   test_sequence< slist<value> >();
   test_associative< set<value, compare<tagged_less> > >();
   test_associative< avl_set<value, compare<tagged_less> > >();
   test_associative< bs_set<value, compare<tagged_less> > >();
   test_associative< sg_set<value, compare<tagged_less> > >();
   test_associative< splay_set<value, compare<tagged_less> > >();
   test_associative< treap_set<value, compare<tagged_less> > >();
   test_associative< multiset<value, compare<tagged_less> > >();
   test_associative< avl_multiset<value, compare<tagged_less> > >();
   test_associative< bs_multiset<value, compare<tagged_less> > >();
   test_associative< sg_multiset<value, compare<tagged_less> > >();
   test_associative< splay_multiset<value, compare<tagged_less> > >();
   test_associative< treap_multiset<value, compare<tagged_less> > >();
   test_unordered();
   return boost::report_errors();
}
