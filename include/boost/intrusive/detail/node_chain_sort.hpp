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

//Stable merge and bottom-up merge sort for chains of nodes
//linked through "next" pointers (NodeTraits::get_next/set_next).
template<class NodeTraits>
struct node_chain_sort
{
   typedef typename NodeTraits::node_ptr node_ptr;

   //Linking policies: "link(a, b)" makes b the node that follows a
   struct next_linker
   {
      BOOST_INTRUSIVE_FORCEINLINE static void link(node_ptr a, node_ptr b)
      {  NodeTraits::set_next(a, b);  }
   };

   //Same as next_linker, but also updates the "previous" pointer of b
   struct next_prev_linker
   {
      static void link(node_ptr a, node_ptr b)
      {
         NodeTraits::set_next(a, b);
         NodeTraits::set_previous(b, a);
      }
   };

   //A null-terminated sub-chain of nodes, from "head" to "tail". Empty if head is null.
   struct chain
   {
      //Does not initialize the members, so arrays of chains cost nothing to create
      BOOST_INTRUSIVE_FORCEINLINE chain() {}
      BOOST_INTRUSIVE_FORCEINLINE chain(node_ptr h, node_ptr t) : head(h), tail(t) {}

      BOOST_INTRUSIVE_FORCEINLINE bool empty() const
      {  return head == node_ptr();  }

      BOOST_INTRUSIVE_FORCEINLINE void clear()
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

   //Returns the last node of the chain that starts in "first" and ends before "end".
   //"hint" is the last node, if known (otherwise, a null pointer)
   static node_ptr find_last(node_ptr first, const node_ptr end, const node_ptr hint)
   {
      if(hint != node_ptr())
         return hint;
      while(NodeTraits::get_next(first) != end)
         first = NodeTraits::get_next(first);
      return first;
   }

   //Merges two sorted non-empty chains: "a" is [pa, ea) and "b" is [pb, eb). The end
   //markers are not part of the chains, and can be null. Nodes of "a" go first on ties.
   //"a_tail" and "b_tail" are the last nodes of the chains, if known (otherwise, null pointers).
   //
   //Returns in "head" the first node of the merged chain, that ends with "ea". "last_b" is
   //the last node that came from "b". "tail" is the last node of the merged chain or null
   //if it is unknown.
   //
   //If "comp" throws, all the nodes are still linked in a chain (in unspecified order)
   //that starts in "head", ends in "tail" and is terminated by "ea".
   template<class Linker, class NodePtrCompare>
   static void merge_nodes
      ( node_ptr pa, node_ptr pb, const node_ptr a_tail, const node_ptr b_tail
      , const node_ptr ea, const node_ptr eb, NodePtrCompare comp
      , node_ptr &head, node_ptr &tail, node_ptr &last_b)
   {
      node_ptr t = node_ptr();
      head = node_ptr();
      //Invariant before each comparison: t is the last node taken, and t's next node is
      //the current node of the chain "t" came from (pb if in_b, pa otherwise). That
      //way next pointers are only written when the merge switches from one chain
      //to the other, and not for every node.
      bool in_b = false;
      BOOST_INTRUSIVE_TRY{
         //A comparison can only throw when both pa and pb are valid nodes
         in_b = comp(pb, pa);
         //Alternate between taking runs of nodes from "b" and from "a". Each run starts
         //with a node whose comparison result is already known.
         head = in_b ? pb : pa;
         for(;;){
            if(in_b){
               do{
                  t  = pb;
                  pb = NodeTraits::get_next(pb);
               }while(pb != eb && comp(pb, pa));
               if(pb == eb)
                  break;
               Linker::link(t, pa);
               in_b = false;
            }
            else{
               do{
                  t  = pa;
                  pa = NodeTraits::get_next(pa);
               }while(pa != ea && !comp(pb, pa));
               if(pa == ea)
                  break;
               Linker::link(t, pb);
               in_b = true;
            }
         }
      }
      BOOST_INTRUSIVE_CATCH(...){
         //pa and pb are valid nodes here: join [head, t], the rest of both chains
         const node_ptr last_a = find_last(pa, ea, a_tail);
         const node_ptr lb     = find_last(pb, eb, b_tail);
         if(in_b){
            Linker::link(lb, pa);
            tail = last_a;
         }
         else{
            if(head == node_ptr())
               head = pa;
            Linker::link(last_a, pb);
            if(eb != ea)
               Linker::link(lb, ea);
            tail = lb;
         }
         BOOST_INTRUSIVE_RETHROW;
      }
      BOOST_INTRUSIVE_CATCH_END
      if(pa == ea){
         //The last node taken came from "a", and the rest of "b" is appended
         Linker::link(t, pb);
         last_b = find_last(pb, eb, b_tail);
         if(eb != ea)
            Linker::link(last_b, ea);
         tail = last_b;
      }
      else{
         //The last node taken came from "b", and the rest of "a" (that ends with "ea") is appended
         Linker::link(t, pa);
         last_b = t;
         tail   = a_tail;
      }
   }

   //Merges "a" (elements that go first on ties) and "b" into "res".
   //Elements of both chains are always in "res", even if "comp" throws.
   //"res" can be the same object as "a" or "b".
   template<class NodePtrCompare>
   static void merge(chain &res, const chain a, const chain b, NodePtrCompare comp)
   {
      node_ptr head = node_ptr(), tail = node_ptr(), last_b = node_ptr();
      BOOST_INTRUSIVE_TRY{
         merge_nodes<next_linker>
            (a.head, b.head, a.tail, b.tail, node_ptr(), node_ptr(), comp, head, tail, last_b);
      }
      BOOST_INTRUSIVE_CATCH(...){
         res.head = head;
         res.tail = tail;
         BOOST_INTRUSIVE_RETHROW;
      }
      BOOST_INTRUSIVE_CATCH_END
      res.head = head;
      res.tail = tail;
   }

   //Stable sort of the nodes first, next(first), ... until "end" is reached (not included).
   //
   //Output: a null-terminated chain [out_head, out_tail] that contains all input nodes.
   //
   //If the comparison throws, "out_head"/"out_tail" hold a valid null-terminated chain
   //with all the nodes (in unspecified order) before the exception is rethrown,
   //so the caller can restore the container.
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
            //Take two nodes at a time and order them directly: this avoids
            //half of the merge calls, that are the most expensive for short chains
            const node_ptr n1 = cur;
            cur = NodeTraits::get_next(n1);
            if(cur == end){
               //A single node is left, it is the newest element
               NodeTraits::set_next(n1, node_ptr());
               carry = chain(n1, n1);
               break;
            }
            const node_ptr n2 = cur;
            cur = NodeTraits::get_next(n2);
            NodeTraits::set_next(n2, node_ptr());
            carry = chain(n1, n2);
            if(comp(n2, n1)){
               NodeTraits::set_next(n2, n1);
               NodeTraits::set_next(n1, node_ptr());
               carry = chain(n2, n1);
            }
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
