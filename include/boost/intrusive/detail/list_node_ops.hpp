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

#ifndef BOOST_INTRUSIVE_DETAIL_LIST_NODE_OPS_HPP
#define BOOST_INTRUSIVE_DETAIL_LIST_NODE_OPS_HPP

#ifndef BOOST_CONFIG_HPP
#  include <boost/config.hpp>
#endif

#if defined(BOOST_HAS_PRAGMA_ONCE)
#  pragma once
#endif

#include <boost/intrusive/detail/workaround.hpp>
#include <boost/intrusive/detail/assert.hpp>
#include <boost/intrusive/circular_list_algorithms.hpp>

namespace boost {
namespace intrusive {
namespace detail {

//Operations of list_impl that only depend on the node type, the size holder
//and the link mode (not on ValueTraits), so that they are instantiated only once
//for all lists that share the same node type.
template<class NodeTraits, class SizeTraits, bool SafeModeOrAutoUnlink>
struct list_node_ops
{
   typedef circular_list_algorithms<NodeTraits>    node_algorithms;
   typedef typename NodeTraits::node_ptr           node_ptr;
   typedef typename NodeTraits::const_node_ptr     const_node_ptr;
   typedef typename SizeTraits::size_type          size_type;
   static const bool constant_time_size = SizeTraits::constant_time_size;

   static size_type size(const_node_ptr header, const SizeTraits &sz) BOOST_NOEXCEPT
   {
      BOOST_IF_CONSTEXPR(constant_time_size)
         return sz.get_size();
      else
         return size_type(node_algorithms::count(header) - 1u);
   }

   //Unlinks n and initializes it, if needed
   static void erase(node_ptr n, SizeTraits &sz) BOOST_NOEXCEPT
   {
      node_algorithms::unlink(n);
      sz.decrement();
      BOOST_IF_CONSTEXPR(SafeModeOrAutoUnlink)
         node_algorithms::init(n);
   }

   //Unlinks [b, e) and initializes the nodes, if needed
   static void erase(node_ptr b, node_ptr e, SizeTraits &sz) BOOST_NOEXCEPT
   {
      node_algorithms::unlink(b, e);
      BOOST_IF_CONSTEXPR(SafeModeOrAutoUnlink || constant_time_size){
         while(b != e){
            node_ptr to_erase(b);
            b = NodeTraits::get_next(b);
            BOOST_IF_CONSTEXPR(SafeModeOrAutoUnlink)
               node_algorithms::init(to_erase);
            sz.decrement();
         }
      }
   }

   //Unlinks [b, e), with n == distance(b, e), and initializes the nodes, if needed
   static void erase(node_ptr b, node_ptr e, size_type n, SizeTraits &sz) BOOST_NOEXCEPT
   {
      BOOST_INTRUSIVE_INVARIANT_ASSERT(node_algorithms::distance(b, e) == n);
      BOOST_IF_CONSTEXPR(SafeModeOrAutoUnlink){
         list_node_ops::erase(b, e, sz);
      }
      else{
         sz.decrease(n);
         node_algorithms::unlink(b, e);
      }
   }

   static void clear(node_ptr header, SizeTraits &sz) BOOST_NOEXCEPT
   {
      BOOST_IF_CONSTEXPR(SafeModeOrAutoUnlink){
         node_ptr p(NodeTraits::get_next(header));
         while(p != header){
            node_ptr to_erase(p);
            p = NodeTraits::get_next(p);
            node_algorithms::init(to_erase);
         }
      }
      node_algorithms::init_header(header);
      sz.set_size(size_type(0));
   }

   //Transfers all the nodes of the list with header x before p
   static void splice_all(node_ptr p, SizeTraits &sz, node_ptr x, SizeTraits &xsz) BOOST_NOEXCEPT
   {
      if(!node_algorithms::unique(x)){
         node_algorithms::transfer(p, NodeTraits::get_next(x), x);
         sz.increase(xsz.get_size());
         xsz.set_size(size_type(0));
      }
   }

   //Transfers node i before p
   static void splice_one(node_ptr p, SizeTraits &sz, node_ptr i, SizeTraits &xsz) BOOST_NOEXCEPT
   {
      node_algorithms::transfer(p, i);
      xsz.decrement();
      sz.increment();
   }

   //Transfers [f, e) before p, with n == distance(f, e) if constant_time_size
   static void splice_range(node_ptr p, SizeTraits &sz, node_ptr f, node_ptr e, size_type n, SizeTraits &xsz) BOOST_NOEXCEPT
   {
      if(n){
         BOOST_IF_CONSTEXPR(constant_time_size){
            BOOST_INTRUSIVE_INVARIANT_ASSERT(n == node_algorithms::distance(f, e));
            node_algorithms::transfer(p, f, e);
            sz.increase(n);
            xsz.decrease(n);
         }
         else{
            node_algorithms::transfer(p, f, e);
         }
      }
   }

   //Transfers [f, e) before p
   static void splice_range(node_ptr p, SizeTraits &sz, node_ptr f, node_ptr e, SizeTraits &xsz) BOOST_NOEXCEPT
   {
      BOOST_IF_CONSTEXPR(constant_time_size)
         list_node_ops::splice_range(p, sz, f, e, size_type(node_algorithms::distance(f, e)), xsz);
      else
         list_node_ops::splice_range(p, sz, f, e, size_type(1), xsz);//distance is a dummy value
   }

   static void transfer_all_size(SizeTraits &sz, SizeTraits &xsz) BOOST_NOEXCEPT
   {
      sz.increase(xsz.get_size());
      xsz.set_size(size_type(0));
   }

   static void check(const_node_ptr header_ptr, const SizeTraits &sz)
   {
      (void)sz;
      BOOST_INTRUSIVE_INVARIANT_ASSERT(NodeTraits::get_next(header_ptr));
      BOOST_INTRUSIVE_INVARIANT_ASSERT(NodeTraits::get_previous(header_ptr));
      BOOST_INTRUSIVE_INVARIANT_ASSERT((NodeTraits::get_next(header_ptr) == header_ptr)
         == (NodeTraits::get_previous(header_ptr) == header_ptr));
      if (NodeTraits::get_next(header_ptr) == header_ptr)
      {
         BOOST_IF_CONSTEXPR(constant_time_size)
            BOOST_INTRUSIVE_INVARIANT_ASSERT(sz.get_size() == 0);
         return;
      }
      size_type node_count = 0; (void)node_count;
      const_node_ptr p = header_ptr;
      while (true)
      {
         const_node_ptr next_p = NodeTraits::get_next(p);
         BOOST_INTRUSIVE_INVARIANT_ASSERT(next_p);
         BOOST_INTRUSIVE_INVARIANT_ASSERT(NodeTraits::get_previous(next_p) == p);
         p = next_p;
         if (p == header_ptr) break;
         ++node_count;
      }
      BOOST_IF_CONSTEXPR(constant_time_size)
         BOOST_INTRUSIVE_INVARIANT_ASSERT(sz.get_size() == node_count);
   }
};

}  //namespace detail{
}  //namespace intrusive{
}  //namespace boost{

#endif //BOOST_INTRUSIVE_DETAIL_LIST_NODE_OPS_HPP
