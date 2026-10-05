/////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Ion Gaztanaga  2007-2013
//
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/intrusive for documentation.
//
/////////////////////////////////////////////////////////////////////////////
//[doc_bucket_traits
#include <boost/intrusive/unordered_set.hpp>
#include <boost/move/utility_core.hpp>
#include <vector>
#include <cassert>

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

//Define the base hook option
typedef base_hook< unordered_set_base_hook<> >     BaseHookOption;

//Obtain the types of the bucket and the bucket pointer
typedef unordered_bucket<BaseHookOption>::type     BucketType;
typedef unordered_bucket_ptr<BaseHookOption>::type BucketPtr;

//The custom bucket traits. A container that is moved keeps using the bucket
//array, so the move operations must leave the moved-from bucket traits without it
//(otherwise, the moved-from container would share the bucket array with the new one).
class custom_bucket_traits
{
   //<-
   BOOST_COPYABLE_AND_MOVABLE(custom_bucket_traits)
   //->

   public:
   static const int NumBuckets = 100;

   custom_bucket_traits(BucketPtr buckets)
      :  buckets_(buckets)
   {}

   custom_bucket_traits(const custom_bucket_traits &x)
      :  buckets_(x.buckets_)
   {}

   custom_bucket_traits(BOOST_RV_REF(custom_bucket_traits) x)
      :  buckets_(x.buckets_)
   {  x.buckets_ = BucketPtr();  }

   custom_bucket_traits& operator=(BOOST_COPY_ASSIGN_REF(custom_bucket_traits) x)
   {  buckets_ = x.buckets_;  return *this;  }

   custom_bucket_traits& operator=(BOOST_RV_REF(custom_bucket_traits) x)
   {  buckets_ = x.buckets_;  x.buckets_ = BucketPtr();  return *this;  }

   //Functions to be implemented by custom bucket traits
   BucketPtr   bucket_begin() const {  return buckets_;  }
   std::size_t bucket_count() const {  return buckets_ ? NumBuckets : 0;  }

   private:
   BucketPtr buckets_;
};

//Define the container using the custom bucket traits
typedef unordered_set<MyClass, bucket_traits<custom_bucket_traits> > BucketTraitsUset;

int main()
{
   typedef std::vector<MyClass>::iterator VectIt;
   std::vector<MyClass> values;

   //Fill values
   for(int i = 0; i < 100; ++i)  values.push_back(MyClass(i));

   //Now create the bucket array and the custom bucket traits object
   BucketType buckets[custom_bucket_traits::NumBuckets];
   custom_bucket_traits btraits(buckets);

   //Now create the unordered set
   BucketTraitsUset uset(btraits);

   //Insert the values in the unordered set
   for(VectIt it(values.begin()), itend(values.end()); it != itend; ++it)
      uset.insert(*it);

   //Move the container: the moved-from bucket traits no longer refer to the bucket array
   BucketTraitsUset uset2(boost::move(uset));
   assert(uset.bucket_count() == 0 && uset2.size() == values.size());

   return 0;
}
//]
