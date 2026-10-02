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

#include <boost/config.hpp>

#ifdef BOOST_NO_EXCEPTIONS

int main()
{
   return 0;
}

#else //BOOST_NO_EXCEPTIONS

//Tests that exceptions thrown by user functors propagate to the
//caller and leave the container in a valid state
//
//It also tests that noexcept functions don't call these functors.
//
//Each container family is tested with several options that select
//different code paths.

#include <boost/intrusive/set.hpp>
#include <boost/intrusive/avl_set.hpp>
#include <boost/intrusive/sg_set.hpp>
#include <boost/intrusive/splay_set.hpp>
#include <boost/intrusive/bs_set.hpp>
#include <boost/intrusive/treap_set.hpp>
#include <boost/intrusive/list.hpp>
#include <boost/intrusive/slist.hpp>
#include <boost/intrusive/unordered_set.hpp>
#include <boost/core/lightweight_test.hpp>
#include <cstddef>
#include <iterator>

namespace bi = boost::intrusive;

//Throw control
//
//The containers construct the functors (compare<>, priority<>, hash<>) or call the
//value operators (operator==, operator<), so the functors must use read global flags.
//When a flag is armed, the next functor that reads it throws and disarms the flag.
//
//There are two flags because unordered containers need hasher and equality functor.
//
// - g_pred_throw_active: comparators, priority, predicates, operator==/<
// - g_hash_throw_armed: hashers
//
//Functions taking `bool &armed` argument receive a reference to one of these two
//globals, never other state.
static bool g_pred_throw_active = false;
static bool g_hash_throw_armed = false;

struct test_exception {};

//Called by every throwing functor: `armed` is the global flag that the functor reads
static void maybe_throw(bool &armed)
{
   if(armed){
      armed = false;
      throw test_exception();
   }
}

//Hooks with options that select other code paths
struct os_tag;
struct au_tag;
struct mk_tag;
typedef bi::set_base_hook
   < bi::tag<os_tag>, bi::optimize_size<true> >                                  rb_os_hook;
typedef bi::avl_set_base_hook
   < bi::tag<os_tag>, bi::optimize_size<true> >                                  avl_os_hook;
typedef bi::list_base_hook
   < bi::tag<au_tag>, bi::link_mode<bi::auto_unlink> >                           au_list_hook;
typedef bi::slist_base_hook
   < bi::tag<au_tag>, bi::link_mode<bi::auto_unlink> >                           au_slist_hook;
typedef bi::unordered_set_base_hook
   < bi::tag<mk_tag>, bi::store_hash<true>, bi::optimize_multikey<true> >        mk_hook;

struct value
   : public bi::set_base_hook<>
   , public rb_os_hook
   , public bi::avl_set_base_hook<>
   , public avl_os_hook
   , public bi::bs_set_base_hook<>
   , public bi::list_base_hook<>
   , public au_list_hook
   , public bi::slist_base_hook<>
   , public au_slist_hook
   , public bi::unordered_set_base_hook<>
   , public mk_hook
{
   int v_;

   explicit value(int v = 0) : v_(v) {}

   friend bool operator==(const value &a, const value &b)
   {  maybe_throw(g_pred_throw_active); return a.v_ == b.v_;  }

   friend bool operator<(const value &a, const value &b)
   {  maybe_throw(g_pred_throw_active); return a.v_ < b.v_;  }
};

struct throwing_less
{
   bool operator()(const value &a, const value &b) const
   {  maybe_throw(g_pred_throw_active); return a.v_ < b.v_;  }
};

struct throwing_equal
{
   bool operator()(const value &a, const value &b) const
   {  maybe_throw(g_pred_throw_active); return a.v_ == b.v_;  }
};

struct throwing_is_4
{
   bool operator()(const value &a) const
   {  maybe_throw(g_pred_throw_active); return a.v_ == 4;  }
};

//Elements nearer to 4 have more priority, so that the top of the treap is 4
//and the treap of 1..7 is balanced: erasing the top needs a rotation.
struct throwing_priority
{
   static int distance(const value &a)
   {  return a.v_ < 4 ? 4 - a.v_ : a.v_ - 4;  }

   bool operator()(const value &a, const value &b) const
   {  maybe_throw(g_pred_throw_active); return distance(a) < distance(b);  }
};

struct throwing_hash
{
   std::size_t operator()(const value &a) const
   {  maybe_throw(g_hash_throw_armed); return std::size_t(a.v_);  }
};

template<bool B>
struct bool_tag {};

struct null_disposer
{  void operator()(value *) const {}  };

struct new_cloner
{  value *operator()(const value &v) const {  return new value(v);  }  };

struct delete_disposer
{  void operator()(value *p) const {  delete p;  }  };

static const std::size_t N = 7u;
static const std::size_t N2 = 3u;

//Initializes values to 1..N and values2 to N+1..N+N2
void init_values(value (&values)[N], value (&values2)[N2])
{
   for(std::size_t i = 0; i != N; ++i)
      values[i].v_ = int(i + 1u);
   for(std::size_t i = 0; i != N2; ++i)
      values2[i].v_ = int(N + i + 1u);
}

template<class Cont>
std::size_t iterated_size(Cont &c)
{  return std::size_t(std::distance(c.begin(), c.end()));  }

template<class Cont>
bool contains(Cont &c, const value &v)
{
   for(typename Cont::iterator it = c.begin(), itend = c.end(); it != itend; ++it){
      if(&*it == &v)
         return true;
   }
   return false;
}

//Test helpers
//
//Each tested member is wrapped in a functor object (`f`) so that the same helper can arm
//a flag, call the member, catch the exception and check the container state:
//
// - test_throws:       the member must throw and offer the strong guarantee
// - test_throws_basic: the member must throw and offer the basic guarantee
// - test_no_throw:     the member is noexcept, so it must not call any functor

//Strong guarantee
template<class Cont, class F>
void test_throws(Cont &c, F f, bool &armed)
{
   const std::size_t old_size = c.size();
   bool thrown = false;
   armed = true;
   try{
      f(c);
   }
   catch(test_exception &){
      thrown = true;
   }
   armed = false;
   BOOST_TEST(thrown);
   BOOST_TEST(c.size() == old_size);
   BOOST_TEST(iterated_size(c) == old_size);
}

//Basic guarantee
template<class Cont, class F>
void test_throws_basic
   (Cont &c, Cont &c2, F f, value (&values)[N], value (&values2)[N2])
{
   bool thrown = false;
   g_pred_throw_active = true;
   try{
      f(c, c2);
   }
   catch(test_exception &){
      thrown = true;
   }
   g_pred_throw_active = false;
   BOOST_TEST(thrown);
   BOOST_TEST(c.size()  == iterated_size(c));
   BOOST_TEST(c2.size() == iterated_size(c2));
   c.check();
   c2.check();
   std::size_t found = 0;
   value *const ranges[2][2] = { { &values[0],  &values[0] + N    }
                               , { &values2[0], &values2[0] + N2  } };
   for(std::size_t r = 0; r != 2; ++r){
      for(value *p = ranges[r][0]; p != ranges[r][1]; ++p){
         if(contains(c, *p) || contains(c2, *p)){
            ++found;
         }
         else{
            BOOST_TEST(Cont::node_algorithms::inited(Cont::value_traits::to_node_ptr(*p)));
         }
      }
   }
   BOOST_TEST(found == c.size() + c2.size());
}

template<class Cont, class F>
void test_no_throw(Cont &c, F f)
{
   bool thrown = false;
   g_pred_throw_active = g_hash_throw_armed = true;
   try{
      f(c);
   }
   catch(test_exception &){
      thrown = true;
   }
   g_pred_throw_active = g_hash_throw_armed = false;
   BOOST_TEST(!thrown);
}

//////////////////////////////////////////
//Functors shared by several containers
//////////////////////////////////////////

struct cont_insert
{
   value *v_;
   template<class Cont>
   void operator()(Cont &c) const
   {  c.insert(*v_);  }
};

struct cont_insert_range
{
   value *v_;
   template<class Cont>
   void operator()(Cont &c) const
   {  c.insert(v_, v_ + 1);  }
};

struct cont_insert_check
{
   template<class Cont>
   void operator()(Cont &c) const
   {
      typename Cont::insert_commit_data data;
      c.insert_check(value(4), data);
   }
};

struct cont_find
{
   template<class Cont>
   void operator()(Cont &c) const
   {  c.find(value(4));  }
};

struct cont_find_const
{
   template<class Cont>
   void operator()(const Cont &c) const
   {  c.find(value(4));  }
};

struct cont_count
{
   template<class Cont>
   void operator()(const Cont &c) const
   {  c.count(value(4));  }
};

struct cont_equal_range
{
   template<class Cont>
   void operator()(Cont &c) const
   {  c.equal_range(value(4));  }
};

struct cont_equal_range_const
{
   template<class Cont>
   void operator()(const Cont &c) const
   {  c.equal_range(value(4));  }
};

struct cont_erase_key
{
   template<class Cont>
   void operator()(Cont &c) const
   {  c.erase(value(4));  }
};

struct cont_erase_and_dispose_key
{
   template<class Cont>
   void operator()(Cont &c) const
   {  c.erase_and_dispose(value(4), null_disposer());  }
};

struct cont_equal
{
   template<class Cont>
   void operator()(const Cont &c) const
   {  (void)(c == c);  }
};

struct cont_less
{
   template<class Cont>
   void operator()(const Cont &c) const
   {  (void)(c < c);  }
};

struct cont_iterator_to
{
   template<class Cont>
   void operator()(Cont &c) const
   {  c.iterator_to(*c.begin());  }
};

struct cont_iterator_to_const
{
   template<class Cont>
   void operator()(const Cont &c) const
   {  c.iterator_to(*c.begin());  }
};

struct cont_erase_it
{
   template<class Cont>
   void operator()(Cont &c) const
   {  c.erase(c.cbegin());  }
};

struct cont_erase_range
{
   template<class Cont>
   void operator()(Cont &c) const
   {  c.erase(c.cbegin(), ++c.cbegin());  }
};

struct cont_erase_and_dispose_it
{
   template<class Cont>
   void operator()(Cont &c) const
   {  c.erase_and_dispose(c.cbegin(), null_disposer());  }
};

struct cont_erase_and_dispose_range
{
   template<class Cont>
   void operator()(Cont &c) const
   {  c.erase_and_dispose(c.cbegin(), ++c.cbegin(), null_disposer());  }
};

struct cont_merge
{
   template<class Cont>
   void operator()(Cont &c, Cont &c2) const
   {  c.merge(c2);  }
};

//insert_unique_commit is noexcept: the debug check of the insertion
//position calls the comparator and its exceptions must not escape
template<class Cont>
void test_insert_commit()
{
   //Insert between two elements, so that both neighbours are compared
   value v0(0), v2(2), v(1);
   Cont c;
   c.insert(v0);
   c.insert(v2);
   typename Cont::insert_commit_data data;
   BOOST_TEST(c.insert_check(v, data).second);
   g_pred_throw_active = true;
   bool thrown = false;
   try{
      c.insert_commit(v, data);
   }
   catch(test_exception &){
      thrown = true;
   }
   g_pred_throw_active = false;
   BOOST_TEST(!thrown);
   BOOST_TEST(c.size() == 3u);
   c.clear();
}

//////////////////////////////////////////
//Tree containers: insertion, search and erasure call the comparator
//(and the priority comparator in treaps), operator== and operator<
//call the value operators. merge calls the comparator.
//////////////////////////////////////////

struct tree_insert_hint
{
   value *v_;
   template<class Cont>
   void operator()(Cont &c) const
   {  c.insert(c.cbegin(), *v_);  }
};

struct tree_lower_bound
{
   template<class Cont>
   void operator()(Cont &c) const
   {  c.lower_bound(value(4));  }
};

struct tree_upper_bound
{
   template<class Cont>
   void operator()(const Cont &c) const
   {  c.upper_bound(value(4));  }
};

struct tree_bounded_range
{
   template<class Cont>
   void operator()(Cont &c) const
   {  c.bounded_range(value(2), value(5), true, true);  }
};

struct tree_push_back
{
   value *v_;
   template<class Cont>
   void operator()(Cont &c) const
   {  c.push_back(*v_);  }
};

struct tree_push_front
{
   value *v_;
   template<class Cont>
   void operator()(Cont &c) const
   {  c.push_front(*v_);  }
};

struct tree_insert_before
{
   value *v_;
   template<class Cont>
   void operator()(Cont &c) const
   {  c.insert_before(c.cbegin(), *v_);  }
};

//insert_check only exists in containers with unique keys
template<class Cont>
void test_tree_unique(Cont &, bool_tag<false>)
{}

template<class Cont>
void test_tree_unique(Cont &c, bool_tag<true>)
{
   test_throws(c, cont_insert_check(), g_pred_throw_active);
   test_insert_commit<Cont>();
}

//Treaps: insert_check also takes the priority
struct treap_insert_check
{
   template<class Cont>
   void operator()(Cont &c) const
   {
      typename Cont::insert_commit_data data;
      c.insert_check(value(4), value(4), data);
   }
};

template<class Cont>
void test_treap_unique(Cont &, bool_tag<false>)
{}

template<class Cont>
void test_treap_unique(Cont &c, bool_tag<true>)
{
   test_throws(c, treap_insert_check(), g_pred_throw_active);

   //insert_commit is noexcept
   value v0(0);
   typename Cont::insert_commit_data data;
   BOOST_TEST(c.insert_check(v0, v0, data).second);
   g_pred_throw_active = true;
   c.insert_commit(v0, data);
   g_pred_throw_active = false;
   BOOST_TEST(c.size() == N + 1u);
   c.erase(c.iterator_to(v0));
}

//Treaps: these functions call the priority comparator, see test_treap
template<class Cont>
void test_tree_no_throw(Cont &, value (&)[N], bool_tag<true>)
{}

template<class Cont>
void test_tree_no_throw(Cont &c, value (&values)[N], bool_tag<false>)
{
   test_no_throw(c, cont_iterator_to());
   test_no_throw(c, cont_iterator_to_const());

   value v_minus1(-1), v0(0), v8(8);
   {  tree_insert_before f = { &v0 };     test_no_throw(c, f);  }
   {  tree_push_front f = { &v_minus1 };  test_no_throw(c, f);  }
   {  tree_push_back f = { &v8 };         test_no_throw(c, f);  }
   BOOST_TEST(c.size() == N + 3u);
   //Erases -1, 0, 1 and 2
   test_no_throw(c, cont_erase_it());
   test_no_throw(c, cont_erase_range());
   test_no_throw(c, cont_erase_and_dispose_it());
   test_no_throw(c, cont_erase_and_dispose_range());
   BOOST_TEST(c.size() == N - 1u);
   BOOST_TEST(c.begin()->v_ == 3);
   c.check();
   c.clear();
   c.insert(&values[0], &values[0] + N);
}

template<class Cont, bool Unique, bool Treap>
void test_tree(value (&values)[N], value (&values2)[N2])
{
   Cont c(&values[0], &values[0] + N);

   value v(4);
   {  cont_insert f = { &v };        test_throws(c, f, g_pred_throw_active);  }
   {  tree_insert_hint f = { &v };   test_throws(c, f, g_pred_throw_active);  }
   {  cont_insert_range f = { &v };  test_throws(c, f, g_pred_throw_active);  }
   BOOST_TEST(!v.bi::set_base_hook<>::is_linked());
   BOOST_TEST(!v.rb_os_hook::is_linked());
   BOOST_TEST(!v.bi::avl_set_base_hook<>::is_linked());
   BOOST_TEST(!v.avl_os_hook::is_linked());
   BOOST_TEST(!v.bi::bs_set_base_hook<>::is_linked());
   test_throws(c, cont_find(), g_pred_throw_active);
   test_throws(c, cont_find_const(), g_pred_throw_active);
   test_throws(c, cont_count(), g_pred_throw_active);
   test_throws(c, tree_lower_bound(), g_pred_throw_active);
   test_throws(c, tree_upper_bound(), g_pred_throw_active);
   test_throws(c, cont_equal_range(), g_pred_throw_active);
   test_throws(c, cont_equal_range_const(), g_pred_throw_active);
   test_throws(c, tree_bounded_range(), g_pred_throw_active);
   test_throws(c, cont_erase_key(), g_pred_throw_active);
   test_throws(c, cont_erase_and_dispose_key(), g_pred_throw_active);
   test_throws(c, cont_equal(), g_pred_throw_active);
   test_throws(c, cont_less(), g_pred_throw_active);
   //Splay trees are restructured by the searches
   c.check();
   test_tree_unique(c, bool_tag<Unique && !Treap>());
   test_treap_unique(c, bool_tag<Unique && Treap>());
   test_tree_no_throw(c, values, bool_tag<Treap>());

   {
      Cont c2(&values2[0], &values2[0] + N2);
      test_throws_basic(c, c2, cont_merge(), values, values2);
      c2.clear();
   }
   c.clear();
}

//Treap: these functions only call the priority comparator

typedef bi::treap_multiset
   < value, bi::compare<throwing_less>, bi::priority<throwing_priority> > treap_t;

struct treap_erase_top
{
   void operator()(treap_t &c) const
   {  c.erase(c.top());  }
};

struct treap_erase_range
{
   void operator()(treap_t &c) const
   {  c.erase(c.top(), c.end());  }
};

struct treap_erase_and_dispose_top
{
   void operator()(treap_t &c) const
   {  c.erase_and_dispose(c.top(), null_disposer());  }
};

struct treap_erase_and_dispose_range
{
   void operator()(treap_t &c) const
   {  c.erase_and_dispose(c.top(), c.end(), null_disposer());  }
};

//The inserted values have the greatest priority, so they must be rotated
struct treap_insert_before
{
   value *v_;
   void operator()(treap_t &c) const
   {  c.insert_before(c.find(value(5)), *v_);  }
};

void test_treap(value (&values)[N])
{
   treap_t c(&values[0], &values[0] + N);
   BOOST_TEST(c.top()->v_ == 4);
   test_throws(c, treap_erase_top(), g_pred_throw_active);
   test_throws(c, treap_erase_range(), g_pred_throw_active);
   test_throws(c, treap_erase_and_dispose_top(), g_pred_throw_active);
   test_throws(c, treap_erase_and_dispose_range(), g_pred_throw_active);
   BOOST_TEST(c.top()->v_ == 4);

   //Inserted values, with the same priority as the top. The strong
   //guarantee leaves them unlinked if the priority comparator throws.
   value v(4);
   {  treap_insert_before f = { &v };  test_throws(c, f, g_pred_throw_active);  }
   {  tree_push_back f = { &v };       test_throws(c, f, g_pred_throw_active);  }
   {  tree_push_front f = { &v };      test_throws(c, f, g_pred_throw_active);  }
   BOOST_TEST(!v.bi::bs_set_base_hook<>::is_linked());
   c.check();
   c.clear();
}

//////////////////////////////////////////
//list/slist: remove, unique, sort and merge call operator==, operator<
//or the passed predicate.
//////////////////////////////////////////

struct list_remove
{
   template<class Cont>
   void operator()(Cont &c) const
   {  c.remove(value(4));  }
};

struct list_remove_and_dispose
{
   template<class Cont>
   void operator()(Cont &c) const
   {  c.remove_and_dispose(value(4), null_disposer());  }
};

struct list_remove_if
{
   template<class Cont>
   void operator()(Cont &c) const
   {  c.remove_if(throwing_is_4());  }
};

struct list_remove_and_dispose_if
{
   template<class Cont>
   void operator()(Cont &c) const
   {  c.remove_and_dispose_if(throwing_is_4(), null_disposer());  }
};

struct list_unique
{
   template<class Cont>
   void operator()(Cont &c) const
   {  c.unique();  }
};

struct list_unique_pred
{
   template<class Cont>
   void operator()(Cont &c) const
   {  c.unique(throwing_equal());  }
};

struct list_unique_and_dispose
{
   template<class Cont>
   void operator()(Cont &c) const
   {  c.unique_and_dispose(null_disposer());  }
};

struct list_unique_and_dispose_pred
{
   template<class Cont>
   void operator()(Cont &c) const
   {  c.unique_and_dispose(throwing_equal(), null_disposer());  }
};

struct list_sort
{
   template<class Cont>
   void operator()(Cont &c, Cont &) const
   {  c.sort();  }
};

struct list_sort_pred
{
   template<class Cont>
   void operator()(Cont &c, Cont &) const
   {  c.sort(throwing_less());  }
};

struct list_merge_pred
{
   template<class Cont>
   void operator()(Cont &c, Cont &c2) const
   {  c.merge(c2, throwing_less());  }
};

template<class Cont>
void test_sequence(value (&values)[N], value (&values2)[N2])
{
   Cont c(&values[0], &values[0] + N);
   test_throws(c, list_remove(), g_pred_throw_active);
   test_throws(c, list_remove_and_dispose(), g_pred_throw_active);
   test_throws(c, list_remove_if(), g_pred_throw_active);
   test_throws(c, list_remove_and_dispose_if(), g_pred_throw_active);
   test_throws(c, list_unique(), g_pred_throw_active);
   test_throws(c, list_unique_pred(), g_pred_throw_active);
   test_throws(c, list_unique_and_dispose(), g_pred_throw_active);
   test_throws(c, list_unique_and_dispose_pred(), g_pred_throw_active);
   test_throws(c, cont_equal(), g_pred_throw_active);
   test_throws(c, cont_less(), g_pred_throw_active);
   c.check();

   //Basic guarantee
   {
      Cont c2(&values2[0], &values2[0] + N2);
      test_throws_basic(c, c2, cont_merge(), values, values2);
      test_throws_basic(c, c2, list_merge_pred(), values, values2);
      c2.clear();
   }
   c.clear();
   {
      //Reverse order, so that sort must move the elements
      Cont c2;
      c.insert(c.end(), &values[0], &values[0] + N);
      c.reverse();
      test_throws_basic(c, c2, list_sort(), values, values2);
      c.clear();
      c.insert(c.end(), &values[0], &values[0] + N);
      c.reverse();
      test_throws_basic(c, c2, list_sort_pred(), values, values2);
   }
   c.clear();
}

//////////////////////////////////////////
//Unordered containers: insertion, search and erasure call the hasher and the equality
//functor (operator==). The rehash functions and clone_from call the hasher unless
//the hash is stored. The noexcept functions must not call them.
//////////////////////////////////////////

struct uset_bucket
{
   template<class Cont>
   void operator()(const Cont &c) const
   {  c.bucket(value(4));  }
};

struct uset_local_iterator_to
{
   template<class Cont>
   void operator()(Cont &c) const
   {  c.local_iterator_to(*c.begin());  }
};

struct uset_local_iterator_to_const
{
   template<class Cont>
   void operator()(const Cont &c) const
   {  c.local_iterator_to(*c.begin());  }
};

struct uset_incremental_rehash
{
   template<class Cont>
   void operator()(Cont &c) const
   {  c.incremental_rehash(true);  }
};

//Checks that the container is empty, but usable, after a failed rehash
template<class Cont>
void test_empty_after_throw(Cont &c, value (&values)[N])
{
   BOOST_TEST(c.empty());
   BOOST_TEST(c.begin() == c.end());
   c.insert(&values[0], &values[0] + N);
   BOOST_TEST(c.size() == N);
   BOOST_TEST(iterated_size(c) == N);
}

//insert_check only exists in containers with unique keys
template<class Cont>
void test_unordered_unique(Cont &, bool_tag<false>)
{}

template<class Cont>
void test_unordered_unique(Cont &c, bool_tag<true>)
{
   test_throws(c, cont_insert_check(), g_hash_throw_armed);
   test_throws(c, cont_insert_check(), g_pred_throw_active);

   //insert_commit and insert_fast_commit are noexcept
   value v(8);
   typename Cont::insert_commit_data data;
   BOOST_TEST(c.insert_check(v, data).second);
   g_pred_throw_active = g_hash_throw_armed = true;
   c.insert_commit(v, data);
   g_pred_throw_active = g_hash_throw_armed = false;
   BOOST_TEST(c.size() == N + 1u);
   c.erase(c.iterator_to(v));

   BOOST_TEST(c.insert_check(v, data).second);
   g_pred_throw_active = g_hash_throw_armed = true;
   c.insert_fast_commit(v, data);
   g_pred_throw_active = g_hash_throw_armed = false;
   BOOST_TEST(c.size() == N + 1u);
   c.erase(c.iterator_to(v));
}

//iterator_to calls the hasher with linear buckets if the hash is not stored
template<class Cont>
void test_unordered_iterator_to(Cont &c, bool_tag<false>)
{
   test_no_throw(c, cont_iterator_to());
   test_no_throw(c, cont_iterator_to_const());
}

template<class Cont>
void test_unordered_iterator_to(Cont &c, bool_tag<true>)
{
   test_throws(c, cont_iterator_to(), g_hash_throw_armed);
   test_throws(c, cont_iterator_to_const(), g_hash_throw_armed);
}

//Only incremental containers have incremental_rehash
template<class Cont>
void test_unordered_incremental(Cont &, bool_tag<false>)
{}

template<class Cont>
void test_unordered_incremental(Cont &c, bool_tag<true>)
{
   //The initial split count is half the bucket count, so the container can grow
   const std::size_t split = c.split_count();
   BOOST_TEST(split < c.bucket_count());
   BOOST_IF_CONSTEXPR(!Cont::store_hash){
      //Strong guarantee
      test_throws(c, uset_incremental_rehash(), g_hash_throw_armed);
      BOOST_TEST(c.split_count() == split);
   }
   BOOST_TEST(c.incremental_rehash(true));
   BOOST_TEST(c.split_count() == split + 1u);
   //Shrinking does not call the hasher
   g_hash_throw_armed = true;
   BOOST_TEST(c.incremental_rehash(false));
   BOOST_TEST(g_hash_throw_armed);
   g_hash_throw_armed = false;
   BOOST_TEST(c.split_count() == split);
}

template<class Cont>
void test_unordered(value (&values)[N], std::size_t bucket_count1, std::size_t bucket_count2)
{
   typedef typename Cont::bucket_type   bucket_t;
   typedef typename Cont::bucket_traits bucket_traits_t;
   //Linear buckets need one more bucket for the sentinel
   const std::size_t overhead = Cont::linear_buckets ? 1u : 0u;
   bucket_t buckets1[32], buckets2[32], buckets3[32];

   Cont c(&values[0], &values[0] + N, bucket_traits_t(buckets1, bucket_count1 + overhead));

   //Functions that call the hasher and the equality functor. Each one is tested twice:
   //first the hasher throws, then the hasher works and the equality functor throws.
   value v(4);
   bool *const flags[] = { &g_hash_throw_armed, &g_pred_throw_active };
   for(std::size_t i = 0; i != sizeof(flags)/sizeof(flags[0]); ++i){
      bool &armed = *flags[i];
      {  cont_insert f = { &v };        test_throws(c, f, armed);  }
      {  cont_insert_range f = { &v };  test_throws(c, f, armed);  }
      test_throws(c, cont_find(), armed);
      test_throws(c, cont_find_const(), armed);
      test_throws(c, cont_count(), armed);
      test_throws(c, cont_equal_range(), armed);
      test_throws(c, cont_equal_range_const(), armed);
      test_throws(c, cont_equal(), armed);
      test_throws(c, cont_erase_key(), armed);
      test_throws(c, cont_erase_and_dispose_key(), armed);
   }
   BOOST_TEST(!v.bi::unordered_set_base_hook<>::is_linked());
   BOOST_TEST(!v.mk_hook::is_linked());
   test_throws(c, uset_bucket(), g_hash_throw_armed);
   test_unordered_unique(c, bool_tag<Cont::unique_keys>());
   test_unordered_iterator_to(c, bool_tag<Cont::linear_buckets && !Cont::store_hash>());

   //noexcept functions
   test_no_throw(c, uset_local_iterator_to());
   test_no_throw(c, uset_local_iterator_to_const());
   test_no_throw(c, cont_erase_it());
   test_no_throw(c, cont_erase_range());
   test_no_throw(c, cont_erase_and_dispose_it());
   test_no_throw(c, cont_erase_and_dispose_range());
   BOOST_TEST(c.size() == N - 4u);
   c.clear();
   c.insert(&values[0], &values[0] + N);

   test_unordered_incremental(c, bool_tag<Cont::incremental>());

   //clone_from with a different bucket count does not copy the structure: it calls the
   //hasher if the hash is not stored. The cloned elements are disposed on exception.
   BOOST_IF_CONSTEXPR(!Cont::store_hash){
      Cont c2(bucket_traits_t(buckets3, bucket_count2 + overhead));
      bool thrown = false;
      g_hash_throw_armed = true;
      try{
         c2.clone_from(c, new_cloner(), delete_disposer());
      }
      catch(test_exception &){
         thrown = true;
      }
      g_hash_throw_armed = false;
      BOOST_TEST(thrown);
      BOOST_TEST(c2.empty());
      BOOST_TEST(c2.begin() == c2.end());
      BOOST_TEST(c.size() == N);
   }

   //rehash calls the hasher if the hash is not stored, full_rehash always calls it.
   //Basic guarantee: the elements are unlinked and the container is left empty.
   {
      bool thrown = false;
      g_hash_throw_armed = true;
      try{
         c.rehash(bucket_traits_t(buckets2, bucket_count2 + overhead));
      }
      catch(test_exception &){
         thrown = true;
      }
      g_hash_throw_armed = false;
      BOOST_TEST(thrown == !Cont::store_hash);
      if(thrown)
         test_empty_after_throw(c, values);
      BOOST_TEST(c.size() == N);
   }
   {
      bool thrown = false;
      g_hash_throw_armed = true;
      try{
         c.full_rehash();
      }
      catch(test_exception &){
         thrown = true;
      }
      g_hash_throw_armed = false;
      BOOST_TEST(thrown);
      test_empty_after_throw(c, values);
   }
   c.clear();
}

//Rehash of linear buckets: other bucket array sizes and the same bucket array

typedef bi::unordered_multiset
   < value, bi::hash<throwing_hash>, bi::linear_buckets<true> > linear_uset_t;
typedef linear_uset_t::bucket_type   linear_bucket_t;
typedef linear_uset_t::bucket_traits linear_bucket_traits_t;

//new_bucket_count == 0 means full_rehash. The rehash fails and leaves the container empty:
//it must still be usable (linear buckets need the sentinel bucket to stop the iteration).
void test_linear_rehash
   ( value (&values)[N], linear_bucket_t *old_buckets, std::size_t old_bucket_count
   , linear_bucket_t *new_buckets, std::size_t new_bucket_count)
{
   linear_uset_t c( &values[0], &values[0] + N
                  , linear_bucket_traits_t(old_buckets, old_bucket_count));
   BOOST_TEST(c.size() == N);
   bool thrown = false;
   g_hash_throw_armed = true;
   try{
      if(new_bucket_count)
         c.rehash(linear_bucket_traits_t(new_buckets, new_bucket_count));
      else
         c.full_rehash();
   }
   catch(test_exception &){
      thrown = true;
   }
   g_hash_throw_armed = false;
   BOOST_TEST(thrown);
   for(std::size_t i = 0; i != N; ++i)
      BOOST_TEST(!values[i].bi::unordered_set_base_hook<>::is_linked());
   test_empty_after_throw(c, values);
   c.clear();
}

void test_linear_rehash(value (&values)[N])
{
   //Linear buckets need one more bucket for the sentinel
   linear_bucket_t buckets1[16], buckets2[16];
   //Different bucket arrays, bigger and smaller
   test_linear_rehash(values, buckets1, 6, buckets2, 12);
   test_linear_rehash(values, buckets1, 12, buckets2, 6);
   //Same bucket array: bigger (the old sentinel is a bucket of the new array) and smaller
   test_linear_rehash(values, buckets1, 6, buckets1, 12);
   test_linear_rehash(values, buckets1, 12, buckets1, 6);
   //full_rehash
   test_linear_rehash(values, buckets1, 6, 0, 0);
}

//////////////////////////////////////////
//Tested variants
//////////////////////////////////////////

void test_trees(value (&values)[N], value (&values2)[N2])
{
   typedef bi::compare<throwing_less> cmp;
   typedef bi::base_hook<bi::bs_set_base_hook<> > bs_hook;

   //Red-black: compressed node colors, linear time size
   test_tree< bi::set<value, cmp>, true, false >(values, values2);
   test_tree< bi::multiset<value, cmp, bi::constant_time_size<false> >, false, false >(values, values2);
   test_tree< bi::set<value, cmp, bi::base_hook<rb_os_hook> >, true, false >(values, values2);
   //AVL: compressed balance
   test_tree< bi::avl_set<value, cmp>, true, false >(values, values2);
   test_tree< bi::avl_multiset<value, cmp, bi::base_hook<avl_os_hook> >, false, false >(values, values2);
   //Scapegoat: float and integer alpha
   test_tree< bi::sg_set<value, cmp, bs_hook>, true, false >(values, values2);
   test_tree< bi::sg_multiset<value, cmp, bs_hook, bi::floating_point<false> >, false, false >(values, values2);
   //Splay: searches restructure the tree
   test_tree< bi::splay_set<value, cmp, bs_hook>, true, false >(values, values2);
   test_tree< bi::splay_multiset<value, cmp, bs_hook, bi::constant_time_size<false> >, false, false >(values, values2);
   //Unbalanced
   test_tree< bi::bs_set<value, cmp>, true, false >(values, values2);
   test_tree< bi::bs_multiset<value, cmp>, false, false >(values, values2);
   //Treap: priority comparator
   test_tree< bi::treap_set<value, cmp, bi::priority<throwing_priority> >, true, true >(values, values2);
   test_tree< treap_t, false, true >(values, values2);
   test_treap(values);
}

void test_sequences(value (&values)[N], value (&values2)[N2])
{
   test_sequence< bi::list<value> >(values, values2);
   test_sequence< bi::list<value, bi::constant_time_size<false> > >(values, values2);
   test_sequence< bi::list<value, bi::base_hook<au_list_hook>, bi::constant_time_size<false> > >(values, values2);
   test_sequence< bi::slist<value> >(values, values2);
   test_sequence< bi::slist<value, bi::constant_time_size<false> > >(values, values2);
   test_sequence< bi::slist<value, bi::linear<true> > >(values, values2);
   test_sequence< bi::slist<value, bi::cache_last<true> > >(values, values2);
   test_sequence< bi::slist<value, bi::linear<true>, bi::cache_last<true> > >(values, values2);
   test_sequence< bi::slist<value, bi::base_hook<au_slist_hook>, bi::constant_time_size<false> > >(values, values2);
}

void test_unordereds(value (&values)[N])
{
   typedef bi::hash<throwing_hash> hash;
   typedef bi::unordered_set<value, hash> uset_t;
   typedef bi::unordered_multiset<value, hash> umset_t;
   typedef bi::unordered_multiset
      < value, hash, bi::power_2_buckets<true>, bi::constant_time_size<false> > p2_umset_t;
   typedef bi::unordered_set
      < value, hash, bi::linear_buckets<true>, bi::cache_begin<true> > cache_linear_uset_t;
   typedef bi::unordered_set
      < value, hash, bi::incremental<true> > incremental_uset_t;
   typedef bi::unordered_set
      < value, hash, bi::fastmod_buckets<true> > fastmod_uset_t;
   //Stored hash and multikey optimization
   typedef bi::unordered_multiset
      < value, bi::base_hook<mk_hook>, hash, bi::compare_hash<true>
      , bi::cache_begin<true> > mk_umset_t;
   typedef bi::unordered_multiset
      < value, bi::base_hook<mk_hook>, hash, bi::linear_buckets<true> > mk_linear_umset_t;

   test_unordered<uset_t>(values, 8, 16);
   test_unordered<umset_t>(values, 8, 16);
   test_unordered<p2_umset_t>(values, 8, 16);
   test_unordered<cache_linear_uset_t>(values, 8, 16);
   test_unordered<incremental_uset_t>(values, 8, 16);
   test_unordered<fastmod_uset_t>(values
      , fastmod_uset_t::suggested_upper_bucket_count(8), fastmod_uset_t::suggested_upper_bucket_count(16));
   test_unordered<mk_umset_t>(values, 8, 16);
   test_unordered<mk_linear_umset_t>(values, 8, 16);
   test_linear_rehash(values);
}

int main()
{
   value values[N];
   value values2[N2];
   init_values(values, values2);

   test_trees(values, values2);
   test_sequences(values, values2);
   test_unordereds(values);

   return boost::report_errors();
}

#endif //BOOST_NO_EXCEPTIONS
