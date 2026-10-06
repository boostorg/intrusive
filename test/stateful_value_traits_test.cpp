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
#include <boost/intrusive/list.hpp>
#include <boost/intrusive/slist.hpp>
#include <boost/intrusive/set.hpp>
#include <boost/intrusive/unordered_set.hpp>
#include <boost/intrusive/pointer_traits.hpp>
#include <boost/move/utility_core.hpp>
#include <vector>
#include <functional>
#include <cstddef>

using namespace boost::intrusive;

class MyClass
{
   public:
   int int_;

   MyClass(int i = 0)
      :  int_(i)
   {}

   friend bool operator<(const MyClass &l, const MyClass &r)
   {  return l.int_ < r.int_; }

   friend bool operator==(const MyClass &l, const MyClass &r)
   {  return l.int_ == r.int_; }

   friend std::size_t hash_value(const MyClass &v)
   {  return std::size_t(v.int_); }
};

template<class T, class NodeTraits>
struct stateful_value_traits
{
   typedef NodeTraits                                          node_traits;
   typedef typename node_traits::node                          node;
   typedef typename node_traits::node_ptr                      node_ptr;
   typedef typename node_traits::const_node_ptr                const_node_ptr;
   typedef T                                                   value_type;
   typedef typename pointer_traits<node_ptr>::
      template rebind_pointer<T>::type                         pointer;
   typedef typename pointer_traits<node_ptr>::
      template rebind_pointer<const T>::type                   const_pointer;

   static const link_mode_type link_mode = normal_link;

   stateful_value_traits(pointer vals, node_ptr node_array)
      :  values_(vals),  node_array_(node_array)
   {}

   node_ptr to_node_ptr (value_type &value) const
   {  return node_array_ + (&value - values_); }

   const_node_ptr to_node_ptr (const value_type &value) const
   {  return node_array_ + (&value - values_); }

   pointer to_value_ptr(node_ptr n) const
   {  return values_ + (n - node_array_); }

   const_pointer to_value_ptr(const_node_ptr n) const
   {  return values_ + (n - node_array_); }

   pointer  values_;
   node_ptr node_array_;
};

//Define a list that will store MyClass using the external hook
typedef stateful_value_traits< MyClass, list_node_traits<void*> > list_traits;
typedef list<MyClass, value_traits<list_traits> > List;

//Define a slist that will store MyClass using the external hook
typedef stateful_value_traits< MyClass, slist_node_traits<void*> > slist_traits;
typedef slist<MyClass, value_traits<slist_traits> > Slist;

//Define a set that will store MyClass using the external hook
typedef stateful_value_traits< MyClass, rbtree_node_traits<void*> > rbtree_traits;
typedef set<MyClass, value_traits<rbtree_traits> > Set;

//uset uses the same traits as slist
typedef unordered_set<MyClass, value_traits<slist_traits> > Uset;


typedef list_traits::node     list_node_t;
typedef slist_traits::node    slist_node_t;
typedef rbtree_traits::node   rbtree_node_t;

const int NumElements = 100;

MyClass        values    [NumElements];
list_node_t    list_hook_array   [NumElements];
slist_node_t   slist_hook_array  [NumElements];
rbtree_node_t  rbtree_hook_array [NumElements];
slist_node_t   uset_hook_array   [NumElements];

//To test swap and move assignment between containers with different value traits
const std::size_t SwapElements = 10;

//A node of one container converted to a value with the value traits of the other
//container is not an element of values1 or values2
template<class Node>
struct swap_data
{
   swap_data()
   {
      for(std::size_t i = 0; i < SwapElements; ++i){
         values1[i].int_ = int(i);
         values2[i].int_ = int(SwapElements + i);
      }
   }

   MyClass values1[SwapElements];
   MyClass values2[SwapElements];
   Node    hooks2 [SwapElements];
   Node    hooks1 [SwapElements];
};

swap_data<list_node_t>     list_swap_data;
swap_data<slist_node_t>    slist_swap_data;
swap_data<rbtree_node_t>   rbtree_swap_data;
swap_data<slist_node_t>    uset_swap_data;

void insert_value(List &c, MyClass &v)    {  c.push_back(v);  }
void insert_value(Slist &c, MyClass &v)   {  c.push_front(v);  }
void insert_value(Set &c, MyClass &v)     {  c.insert(v);  }
void insert_value(Uset &c, MyClass &v)    {  c.insert(v);  }

//Checks that the container holds all the elements of "vals" and no other element
template<class Container>
bool holds_values(const Container &c, const MyClass *vals)
{
   std::less<const MyClass*> less;
   std::size_t count = 0;
   for(typename Container::const_iterator it(c.begin()), itend(c.end()); it != itend; ++it, ++count){
      if(count == SwapElements || less(&*it, vals) || !less(&*it, vals + SwapElements))
         return false;
   }
   return count == SwapElements;
}

//Swap and move assignment must swap also the value traits
//and move construction must take the value traits of the source
template<class Container, class Node>
bool test_swap(Container &a, Container &b, swap_data<Node> &d)
{
   for(std::size_t i = 0; i < SwapElements; ++i){
      insert_value(a, d.values1[i]);
      insert_value(b, d.values2[i]);
   }
   a.swap(b);
   if(!holds_values(a, d.values2) || !holds_values(b, d.values1))
      return false;
   a = boost::move(b);
   if(!holds_values(a, d.values1) || !holds_values(b, d.values2))
      return false;
   Container c(boost::move(a));
   if(!holds_values(c, d.values1) || !a.empty())
      return false;
   c.clear();
   b.clear();
   return true;
}

int main()
{
   //Create several MyClass objects, each one with a different value
   for(int i = 0; i < NumElements; ++i)
      values[i].int_ = i;

   Uset::bucket_type buckets[NumElements];

   List  my_list (list_traits (values, list_hook_array));
   Slist my_slist(slist_traits(values, slist_hook_array));
   Set   my_set  (std::less<MyClass>(), rbtree_traits(values, rbtree_hook_array));
   Uset  my_uset ( Uset::bucket_traits(buckets, NumElements)
                 , Uset::hasher()
                 , Uset::key_equal()
                 , slist_traits(values, uset_hook_array)
                 );

   //Now insert them in containers
   for(MyClass * it(&values[0]), *itend(&values[NumElements])
      ; it != itend
      ; ++it){
      my_list.push_front(*it);
      if(&*my_list.iterator_to(*it) != &my_list.front())
         return 1;
      my_slist.push_front(*it);
      if(&*my_slist.iterator_to(*it) != &my_slist.front())
         return 1;
      Set::iterator sit = my_set.insert(*it).first;
      if(&*my_set.iterator_to(*it) != &*sit)
         return 1;
      Uset::iterator uit = my_uset.insert(*it).first;
      my_uset.insert(*it);
      if(&*my_uset.iterator_to(*it) != &*uit)
         return 1;
   }

   //Now test lists
   {
      List::const_iterator   list_it (my_list.cbegin());
      Slist::const_iterator  slist_it(my_slist.cbegin());
      Set::const_reverse_iterator set_rit(my_set.crbegin());
      MyClass *it_val(&values[NumElements]), *it_rbeg_val(&values[0]);

      //Test the objects inserted in the base hook list
      for(; it_val != it_rbeg_val; --it_val, ++list_it, ++slist_it, ++set_rit){
         if(&*list_it  != &it_val[-1])   return 1;
         if(&*slist_it != &it_val[-1])   return 1;
         if(&*set_rit  != &it_val[-1])   return 1;
         if(my_uset.find(it_val[-1]) == my_uset.cend())  return 1;
      }
   }

   //Now test swap and move assignment
   {
      swap_data<list_node_t> &ld = list_swap_data;
      List l1(list_traits(ld.values1, ld.hooks1));
      List l2(list_traits(ld.values2, ld.hooks2));
      if(!test_swap(l1, l2, ld))  return 1;

      swap_data<slist_node_t> &sld = slist_swap_data;
      Slist sl1(slist_traits(sld.values1, sld.hooks1));
      Slist sl2(slist_traits(sld.values2, sld.hooks2));
      if(!test_swap(sl1, sl2, sld))  return 1;

      swap_data<rbtree_node_t> &sd = rbtree_swap_data;
      Set s1(std::less<MyClass>(), rbtree_traits(sd.values1, sd.hooks1));
      Set s2(std::less<MyClass>(), rbtree_traits(sd.values2, sd.hooks2));
      if(!test_swap(s1, s2, sd))  return 1;

      swap_data<slist_node_t> &ud = uset_swap_data;
      Uset::bucket_type buckets1[SwapElements];
      Uset::bucket_type buckets2[SwapElements];
      Uset u1( Uset::bucket_traits(buckets1, SwapElements), Uset::hasher(), Uset::key_equal()
             , slist_traits(ud.values1, ud.hooks1));
      Uset u2( Uset::bucket_traits(buckets2, SwapElements), Uset::hasher(), Uset::key_equal()
             , slist_traits(ud.values2, ud.hooks2));
      if(!test_swap(u1, u2, ud))  return 1;
   }

   return 0;
}
