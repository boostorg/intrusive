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

#include <boost/intrusive/unordered_set.hpp>
#include <boost/intrusive/unordered_bucket_manager.hpp>
#include <boost/move/utility_core.hpp>
#include <boost/static_assert.hpp>

#include <boost/core/lightweight_test.hpp>
#include <cstddef>
#include <cstdio>
#include <functional>
#include <memory>
#include <vector>

namespace bi = boost::intrusive;

struct item : public bi::unordered_set_base_hook<>
{
   int key_;
   int mapped_;   //make it map-like: key + payload

   explicit item(int k, int m = 0) : key_(k), mapped_(m) {}

   friend bool operator==(const item &a, const item &b)
   {  return a.key_ == b.key_;  }
};

//Hash for the whole value
struct item_hash
{
   std::size_t operator()(const item &i) const
   {  return std::size_t(i.key_) * 2654435761u;  }
};

//Hash/equality for heterogeneous (map-style) lookup/insertion by int key.
//Must be compatible with item_hash.
struct int_hash
{
   std::size_t operator()(int k) const
   {  return std::size_t(k) * 2654435761u;  }
};

struct int_item_equal
{
   bool operator()(int k, const item &i) const {  return k == i.key_;  }
   bool operator()(const item &i, int k) const {  return k == i.key_;  }
};

struct delete_disposer
{
   template<class T>
   void operator()(T *p) const {  delete p;  }
};

//Value whose hook stores the hash and groups equivalent elements, as both
//options change how a rehash relinks the nodes into the new bucket array
struct grouped_item
   : public bi::unordered_set_base_hook< bi::store_hash<true>, bi::optimize_multikey<true> >
{
   int key_;

   explicit grouped_item(int k) : key_(k) {}

   friend bool operator==(const grouped_item &a, const grouped_item &b)
   {  return a.key_ == b.key_;  }
};

struct grouped_item_hash
{
   std::size_t operator()(const grouped_item &i) const
   {  return std::size_t(i.key_) * 2654435761u;  }
};

template<class SizeType>
bool is_power_of_two(SizeType n)
{  return n != 0 && 0 == (n & SizeType(n - 1));  }

typedef bi::unordered_set<item, bi::hash<item_hash> >         uset_t;
typedef bi::unordered_multiset<item, bi::hash<item_hash> >    umset_t;
typedef bi::unordered_bucket_manager<uset_t>                  uset_mgr_t;
typedef bi::unordered_bucket_manager<umset_t>                 umset_mgr_t;

//Allocator that counts the bucket array allocations. It holds no state (the
//counter is static), so the manager keeps the empty base optimization.
template<class T>
struct counting_allocator
{
   typedef T                  value_type;
   typedef T*                 pointer;
   typedef const T*           const_pointer;
   typedef T&                 reference;
   typedef const T&           const_reference;
   typedef std::size_t        size_type;
   typedef std::ptrdiff_t     difference_type;

   template<class U> struct rebind {  typedef counting_allocator<U> other;  };

   counting_allocator() {}
   template<class U> counting_allocator(const counting_allocator<U> &) {}

   pointer allocate(size_type n)
   {
      ++allocations;
      return static_cast<pointer>(::operator new(n * sizeof(T)));
   }

   void deallocate(pointer p, size_type)
   {  ::operator delete(static_cast<void*>(p));  }

   static std::size_t allocations;
};

template<class T> std::size_t counting_allocator<T>::allocations = 0;

//The rebound allocator is an empty base, so an empty allocator (the default)
//must add no size to the manager.
struct manager_layout
{
   uset_t::bucket_ptr   buckets_;
   uset_t::size_type    bucket_count_;
   float                max_load_factor_;
   uset_t::size_type    max_elements_;
};

BOOST_STATIC_ASSERT((sizeof(uset_mgr_t) == sizeof(manager_layout)));

//Grows and shrinks a container whose option list demands a power of two
//bucket count, and checks the count after every step.
template<class Cont>
void test_power_of_two(const char *name)
{
   typedef bi::unordered_bucket_manager<Cont>   mgr_t;

   BOOST_STATIC_ASSERT((Cont::power_2_buckets));

   const int n = 2000;
   mgr_t  mgr;
   Cont   c(mgr.traits());
   BOOST_TEST(is_power_of_two(mgr.bucket_count()));

   for(int i = 0; i != n; ++i){
      mgr.reserve_additional(c);
      c.insert(*new item(i));
      BOOST_TEST(is_power_of_two(mgr.bucket_count()));
   }
   BOOST_TEST(c.size() == typename Cont::size_type(n));
   BOOST_TEST(c.bucket_count() == mgr.bucket_count());
   for(int i = 0; i != n; ++i)
      BOOST_TEST(c.find(item(i)) != c.end());

   mgr.reserve(c, typename Cont::size_type(10 * n));
   BOOST_TEST(is_power_of_two(mgr.bucket_count()));

   c.erase_and_dispose(c.begin(), c.end(), delete_disposer());
   BOOST_TEST(c.empty());

   mgr.shrink_to_fit(c);
   BOOST_TEST(is_power_of_two(mgr.bucket_count()));
   std::printf("%s: buckets=%u\n", name, unsigned(mgr.bucket_count()));
}

//Fills and empties a container whose manager holds the given allocator, so
//that every allocator path is exercised
template<class Mgr>
void test_allocator_path(const char *name)
{
   umset_t::size_type buckets = 0;
   {
      Mgr      mgr;
      umset_t  mset(mgr.traits());
      mgr.reserve_additional(mset, umset_t::size_type(500));
      for(int i = 0; i != 500; ++i)
         mset.insert(*new item(i));
      BOOST_TEST(mset.size() == 500u);
      for(int i = 0; i != 500; ++i)
         BOOST_TEST(mset.find(item(i)) != mset.end());
      buckets = mgr.bucket_count();
      BOOST_TEST(mset.bucket_count() == buckets);
      mset.clear_and_dispose(delete_disposer());
      mgr.shrink_to_fit(mset);
      BOOST_TEST(mgr.bucket_count() < buckets);
   }
   std::printf("%s: buckets=%u\n", name, unsigned(buckets));
}

//Invariant checked after every phase: the container really uses the
//manager's array and the load factor is respected.
template<class Mgr, class Cont>
void check_sync(const Mgr &mgr, const Cont &c)
{
   BOOST_TEST(c.bucket_pointer() == mgr.traits().bucket_begin());
   BOOST_TEST(c.bucket_count()   == mgr.bucket_count());
   BOOST_TEST(float(c.size()) <= mgr.max_load_factor() * float(mgr.bucket_count()));
}

//Checks that the bucket count is accepted by the container: a power of two
//with power_2_buckets, a value of the prime table otherwise
//(suggested_lower_bucket_count returns n only if n is in the table)
template<class Cont>
bool is_valid_bucket_count(typename Cont::size_type n)
{
   BOOST_IF_CONSTEXPR(Cont::power_2_buckets)
      return is_power_of_two(n);
   else
      return Cont::suggested_lower_bucket_count(n) == n;
}

//Checks that every key in [0, n) is found `count` times, or only odd keys
//when `odd_only` is true (even keys are then not found)
template<class Cont>
void check_keys(const Cont &c, int n, typename Cont::size_type count, bool odd_only)
{
   typedef typename Cont::value_type   value_type;
   for(int i = 0; i != n; ++i){
      const bool present = !odd_only || (i % 2 != 0);
      BOOST_TEST(c.count(value_type(i)) == (present ? count : 0u));
   }
}

//Grows, rehashes and shrinks a multiset with the given options. Each key is
//inserted twice so that groups of equivalent elements are relinked by every
//rehash. The manager's bucket count must be the container's bucket count
//(without the additional sentinel bucket of linear_buckets) and a valid
//bucket count.
template<class Cont>
void test_bucket_options(const char *name)
{
   typedef bi::unordered_bucket_manager<Cont>   mgr_t;
   typedef typename Cont::size_type             size_type;
   typedef typename Cont::value_type            value_type;

   const int n = 1000;
   mgr_t  mgr;
   Cont   c(mgr.traits());
   check_sync(mgr, c);
   BOOST_TEST(is_valid_bucket_count<Cont>(mgr.bucket_count()));

   for(int rep = 0; rep != 2; ++rep){
      for(int i = 0; i != n; ++i){
         mgr.reserve_additional(c);
         c.insert(*new value_type(i));
         check_sync(mgr, c);
         BOOST_TEST(is_valid_bucket_count<Cont>(mgr.bucket_count()));
      }
   }
   BOOST_TEST(c.size() == size_type(2 * n));
   check_keys(c, n, 2u, false);

   mgr.reserve(c, size_type(10 * n));
   check_sync(mgr, c);
   BOOST_TEST(is_valid_bucket_count<Cont>(mgr.bucket_count()));
   check_keys(c, n, 2u, false);

   mgr.rehash(c, size_type(3 * n));
   check_sync(mgr, c);
   BOOST_TEST(is_valid_bucket_count<Cont>(mgr.bucket_count()));
   check_keys(c, n, 2u, false);

   //Erase half of the keys and release buckets
   for(int i = 0; i < n; i += 2)
      BOOST_TEST(c.erase_and_dispose(value_type(i), delete_disposer()) == 2u);
   BOOST_TEST(c.size() == size_type(n));
   mgr.shrink_to_fit(c);
   check_sync(mgr, c);
   BOOST_TEST(is_valid_bucket_count<Cont>(mgr.bucket_count()));
   check_keys(c, n, 2u, true);

   //A smaller maximum load factor applies to the next growth
   mgr.max_load_factor(0.25f);
   BOOST_TEST(mgr.reserve_additional(c));
   check_sync(mgr, c);
   BOOST_TEST(is_valid_bucket_count<Cont>(mgr.bucket_count()));
   check_keys(c, n, 2u, true);

   c.clear_and_dispose(delete_disposer());
   std::printf("%s: buckets=%u\n", name, unsigned(mgr.bucket_count()));
}

int main()
{
   const int N = 10000;

   //////////////////////////////////////
   // unique container (set-like usage)
   //////////////////////////////////////
   {
      uset_mgr_t mgr;                     //declared BEFORE the container
      uset_t     set(mgr.traits());
      const uset_t::size_type initial_buckets = mgr.bucket_count();
      BOOST_TEST(initial_buckets != 0);
      std::printf("initial buckets: %u\n", unsigned(initial_buckets));

      std::vector<item*> storage;
      storage.reserve(std::size_t(N));

      //Massive insertion through the manager: it must grow many times
      for(int i = 0; i != N; ++i){
         item *p = new item(i, -i);
         storage.push_back(p);
         mgr.reserve_additional(set);
         std::pair<uset_t::iterator, bool> r = set.insert(*p);
         BOOST_TEST(r.second);
      }
      BOOST_TEST(set.size() == uset_t::size_type(N));
      BOOST_TEST(mgr.bucket_count() > initial_buckets);
      check_sync(mgr, set);
      std::printf("after %d inserts: buckets=%u load=%.3f\n",
         N, unsigned(mgr.bucket_count()), mgr.load_factor(set));

      //Duplicates rejected, no growth from failed inserts
      {
         item dup(123);
         mgr.reserve_additional(set);
         std::pair<uset_t::iterator, bool> r = set.insert(dup);
         BOOST_TEST(!r.second);
         BOOST_TEST(set.size() == uset_t::size_type(N));
      }

      //Plain lookups need no wrapper at all
      for(int i = 0; i != N; ++i){
         uset_t::iterator it = set.find(item(i));
         BOOST_TEST(it != set.end() && it->mapped_ == -i);
      }
      //Heterogeneous lookup by key (no temporary item needed)
      BOOST_TEST(set.count(777, int_hash(), int_item_equal()) == 1);

      //Map-style two-phase insertion: check first, create node only if new
      {
         uset_t::insert_commit_data cd;
         std::pair<uset_t::iterator, bool> r =
            set.insert_check(N + 1, int_hash(), int_item_equal(), cd);
         BOOST_TEST(r.second);                //key not present: we may commit
         item *p = new item(N + 1, 42);   //node built only on success
         storage.push_back(p);
         //Room is made between the check and the commit: commit_data holds
         //the hash, so it survives the rehash
         mgr.reserve_additional(set);
         uset_t::iterator it = set.insert_commit(*p, cd);
         BOOST_TEST(it->key_ == N + 1 && set.size() == uset_t::size_type(N + 1));

         const uset_t::size_type buckets = mgr.bucket_count();
         r = set.insert_check(N + 1, int_hash(), int_item_equal(), cd);
         BOOST_TEST(!r.second && r.first->mapped_ == 42);  //already there
         BOOST_TEST(mgr.bucket_count() == buckets);
      }
      check_sync(mgr, set);

      //reserve() then verify no rehash happens while inserting below capacity
      {
         mgr.reserve(set, uset_t::size_type(3 * N));
         const uset_t::size_type buckets_after_reserve = mgr.bucket_count();
         for(int i = N; i != 2 * N; ++i){
            if(i == N + 1)      //already inserted by insert_commit
               continue;
            item *p = new item(i);
            storage.push_back(p);
            bool rehashed = mgr.reserve_additional(set);
            BOOST_TEST(!rehashed);
            set.insert(*p);   //direct insertion is fine after reserve
         }
         BOOST_TEST(mgr.bucket_count() == buckets_after_reserve);
         check_sync(mgr, set);
      }

      //Mass erasure through the manager: the array must NOT change, as in
      //the standard unordered containers. References to surviving elements
      //stay valid across every erasure.
      {
         const uset_t::size_type big = mgr.bucket_count();
         const item &survivor = *set.find(item(100));
         for(int i = 0; i != 2 * N; ++i){
            if(i % 100 != 0){   //keep 1 of each 100
               uset_t::size_type n =
                  set.erase_and_dispose(item(i), delete_disposer());
               (void)n;
            }
         }
         BOOST_TEST(mgr.bucket_count() == big);   //no shrink on erase
         check_sync(mgr, set);
         std::printf("after erasures: size=%u buckets=%u\n",
            unsigned(set.size()), unsigned(mgr.bucket_count()));
         //Survivors still reachable, and the reference taken before the
         //erasures still designates the same element
         for(int i = 0; i < 2 * N; i += 100)
            BOOST_TEST(set.find(item(i)) != set.end());
         BOOST_TEST(survivor.key_ == 100);
         BOOST_TEST(&survivor == &*set.find(item(100)));

         //Releasing buckets is opt-in and explicit
         const bool rehashed = mgr.shrink_to_fit(set);
         BOOST_TEST(rehashed && mgr.bucket_count() < big);
         check_sync(mgr, set);
         for(int i = 0; i < 2 * N; i += 100)
            BOOST_TEST(set.find(item(i)) != set.end());
      }

      //erase by iterator + erase by range: bucket count survives emptying
      {
         const uset_t::size_type buckets = mgr.bucket_count();
         uset_t::size_type sz = set.size();
         set.erase_and_dispose(set.begin(), delete_disposer());
         BOOST_TEST(set.size() == sz - 1);
         set.erase_and_dispose(set.begin(), set.end(), delete_disposer());
         BOOST_TEST(set.empty());
         BOOST_TEST(mgr.bucket_count() == buckets);   //empty, but same buckets
      }

      //Move the manager: the container keeps working with the new owner
      {
         for(int i = 0; i != 100; ++i){
            item *p = new item(i);
            storage.push_back(p);   //storage now over-holds deleted ones; ok, raw test
            mgr.reserve_additional(set);
            set.insert(*p);
         }
         uset_mgr_t mgr2(::boost::move(mgr));
         BOOST_TEST(mgr.bucket_count() == 0);   //moved-from
         check_sync(mgr2, set);
         mgr2.reserve_additional(set);
         set.insert(*new item(100));
         BOOST_TEST(set.size() == 101);
         mgr = ::boost::move(mgr2);          //move it back via assignment
         check_sync(mgr, set);

         //shrink_to_fit and explicit rehash round-trip
         mgr.reserve(set, uset_t::size_type(5000));
         const uset_t::size_type big = mgr.bucket_count();
         mgr.shrink_to_fit(set);
         BOOST_TEST(mgr.bucket_count() < big);
         check_sync(mgr, set);
         for(int i = 0; i != 101; ++i)
            BOOST_TEST(set.find(item(i)) != set.end());
      }

      //Leave buckets empty before the manager dies (reverse destruction
      //order also destroys `set` first, which requires unhooked nodes)
      set.clear_and_dispose(delete_disposer());
      BOOST_TEST(set.empty());
   }

   //////////////////////////////////////
   // multiset usage (equal keys)
   //////////////////////////////////////
   {
      umset_mgr_t mgr(umset_t::size_type(8));  //bucket count hint
      umset_t     mset(mgr.traits());

      for(int rep = 0; rep != 5; ++rep){
         for(int i = 0; i != 1000; ++i){
            mgr.reserve_additional(mset);
            umset_t::iterator it = mset.insert(*new item(i, rep));
            BOOST_TEST(it->key_ == i);
         }
      }
      BOOST_TEST(mset.size() == 5000);
      BOOST_TEST(mset.count(item(500)) == 5);
      check_sync(mgr, mset);

      //Erase all duplicates of one key
      umset_t::size_type n = mset.erase_and_dispose(item(500), delete_disposer());
      BOOST_TEST(n == 5 && mset.count(item(500)) == 0);

      //Heterogeneous erase overload (key + hash + equal)
      n = mset.erase_and_dispose(501, int_hash(), int_item_equal(),
                                 delete_disposer());
      BOOST_TEST(n == 5);
      check_sync(mgr, mset);

      //Batch insertion from external storage: one reservation for all
      std::vector<item*> more;
      for(int i = 0; i != 64; ++i) more.push_back(new item(9000 + i));
      {
         mgr.reserve_additional(mset, umset_t::size_type(more.size()));
         std::vector<item*>::iterator b = more.begin(), e = more.end();
         for(; b != e; ++b){
            mset.insert(**b);
         }
      }
      BOOST_TEST(mset.count(item(9000)) == 1);
      BOOST_TEST(mset.count(item(9063)) == 1);

      {  //clear() doesn't touch the buckets either
         const umset_t::size_type buckets = mgr.bucket_count();
         mset.clear_and_dispose(delete_disposer());
         BOOST_TEST(mset.empty() && mgr.bucket_count() == buckets);
      }
   }

   //swap between two managers/containers
   {
      uset_mgr_t ma, mb;
      uset_t sa(ma.traits()), sb(mb.traits());
      item x(1), y(2);
      ma.reserve_additional(sa);
      sa.insert(x);
      mb.reserve_additional(sb);
      sb.insert(y);
      ma.swap(mb);      //managers swap arrays...
      sa.swap(sb);      //...and containers swap contents/traits: re-paired
      check_sync(ma, sa);
      check_sync(mb, sb);
      BOOST_TEST(sa.find(item(2)) != sa.end());
      BOOST_TEST(sb.find(item(1)) != sb.end());
      sa.clear();
      sb.clear();       //x, y are automatic: just unlink
   }

   //////////////////////////////////////
   // reserve_additional for a whole batch: the array grows only once, while
   // a call per element grows it step by step
   //////////////////////////////////////
   {
      typedef counting_allocator<umset_t::bucket_type>       count_alloc_t;
      typedef bi::unordered_bucket_manager
         <umset_t, count_alloc_t>                            count_mgr_t;

      //An empty user allocator also gets the empty base optimization
      BOOST_STATIC_ASSERT((sizeof(count_mgr_t) == sizeof(manager_layout)));

      const int n = 1000;
      std::vector<item*> v;
      for(int i = 0; i != n; ++i)
         v.push_back(new item(i));

      umset_t::size_type fwd_buckets = 0;

      {  //one call for the batch: the initial array plus one single rehash
         count_alloc_t::allocations = 0;
         count_mgr_t mgr;
         umset_t     mset(mgr.traits());
         BOOST_TEST(count_alloc_t::allocations == 1);   //the initial array
         mgr.reserve_additional(mset, umset_t::size_type(n));
         for(int i = 0; i != n; ++i)
            mset.insert(*v[std::size_t(i)]);
         BOOST_TEST(mset.size() == umset_t::size_type(n));
         BOOST_TEST(count_alloc_t::allocations == 2);   //one rehash only
         check_sync(mgr, mset);
         fwd_buckets = mgr.bucket_count();
         mset.clear();
      }

      {  //one call per element: the same result, but many rehashes
         count_alloc_t::allocations = 0;
         count_mgr_t mgr;
         umset_t     mset(mgr.traits());
         for(int i = 0; i != n; ++i){
            mgr.reserve_additional(mset);
            mset.insert(*v[std::size_t(i)]);
         }
         BOOST_TEST(mset.size() == umset_t::size_type(n));
         BOOST_TEST(count_alloc_t::allocations > 2);
         BOOST_TEST(mgr.bucket_count() == fwd_buckets);
         check_sync(mgr, mset);
         mset.clear();
      }

      for(int i = 0; i != n; ++i)
         delete v[std::size_t(i)];
   }

   //////////////////////////////////////
   // power_2_buckets<> and incremental<>: the bucket count must stay a
   // power of two, because the container asserts it on every rehash
   //////////////////////////////////////
   {
      typedef bi::unordered_multiset
         < item, bi::hash<item_hash>, bi::power_2_buckets<true> >   pow2_t;
      typedef bi::unordered_multiset
         < item, bi::hash<item_hash>, bi::incremental<true> >       incr_t;

      test_power_of_two<pow2_t>("power_2_buckets");
      test_power_of_two<incr_t>("incremental");
   }

   //////////////////////////////////////
   // the allocator paths: void (the default, global operator new), a user
   // allocator of buckets, and std::allocator, whose pointer and size_type
   // members were removed in C++20
   //////////////////////////////////////
   {
      typedef bi::unordered_bucket_manager<umset_t>          void_mgr_t;
      typedef bi::unordered_bucket_manager
         <umset_t, counting_allocator<umset_t::bucket_type> > user_mgr_t;
      typedef bi::unordered_bucket_manager
         <umset_t, std::allocator<umset_t::bucket_type> >     std_mgr_t;

      //Every one of them is stateless, so the size stays the same
      BOOST_STATIC_ASSERT((sizeof(void_mgr_t)   == sizeof(manager_layout)));
      BOOST_STATIC_ASSERT((sizeof(user_mgr_t)   == sizeof(manager_layout)));
      BOOST_STATIC_ASSERT((sizeof(std_mgr_t)    == sizeof(manager_layout)));

      test_allocator_path<void_mgr_t>("void (operator new)");
      test_allocator_path<user_mgr_t>("user allocator");
      test_allocator_path<std_mgr_t>("std::allocator");
   }

   //////////////////////////////////////
   // Bucket options that change the accepted bucket counts or need an
   // additional sentinel bucket (linear_buckets)
   //////////////////////////////////////
   {
      typedef bi::unordered_multiset
         < item, bi::hash<item_hash>, bi::linear_buckets<true> >  lin_t;
      typedef bi::unordered_multiset
         < item, bi::hash<item_hash>, bi::linear_buckets<true>
         , bi::power_2_buckets<true> >                             lin_pow2_t;
      typedef bi::unordered_multiset
         < item, bi::hash<item_hash>, bi::linear_buckets<true>
         , bi::incremental<true> >                                 lin_incr_t;
      typedef bi::unordered_multiset
         < item, bi::hash<item_hash>, bi::fastmod_buckets<true> >  fmod_t;
      typedef bi::unordered_multiset
         < item, bi::hash<item_hash>, bi::linear_buckets<true>
         , bi::fastmod_buckets<true> >                             lin_fmod_t;

      test_bucket_options<lin_t>("linear_buckets");
      test_bucket_options<lin_pow2_t>("linear_buckets + power_2_buckets");
      test_bucket_options<lin_incr_t>("linear_buckets + incremental");
      test_bucket_options<fmod_t>("fastmod_buckets");
      test_bucket_options<lin_fmod_t>("linear_buckets + fastmod_buckets");
   }

   //////////////////////////////////////
   // Options that change how the container uses the bucket array when it
   // is rehashed into a new one: the cached first bucket, a size that is
   // not stored (reserve_additional calls size()), the stored hash and the
   // groups of equivalent elements
   //////////////////////////////////////
   {
      typedef bi::unordered_multiset
         < item, bi::hash<item_hash>, bi::cache_begin<true>
         , bi::constant_time_size<false> >                         cache_t;
      typedef bi::unordered_multiset
         < item, bi::hash<item_hash>, bi::cache_begin<true>
         , bi::constant_time_size<false>, bi::linear_buckets<true> > lin_cache_t;
      typedef bi::unordered_multiset
         < grouped_item, bi::hash<grouped_item_hash> >               grouped_t;
      typedef bi::unordered_multiset
         < grouped_item, bi::hash<grouped_item_hash>, bi::linear_buckets<true>
         , bi::fastmod_buckets<true>, bi::cache_begin<true> >        lin_fmod_grouped_t;
      typedef bi::unordered_multiset
         < grouped_item, bi::hash<grouped_item_hash>
         , bi::incremental<true> >                                   incr_grouped_t;

      test_bucket_options<cache_t>("cache_begin + !constant_time_size");
      test_bucket_options<lin_cache_t>("cache_begin + !constant_time_size + linear_buckets");
      test_bucket_options<grouped_t>("store_hash + optimize_multikey");
      test_bucket_options<lin_fmod_grouped_t>("store_hash + optimize_multikey + linear_buckets + fastmod_buckets + cache_begin");
      test_bucket_options<incr_grouped_t>("store_hash + optimize_multikey + incremental");
   }

   //////////////////////////////////////
   // A bucket count for the requested capacity that does not fit in
   // size_type is limited to the maximum value of size_type
   //////////////////////////////////////
   {
      typedef bi::unordered_multiset
         < item, bi::hash<item_hash>, bi::size_type<unsigned short> > small_t;
      typedef bi::unordered_bucket_manager<small_t>                  small_mgr_t;

      small_mgr_t mgr;
      small_t     mset(mgr.traits());
      mgr.max_load_factor(0.001f);
      //1000 elements need 1000000 buckets, more than unsigned short can hold
      BOOST_TEST(mgr.reserve(mset, small_t::size_type(1000u)));
      BOOST_TEST(mgr.bucket_count() == small_t::size_type(-1));
      check_sync(mgr, mset);
      item x(1);
      mset.insert(x);
      BOOST_TEST(mset.find(item(1)) != mset.end());
      mset.clear();
   }

   std::printf("All tests passed.\n");
   return boost::report_errors();
}
