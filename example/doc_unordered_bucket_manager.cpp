/////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Ion Gaztanaga  2026-2026
//
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/intrusive for documentation.
//
/////////////////////////////////////////////////////////////////////////////
//[doc_unordered_bucket_manager
#include <boost/intrusive/unordered_set.hpp>
#include <boost/intrusive/unordered_bucket_manager.hpp>
#include <cassert>
#include <vector>

using namespace boost::intrusive;

//A class to be inserted in an unordered_set
class MyClass : public unordered_set_base_hook<>
{
   int int_;

   public:
   MyClass(int i = 0) : int_(i)
   {}

   friend bool operator==(const MyClass &l, const MyClass &r)
      {  return l.int_ == r.int_;   }
   friend std::size_t hash_value(const MyClass &v)
   {  //Use your favorite hash function, like boost::hash or std::hash
      return std::size_t(v.int_);
   }
};

typedef unordered_set<MyClass>            Uset;

//The manager owns the bucket array of a single container
typedef unordered_bucket_manager<Uset>    Manager;

int main()
{
   //The values are still supplied by the user, as in any intrusive container
   std::vector<MyClass> values;
   for(int i = 0; i < 100; ++i)  values.push_back(MyClass(i));

   //Declare the manager BEFORE the container, so that it is destroyed after
   //it, and build the container from the managed bucket array
   Manager mgr;
   Uset    uset(mgr.traits());

   //Insertions are performed on the container. Ask the manager for room
   //first, so that the maximum load factor is respected.
   mgr.reserve_additional(uset, Uset::size_type(values.size()));
   for(std::size_t i = 0; i != values.size(); ++i)
      uset.insert(values[i]);

   //Lookups don't change the load factor: no manager needed
   assert(uset.find(MyClass(50)) != uset.end());

   //Erasures don't need the manager either, they never resize the bucket array
   uset.erase(MyClass(50));
   assert(uset.find(MyClass(50)) == uset.end());

   //Releasing buckets is always explicit. This call replaces the array with
   //the smallest one that fits the current size
   uset.clear();
   mgr.shrink_to_fit(uset);

   //The buckets are empty now, so the manager can be safely destroyed
   return 0;
}
//]
