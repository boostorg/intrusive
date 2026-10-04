/////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Ion Gaztanaga  2025-2025
//
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/intrusive for documentation.
//
/////////////////////////////////////////////////////////////////////////////

#ifndef BOOST_INTRUSIVE_DETAIL_NODE_CHAIN_SORT_HPP
#define BOOST_INTRUSIVE_DETAIL_NODE_CHAIN_SORT_HPP

#ifndef BOOST_CONFIG_HPP
#  include <boost/config.hpp>
#endif

#if defined(BOOST_HAS_PRAGMA_ONCE)
#  pragma once
#endif

#include <boost/intrusive/detail/config_begin.hpp>
#include <boost/intrusive/detail/workaround.hpp>
#include <cstddef>

namespace boost {
namespace intrusive {
namespace detail {

//Stable bottom-up merge sort for chains of nodes linked through
//"next" pointers only (NodeTraits::get_next/set_next).
//
//Input:  nodes first, next(first), ... until "end" is reached (not included).
//Output: a null-terminated chain [head, tail] that contains all input nodes.
//
//If the comparison throws, "head"/"tail" hold a valid null-terminated chain
//with all the nodes (in unspecified order) before the exception is rethrown,
//so the caller can restore the container.
template<class NodeTraits>
struct node_chain_sort
{
   typedef typename NodeTraits::node_ptr node_ptr;

   //A null-terminated sub-chain of nodes, from "head" to "tail". Empty if head is null.
   struct chain
   {
      //Does not initialize the members, so arrays of chains cost nothing to create
      chain() {}
      chain(node_ptr h, node_ptr t) : head(h), tail(t) {}

      bool empty() const
      {  return head == node_ptr();  }

      void clear()
      {  head = node_ptr();  }

      //Links "other" after the last node of this chain
      void append(const chain &other)
      {
         if(!other.empty()){
            if(this->empty()){
               *this = other;
            }
            else{
               NodeTraits::set_next(tail, other.head);
               tail = other.tail;
            }
         }
      }

      node_ptr head;
      node_ptr tail;
   };

   static const std::size_t max_bins = 64u;

   //Merges "a" (elements that go first on ties) and "b" into "res".
   //Elements of both chains are always in "res", even if "comp" throws.
   //"res" can be the same object as "a" or "b".
   template<class NodePtrCompare>
   static void merge(chain &res, const chain a, const chain b, NodePtrCompare comp)
   {
      node_ptr pa = a.head, pb = b.head;
      node_ptr head = node_ptr(), t = node_ptr();
      BOOST_INTRUSIVE_TRY{
         //A comparison can only throw when both pa and pb are not null
         if(comp(pb, pa)){
            head = pb;
            pb   = NodeTraits::get_next(pb);
         }
         else{
            head = pa;
            pa   = NodeTraits::get_next(pa);
         }
         t = head;
         while(pa != node_ptr() && pb != node_ptr()){
            if(comp(pb, pa)){
               NodeTraits::set_next(t, pb);
               t  = pb;
               pb = NodeTraits::get_next(pb);
            }
            else{
               NodeTraits::set_next(t, pa);
               t  = pa;
               pa = NodeTraits::get_next(pa);
            }
         }
      }
      BOOST_INTRUSIVE_CATCH(...){
         //pa and pb are not null here. Join: merged part, rest of a, rest of b
         NodeTraits::set_next(a.tail, pb);
         if(head != node_ptr()){
            NodeTraits::set_next(t, pa);
            res.head = head;
         }
         else{
            res.head = pa;
         }
         res.tail = b.tail;
         BOOST_INTRUSIVE_RETHROW;
      }
      BOOST_INTRUSIVE_CATCH_END
      if(pa == node_ptr()){
         NodeTraits::set_next(t, pb);
         res.tail = b.tail;
      }
      else{
         NodeTraits::set_next(t, pa);
         res.tail = a.tail;
      }
      res.head = head;
   }

   template<class NodePtrCompare>
   static void sort(node_ptr cur, const node_ptr end, NodePtrCompare comp, node_ptr &out_head, node_ptr &out_tail)
   {
      //bins[i] with i < fill holds a sorted chain of 2^i nodes or is empty (head == null).
      //Bins with i >= fill are not initialized (they are set when "fill" grows).
      //Higher bins hold elements that were in the original chain before
      //lower bins, which is needed to keep the sort stable.
      chain bins[max_bins];
      chain carry((node_ptr()), (node_ptr()));
      std::size_t fill = 0;
      BOOST_INTRUSIVE_TRY{
         while(cur != end){
            const node_ptr n = cur;
            cur = NodeTraits::get_next(n);
            NodeTraits::set_next(n, node_ptr());
            carry = chain(n, n);
            std::size_t i = 0;
            for(; i < fill && !bins[i].empty(); ++i){
               const chain older = bins[i];
               bins[i].clear();
               merge(carry, older, carry, comp);
            }
            bins[i] = carry;
            carry.clear();
            if(i == fill)
               ++fill;
         }
         for(std::size_t i = 0; i != fill; ++i){
            if(bins[i].empty())
               continue;
            if(carry.empty()){
               carry = bins[i];
               bins[i].clear();
            }
            else{
               const chain older = bins[i];
               bins[i].clear();
               merge(carry, older, carry, comp);
            }
         }
      }
      BOOST_INTRUSIVE_CATCH(...){
         //Gather every node in a single chain
         chain all((node_ptr()), (node_ptr()));
         for(std::size_t i = 0; i != fill; ++i)
            all.append(bins[i]);
         all.append(carry);
         if(cur != end){
            node_ptr last = cur;
            for(node_ptr nxt = NodeTraits::get_next(last); nxt != end; nxt = NodeTraits::get_next(last))
               last = nxt;
            NodeTraits::set_next(last, node_ptr());
            chain rest(cur, last);
            all.append(rest);
         }
         out_head = all.head;
         out_tail = all.tail;
         BOOST_INTRUSIVE_RETHROW;
      }
      BOOST_INTRUSIVE_CATCH_END
      out_head = carry.head;
      out_tail = carry.tail;
   }
};

}  //namespace detail
}  //namespace intrusive
}  //namespace boost

#include <boost/intrusive/detail/config_end.hpp>

#endif //BOOST_INTRUSIVE_DETAIL_NODE_CHAIN_SORT_HPP
