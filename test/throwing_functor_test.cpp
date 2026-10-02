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

//Tests that exceptions thrown by user functors (comparators, priority
//comparators, operator==) propagate to the caller: the functions that call
//them must not be marked noexcept, otherwise std::terminate is called.

#include <boost/intrusive/set.hpp>
#include <boost/intrusive/avl_set.hpp>
#include <boost/intrusive/splay_set.hpp>
#include <boost/intrusive/treap_set.hpp>
#include <boost/intrusive/list.hpp>
#include <boost/intrusive/slist.hpp>
#include <boost/core/lightweight_test.hpp>
#include <cstddef>

namespace bi = boost::intrusive;

//When armed, the functors throw on their first call
static bool g_armed = false;

struct test_exception {};

static void maybe_throw()
{
   if(g_armed){
      g_armed = false;
      throw test_exception();
   }
}

struct value
   : public bi::set_base_hook<>
   , public bi::avl_set_base_hook<>
   , public bi::bs_set_base_hook<>
   , public bi::list_base_hook<>
   , public bi::slist_base_hook<>
{
   int v_;

   explicit value(int v = 0) : v_(v) {}

   //Used by list/slist remove
   friend bool operator==(const value &a, const value &b)
   {  maybe_throw(); return a.v_ == b.v_;  }
};

struct throwing_less
{
   bool operator()(const value &a, const value &b) const
   {  maybe_throw(); return a.v_ < b.v_;  }
};

//Elements nearer to 4 have more priority, so that the top of the treap is 4
//and the treap of 1..7 is balanced: erasing the top needs a rotation.
struct throwing_priority
{
   static int distance(const value &a)
   {  return a.v_ < 4 ? 4 - a.v_ : a.v_ - 4;  }

   bool operator()(const value &a, const value &b) const
   {  maybe_throw(); return distance(a) < distance(b);  }
};

static const std::size_t N = 7u;

//Checks that `f(c)` throws test_exception and that the container is unchanged
template<class Cont, class F>
void test_throws(Cont &c, F f)
{
   const std::size_t old_size = c.size();
   bool thrown = false;
   g_armed = true;
   try{
      f(c);
   }
   catch(test_exception &){
      thrown = true;
   }
   g_armed = false;
   BOOST_TEST(thrown);
   BOOST_TEST(c.size() == old_size);
}

//Tree containers: erase(key) calls the comparator

struct erase_key
{
   template<class Cont>
   void operator()(Cont &c) const
   {  c.erase(value(4));  }
};

struct erase_and_dispose_key
{
   struct null_disposer {  void operator()(value *) const {}  };

   template<class Cont>
   void operator()(Cont &c) const
   {  c.erase_and_dispose(value(4), null_disposer());  }
};

template<class Cont>
void test_tree_erase_key(value (&values)[N])
{
   Cont c(&values[0], &values[0] + N);
   test_throws(c, erase_key());
   test_throws(c, erase_and_dispose_key());
   c.clear();
}

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
   g_armed = true;
   bool thrown = false;
   try{
      c.insert_commit(v, data);
   }
   catch(test_exception &){
      thrown = true;
   }
   g_armed = false;
   BOOST_TEST(!thrown);
   BOOST_TEST(c.size() == 3u);
   c.clear();
}

//Treap: insertion and erasure call the priority comparator

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
   struct null_disposer {  void operator()(value *) const {}  };

   void operator()(treap_t &c) const
   {  c.erase_and_dispose(c.top(), null_disposer());  }
};

struct treap_erase_and_dispose_range
{
   struct null_disposer {  void operator()(value *) const {}  };

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

struct treap_push_back
{
   value *v_;
   void operator()(treap_t &c) const
   {  c.push_back(*v_);  }
};

struct treap_push_front
{
   value *v_;
   void operator()(treap_t &c) const
   {  c.push_front(*v_);  }
};

void test_treap(value (&values)[N])
{
   treap_t c(&values[0], &values[0] + N);
   BOOST_TEST(c.top()->v_ == 4);
   test_throws(c, erase_key());
   test_throws(c, treap_erase_top());
   test_throws(c, treap_erase_range());
   test_throws(c, treap_erase_and_dispose_top());
   test_throws(c, treap_erase_and_dispose_range());
   BOOST_TEST(c.top()->v_ == 4);

   //Inserted values, with the same priority as the top. The strong
   //guarantee leaves them unlinked if the priority comparator throws.
   value v(4);
   {  treap_insert_before f = { &v };  test_throws(c, f);  }
   {  treap_push_back f = { &v };      test_throws(c, f);  }
   {  treap_push_front f = { &v };     test_throws(c, f);  }
   BOOST_TEST(!v.bi::bs_set_base_hook<>::is_linked());
   c.clear();
}

//list/slist: remove calls operator==

struct remove_value
{
   template<class Cont>
   void operator()(Cont &c) const
   {  c.remove(value(4));  }
};

struct remove_and_dispose_value
{
   struct null_disposer {  void operator()(value *) const {}  };

   template<class Cont>
   void operator()(Cont &c) const
   {  c.remove_and_dispose(value(4), null_disposer());  }
};

template<class Cont>
void test_sequence_remove(value (&values)[N])
{
   Cont c(&values[0], &values[0] + N);
   test_throws(c, remove_value());
   test_throws(c, remove_and_dispose_value());
   c.clear();
}

int main()
{
   value values[N];
   for(std::size_t i = 0; i != N; ++i)
      values[i].v_ = int(i + 1u);

   test_tree_erase_key< bi::set<value, bi::compare<throwing_less> > >(values);
   test_tree_erase_key< bi::multiset<value, bi::compare<throwing_less> > >(values);
   test_tree_erase_key< bi::avl_set<value, bi::compare<throwing_less> > >(values);
   test_tree_erase_key< bi::splay_multiset
      <value, bi::compare<throwing_less>, bi::base_hook<bi::bs_set_base_hook<> > > >(values);
   test_insert_commit< bi::set<value, bi::compare<throwing_less> > >();
   test_insert_commit< bi::avl_set<value, bi::compare<throwing_less> > >();
   test_treap(values);
   test_sequence_remove< bi::list<value> >(values);
   test_sequence_remove< bi::slist<value> >(values);
   test_sequence_remove< bi::slist<value, bi::linear<true> > >(values);

   return boost::report_errors();
}

#endif //BOOST_NO_EXCEPTIONS
