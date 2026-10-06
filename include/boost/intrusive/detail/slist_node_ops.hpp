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

#ifndef BOOST_INTRUSIVE_DETAIL_SLIST_NODE_OPS_HPP
#define BOOST_INTRUSIVE_DETAIL_SLIST_NODE_OPS_HPP

#ifndef BOOST_CONFIG_HPP
#  include <boost/config.hpp>
#endif

#if defined(BOOST_HAS_PRAGMA_ONCE)
#  pragma once
#endif

#include <boost/intrusive/detail/workaround.hpp>
#include <boost/intrusive/detail/assert.hpp>
#include <boost/intrusive/detail/mpl.hpp>
#include <boost/intrusive/detail/simple_disposers.hpp>
#include <boost/intrusive/circular_slist_algorithms.hpp>
#include <boost/intrusive/linear_slist_algorithms.hpp>
#include <cstddef>

namespace boost {
namespace intrusive {
namespace detail {

//Operations of slist_impl that only depend on the node type, the size holder,
//the linear/cache_last options and the link mode (not on ValueTraits), so that
//they are instantiated only once for all slists that share the same node type.
//
//"header" is the header node of the list, "plast" points to the cached last node
//(it's null if CacheLast is false) and "sz" is the size holder.
template<class NodeTraits, class SizeTraits, bool Linear, bool CacheLast, bool SafeModeOrAutoUnlink>
struct slist_node_ops
{
   typedef typename detail::if_c
      < Linear
      , linear_slist_algorithms<NodeTraits>
      , circular_slist_algorithms<NodeTraits>
      >::type                                      node_algorithms;
   typedef typename NodeTraits::node_ptr           node_ptr;
   typedef typename NodeTraits::const_node_ptr     const_node_ptr;
   typedef typename SizeTraits::size_type          size_type;
   static const bool constant_time_size = SizeTraits::constant_time_size;

   static void init(node_ptr header, node_ptr *plast, SizeTraits &sz) BOOST_NOEXCEPT
   {
      (void)plast;
      node_algorithms::init_header(header);
      sz.set_size(size_type(0));
      BOOST_IF_CONSTEXPR(CacheLast){
         *plast = header;
      }
   }

   static size_type size(const_node_ptr header, const SizeTraits &sz) BOOST_NOEXCEPT
   {
      (void)header; (void)sz;
      BOOST_IF_CONSTEXPR(constant_time_size)
         return sz.get_size();
      else
         return size_type(node_algorithms::count(header) - 1u);
   }

   static void clear(node_ptr header, node_ptr *plast, SizeTraits &sz) BOOST_NOEXCEPT
   {
      BOOST_IF_CONSTEXPR(SafeModeOrAutoUnlink){
         node_algorithms::detach_and_dispose(header, detail::init_disposer<node_algorithms>());
      }
      slist_node_ops::init(header, plast, sz);
   }

   //Unlinks the node after prev_n and initializes it, if needed
   static void erase_after(node_ptr prev_n, node_ptr *plast, SizeTraits &sz) BOOST_NOEXCEPT
   {
      (void)plast;
      node_ptr const to_erase(NodeTraits::get_next(prev_n));
      node_algorithms::unlink_after(prev_n);
      BOOST_IF_CONSTEXPR(CacheLast){
         if(to_erase == *plast){
            *plast = prev_n;
         }
      }
      BOOST_IF_CONSTEXPR(SafeModeOrAutoUnlink)
         node_algorithms::init(to_erase);
      sz.decrement();
   }

   //Unlinks (bfp, lp) and initializes the nodes, if needed
   static void erase_after(node_ptr bfp, node_ptr lp, node_ptr header, node_ptr *plast, SizeTraits &sz) BOOST_NOEXCEPT
   {
      (void)header; (void)plast;
      BOOST_IF_CONSTEXPR(CacheLast){
         if(lp == node_algorithms::end_node(header)){
            *plast = bfp;
         }
      }
      node_ptr fp(NodeTraits::get_next(bfp));
      node_algorithms::unlink_after(bfp, lp);
      BOOST_IF_CONSTEXPR(SafeModeOrAutoUnlink || constant_time_size){
         while(fp != lp){
            node_ptr to_erase(fp);
            fp = NodeTraits::get_next(fp);
            BOOST_IF_CONSTEXPR(SafeModeOrAutoUnlink)
               node_algorithms::init(to_erase);
            sz.decrement();
         }
      }
   }

   //Unlinks (bfp, lp), with n == distance(next(bfp), lp), and initializes the nodes, if needed
   static void erase_after(node_ptr bfp, node_ptr lp, size_type n, node_ptr header, node_ptr *plast, SizeTraits &sz) BOOST_NOEXCEPT
   {
      (void)n; (void)header; (void)plast;
      BOOST_INTRUSIVE_INVARIANT_ASSERT(node_algorithms::distance(NodeTraits::get_next(bfp), lp) == n);
      BOOST_IF_CONSTEXPR(SafeModeOrAutoUnlink){
         slist_node_ops::erase_after(bfp, lp, header, plast, sz);
      }
      else{
         BOOST_IF_CONSTEXPR(CacheLast){
            if(lp == node_algorithms::end_node(header)){
               *plast = bfp;
            }
         }
         node_algorithms::unlink_after(bfp, lp);
         sz.decrease(n);
      }
   }

   //Transfers all the nodes of the list x (non-empty) after prev_n,
   //returns the previous last node of x
   static node_ptr splice_after_all
      ( node_ptr prev_n, node_ptr header, node_ptr *plast, SizeTraits &sz
      , node_ptr xheader, node_ptr *xplast, SizeTraits &xsz) BOOST_NOEXCEPT
   {
      (void)header; (void)plast; (void)xplast;
      node_ptr last_x_n;
      BOOST_IF_CONSTEXPR(CacheLast){
         last_x_n = *xplast;
         *xplast = xheader;
         if(NodeTraits::get_next(prev_n) == node_algorithms::end_node(header)){
            *plast = last_x_n;
         }
      }
      else{
         last_x_n = node_algorithms::get_previous_node(xheader, node_algorithms::end_node(xheader));
      }
      node_algorithms::transfer_after(prev_n, xheader, last_x_n);
      sz.increase(xsz.get_size());
      xsz.set_size(size_type(0));
      return last_x_n;
   }

   //Transfers (before_f_n, before_l_n] from x after prev_pos_n
   static void splice_after
      ( node_ptr prev_pos_n, node_ptr header, node_ptr *plast
      , node_ptr before_f_n, node_ptr before_l_n, node_ptr xheader, node_ptr *xplast) BOOST_NOEXCEPT
   {
      (void)header; (void)plast; (void)xheader; (void)xplast;
      node_algorithms::transfer_after(prev_pos_n, before_f_n, before_l_n);
      BOOST_IF_CONSTEXPR(CacheLast){
         if(before_f_n != before_l_n){
            if(NodeTraits::get_next(before_l_n) == node_algorithms::end_node(header)){
               *plast = before_l_n;
            }
            if(NodeTraits::get_next(before_f_n) == node_algorithms::end_node(xheader)){
               *xplast = before_f_n;
            }
         }
      }
   }

   //Transfers (before_f_n, before_l_n] from x after prev_pos_n, n == distance(before_f_n, before_l_n)
   static void splice_after
      ( node_ptr prev_pos_n, node_ptr header, node_ptr *plast, SizeTraits &sz
      , node_ptr before_f_n, node_ptr before_l_n, size_type n
      , node_ptr xheader, node_ptr *xplast, SizeTraits &xsz) BOOST_NOEXCEPT
   {
      (void)n;
      BOOST_INTRUSIVE_INVARIANT_ASSERT(node_algorithms::distance(before_f_n, before_l_n) == n);
      slist_node_ops::splice_after(prev_pos_n, header, plast, before_f_n, before_l_n, xheader, xplast);
      sz.increase(n);
      xsz.decrease(n);
   }

   //Transfers (before_f_n, before_l_n] from x after prev_pos_n
   static void splice_after
      ( node_ptr prev_pos_n, node_ptr header, node_ptr *plast, SizeTraits &sz
      , node_ptr before_f_n, node_ptr before_l_n
      , node_ptr xheader, node_ptr *xplast, SizeTraits &xsz) BOOST_NOEXCEPT
   {
      BOOST_IF_CONSTEXPR(constant_time_size)
         slist_node_ops::splice_after
            ( prev_pos_n, header, plast, sz, before_f_n, before_l_n
            , size_type(node_algorithms::distance(before_f_n, before_l_n)), xheader, xplast, xsz);
      else
         slist_node_ops::splice_after(prev_pos_n, header, plast, before_f_n, before_l_n, xheader, xplast);
   }

   //Links [first_n, before_l_n] after prev_pos_n
   static void incorporate_after(node_ptr prev_pos_n, node_ptr first_n, node_ptr before_l_n, node_ptr *plast) BOOST_NOEXCEPT
   {
      (void)plast;
      BOOST_IF_CONSTEXPR(CacheLast){
         if(prev_pos_n == *plast){
            *plast = before_l_n;
         }
      }
      node_algorithms::incorporate_after(prev_pos_n, first_n, before_l_n);
   }

   //Links [first_n, before_l_n] after prev_pos_n, n == distance(first_n, before_l_n) + 1
   static void incorporate_after(node_ptr prev_pos_n, node_ptr first_n, node_ptr before_l_n, size_type n, node_ptr *plast, SizeTraits &sz) BOOST_NOEXCEPT
   {
      if(n){
         BOOST_INTRUSIVE_INVARIANT_ASSERT(size_type(node_algorithms::distance(first_n, before_l_n) + 1u) == n);
         slist_node_ops::incorporate_after(prev_pos_n, first_n, before_l_n, plast);
         sz.increase(n);
      }
   }

   //Links [first_n, before_l_n] after prev_pos_n
   static void incorporate_after(node_ptr prev_pos_n, node_ptr first_n, node_ptr before_l_n, node_ptr *plast, SizeTraits &sz) BOOST_NOEXCEPT
   {
      BOOST_IF_CONSTEXPR(constant_time_size)
         slist_node_ops::incorporate_after
            (prev_pos_n, first_n, before_l_n, size_type(node_algorithms::distance(first_n, before_l_n) + 1u), plast, sz);
      else
         slist_node_ops::incorporate_after(prev_pos_n, first_n, before_l_n, plast);
   }

   static void reverse(node_ptr header, node_ptr *plast) BOOST_NOEXCEPT
   {
      (void)plast;
      BOOST_IF_CONSTEXPR(CacheLast){
         if(!node_algorithms::is_empty(header)){
            *plast = NodeTraits::get_next(header);
         }
      }
      slist_node_ops::priv_reverse(header, detail::bool_<Linear>());
   }

   static void shift_backwards(node_ptr header, node_ptr *plast, std::size_t n) BOOST_NOEXCEPT
   {
      slist_node_ops::priv_shift_backwards(header, plast, n, detail::bool_<Linear>());
   }

   static void shift_forward(node_ptr header, node_ptr *plast, std::size_t n) BOOST_NOEXCEPT
   {
      slist_node_ops::priv_shift_forward(header, plast, n, detail::bool_<Linear>());
   }

   //Swaps the nodes of both lists
   static void swap(node_ptr header, node_ptr *plast, node_ptr xheader, node_ptr *xplast) BOOST_NOEXCEPT
   {
      (void)plast; (void)xplast;
      BOOST_IF_CONSTEXPR(CacheLast){
         slist_node_ops::priv_swap_cache_last(header, plast, xheader, xplast);
      }
      else{
         slist_node_ops::priv_swap_lists(header, xheader, detail::bool_<Linear>());
      }
   }

   //Updates sizes and the cached last node of x after transferring all its nodes
   static void transfer_all(SizeTraits &sz, node_ptr xheader, node_ptr *xplast, SizeTraits &xsz) BOOST_NOEXCEPT
   {
      (void)xheader; (void)xplast;
      sz.increase(xsz.get_size());
      xsz.set_size(size_type(0));
      BOOST_IF_CONSTEXPR(CacheLast){
         *xplast = xheader;
      }
   }

   static void update_last_node(node_ptr header, node_ptr *plast) BOOST_NOEXCEPT
   {
      const node_ptr end_node = node_algorithms::end_node(header);
      node_ptr l = header;
      for(node_ptr n = NodeTraits::get_next(l); n != end_node; n = NodeTraits::get_next(n)){
         l = n;
      }
      *plast = l;
   }

   static void check(const_node_ptr header_ptr, const_node_ptr last, const SizeTraits &sz)
   {
      (void)last; (void)sz;
      const_node_ptr const first_p = NodeTraits::get_next(header_ptr);
      BOOST_INTRUSIVE_INVARIANT_ASSERT(Linear || first_p);
      if (first_p == (Linear ? const_node_ptr() : header_ptr))
      {
         BOOST_INTRUSIVE_INVARIANT_ASSERT(!constant_time_size || sz.get_size() == 0);
         return;
      }
      size_type node_count = 0; (void)node_count;
      const_node_ptr p = header_ptr;
      while (true)
      {
         const_node_ptr next_p = NodeTraits::get_next(p);
         BOOST_IF_CONSTEXPR(!Linear)
         {
            BOOST_INTRUSIVE_INVARIANT_ASSERT(next_p);
         }
         else
         {
            BOOST_INTRUSIVE_INVARIANT_ASSERT(next_p != header_ptr);
         }
         if ((!Linear && next_p == header_ptr) || (Linear && !next_p))
         {
            BOOST_INTRUSIVE_INVARIANT_ASSERT(!CacheLast || last == p);
            break;
         }
         p = next_p;
         ++node_count;
      }
      BOOST_INTRUSIVE_INVARIANT_ASSERT(!constant_time_size || sz.get_size() == node_count);
   }

   private:
   static void priv_reverse(node_ptr header, detail::bool_<false>)
   {  node_algorithms::reverse(header);   }

   static void priv_reverse(node_ptr header, detail::bool_<true>)
   {
      node_ptr new_first = node_algorithms::reverse(NodeTraits::get_next(header));
      NodeTraits::set_next(header, new_first);
   }

   static void priv_shift_backwards(node_ptr header, node_ptr *plast, std::size_t n, detail::bool_<false>)
   {
      (void)plast;
      node_ptr l = node_algorithms::move_forward(header, n);
      (void)l;
      BOOST_IF_CONSTEXPR(CacheLast){
         if(l){
            *plast = l;
         }
      }
   }

   static void priv_shift_backwards(node_ptr header, node_ptr *plast, std::size_t n, detail::bool_<true>)
   {
      (void)plast;
      typename node_algorithms::node_pair ret(
         node_algorithms::move_first_n_forward(NodeTraits::get_next(header), n));
      if(ret.first){
         NodeTraits::set_next(header, ret.first);
         BOOST_IF_CONSTEXPR(CacheLast){
            *plast = ret.second;
         }
      }
   }

   static void priv_shift_forward(node_ptr header, node_ptr *plast, std::size_t n, detail::bool_<false>)
   {
      (void)plast;
      node_ptr l = node_algorithms::move_backwards(header, n);
      (void)l;
      BOOST_IF_CONSTEXPR(CacheLast){
         if(l){
            *plast = l;
         }
      }
   }

   static void priv_shift_forward(node_ptr header, node_ptr *plast, std::size_t n, detail::bool_<true>)
   {
      (void)plast;
      typename node_algorithms::node_pair ret(
         node_algorithms::move_first_n_backwards(NodeTraits::get_next(header), n));
      if(ret.first){
         NodeTraits::set_next(header, ret.first);
         BOOST_IF_CONSTEXPR(CacheLast){
            *plast = ret.second;
         }
      }
   }

   static void priv_swap_cache_last(node_ptr this_bfirst, node_ptr *this_plast, node_ptr other_bfirst, node_ptr *other_plast)
   {
      bool other_was_empty = false;
      if(node_algorithms::is_empty(this_bfirst)){
         //Check if both are empty or
         if(node_algorithms::is_empty(other_bfirst))
            return;
         //If this is empty swap pointers
         node_ptr tmp_bfirst = this_bfirst;
         node_ptr *tmp_plast = this_plast;
         this_bfirst = other_bfirst;
         this_plast  = other_plast;
         other_bfirst = tmp_bfirst;
         other_plast  = tmp_plast;
         other_was_empty = true;
      }
      else{
         other_was_empty = node_algorithms::is_empty(other_bfirst);
      }

      //Precondition: this is not empty
      node_ptr other_old_last(*other_plast);
      node_ptr this_old_last(*this_plast);

      //Move all nodes from this to other's beginning
      node_algorithms::transfer_after(other_bfirst, this_bfirst, this_old_last);
      *other_plast = this_old_last;

      if(other_was_empty){
         *this_plast = this_bfirst;
      }
      else{
         //Move trailing nodes from other to this
         node_algorithms::transfer_after(this_bfirst, this_old_last, other_old_last);
         *this_plast = other_old_last;
      }
   }

   static void priv_swap_lists(node_ptr this_node, node_ptr other_node, detail::bool_<false>)
   {  node_algorithms::swap_nodes(this_node, other_node); }

   static void priv_swap_lists(node_ptr this_node, node_ptr other_node, detail::bool_<true>)
   {  node_algorithms::swap_trailing_nodes(this_node, other_node); }
};

}  //namespace detail{
}  //namespace intrusive{
}  //namespace boost{

#endif //BOOST_INTRUSIVE_DETAIL_SLIST_NODE_OPS_HPP
