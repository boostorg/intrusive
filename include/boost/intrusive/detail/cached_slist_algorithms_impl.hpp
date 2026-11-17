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

#ifndef BOOST_INTRUSIVE_DETAIL_CACHED_SLIST_ALGORITHMS_IMPL_HPP
#define BOOST_INTRUSIVE_DETAIL_CACHED_SLIST_ALGORITHMS_IMPL_HPP

#ifndef BOOST_CONFIG_HPP
#  include <boost/config.hpp>
#endif

#if defined(BOOST_HAS_PRAGMA_ONCE)
#  pragma once
#endif

#include <boost/intrusive/detail/config_begin.hpp>
#include <boost/intrusive/detail/workaround.hpp>
#include <boost/intrusive/detail/node_chain_sort.hpp>
#include <cstddef>

namespace boost {
namespace intrusive {
namespace detail {

//Operations on a whole list, identified by its header node, with the same interface
//for linear and circular lists. "Algo" is linear_slist_algorithms or circular_slist_algorithms.
//Algo is a private base, so that the functions of Algo with the same interface are made
//public with using declarations.
template<class Algo, bool Linear>
struct slist_header_algorithms;

template<class Algo>
struct slist_header_algorithms<Algo, true>
   : private Algo
{
   typedef typename Algo::node_traits  node_traits;
   typedef typename Algo::node_ptr     node_ptr;

   static void reverse(node_ptr header) BOOST_NOEXCEPT
   {  node_traits::set_next(header, Algo::reverse(node_traits::get_next(header)));  }

   //Returns the new last node, or null if there is no movement
   static node_ptr shift_backwards(node_ptr header, std::size_t n) BOOST_NOEXCEPT
   {
      typename Algo::node_pair ret(Algo::move_first_n_forward(node_traits::get_next(header), n));
      if(ret.first){
         node_traits::set_next(header, ret.first);
      }
      return ret.second;
   }

   //Returns the new last node, or null if there is no movement
   static node_ptr shift_forward(node_ptr header, std::size_t n) BOOST_NOEXCEPT
   {
      typename Algo::node_pair ret(Algo::move_first_n_backwards(node_traits::get_next(header), n));
      if(ret.first){
         node_traits::set_next(header, ret.first);
      }
      return ret.second;
   }

   using Algo::swap_trailing_nodes;
};

template<class Algo>
struct slist_header_algorithms<Algo, false>
   : private Algo
{
   typedef typename Algo::node_ptr     node_ptr;

   using Algo::reverse;

   //Returns the new last node, or null if there is no movement
   BOOST_INTRUSIVE_FORCEINLINE static node_ptr shift_backwards(node_ptr header, std::size_t n) BOOST_NOEXCEPT
   {  return Algo::move_forward(header, n);  }

   //Returns the new last node, or null if there is no movement
   BOOST_INTRUSIVE_FORCEINLINE static node_ptr shift_forward(node_ptr header, std::size_t n) BOOST_NOEXCEPT
   {  return Algo::move_backwards(header, n);  }

   BOOST_INTRUSIVE_FORCEINLINE static void swap_trailing_nodes(node_ptr header, node_ptr other_header) BOOST_NOEXCEPT
   {  Algo::swap_nodes(header, other_header);  }
};

//Implementation of cached_linear_slist_algorithms and cached_circular_slist_algorithms.
//"Algo" is linear_slist_algorithms or circular_slist_algorithms. Functions that change the links
//take a reference to the cached last node of the list ("last"), that is the header node if the list
//is empty, and update it.
//
//Node pointers are taken by value and not as BOOST_INTRUSIVE_CPARAM, as the reference to the
//cached last node is written and a node pointer argument could be the same object.
//
//Algo is a protected base so that functions that do not update the cached last node are not
//available. Functions that do not change the links are made public with using declarations.
template<class Algo, bool Linear>
class cached_slist_algorithms_impl
   : protected Algo
{
   typedef Algo                                    base_t;
   typedef slist_header_algorithms<Algo, Linear>   header_algo;

   public:
   typedef typename Algo::node            node;
   typedef typename Algo::node_ptr        node_ptr;
   typedef typename Algo::const_node_ptr  const_node_ptr;
   typedef typename Algo::node_traits     node_traits;

   private:
   typedef node_chain_sort<node_traits>   chain_sort;

   public:
   using base_t::init;
   using base_t::unique;
   using base_t::inited;
   using base_t::end_node;
   using base_t::is_empty;
   using base_t::get_previous_node;
   using base_t::count;

   static void init_header(node_ptr header, node_ptr &last) BOOST_NOEXCEPT
   {
      base_t::init_header(header);
      last = header;
   }

   static void link_after(node_ptr prev_node, node_ptr this_node, node_ptr &last) BOOST_NOEXCEPT
   {
      base_t::link_after(prev_node, this_node);
      if(prev_node == last){
         last = this_node;
      }
   }

   static void unlink_after(node_ptr prev_node, node_ptr &last) BOOST_NOEXCEPT
   {
      if(node_traits::get_next(prev_node) == last){
         last = prev_node;
      }
      base_t::unlink_after(prev_node);
   }

   static void unlink_after(node_ptr prev_node, node_ptr e, node_ptr &last) BOOST_NOEXCEPT
   {
      //"e" is the end of the list only if it follows the last node
      if(node_traits::get_next(last) == e){
         last = prev_node;
      }
      base_t::unlink_after(prev_node, e);
   }

   static void incorporate_after(node_ptr bp, node_ptr b, node_ptr be, node_ptr &last) BOOST_NOEXCEPT
   {
      base_t::incorporate_after(bp, b, be);
      if(bp == last){
         last = be;
      }
   }

   static void transfer_after(node_ptr p, node_ptr b, node_ptr e, node_ptr &p_last, node_ptr &b_last) BOOST_NOEXCEPT
   {
      if(p != b && p != e && b != e){
         //p_last and b_last can be the same object if both ranges are in the same list
         const bool p_is_last = p == p_last;
         const bool e_is_last = e == b_last;
         base_t::transfer_after(p, b, e);
         if(e_is_last){
            b_last = b;
         }
         if(p_is_last){
            p_last = e;
         }
      }
   }

   static void transfer_after(node_ptr p, node_ptr other, node_ptr &p_last, node_ptr &other_last) BOOST_NOEXCEPT
   {
      const node_ptr old_other_last = other_last;
      if(old_other_last != other){
         if(p == p_last){
            p_last = old_other_last;
         }
         other_last = other;
         base_t::transfer_after(p, other, old_other_last);
      }
   }

   static void swap_trailing_nodes
      (node_ptr this_node, node_ptr other_node, node_ptr &this_last, node_ptr &other_last) BOOST_NOEXCEPT
   {
      if(base_t::is_empty(this_node)){
         if(!base_t::is_empty(other_node)){
            cached_slist_algorithms_impl::priv_swap_non_empty(other_node, other_last, this_node, this_last, true);
         }
      }
      else{
         cached_slist_algorithms_impl::priv_swap_non_empty
            (this_node, this_last, other_node, other_last, base_t::is_empty(other_node));
      }
   }

   static void reverse(node_ptr header, node_ptr &last) BOOST_NOEXCEPT
   {
      if(!base_t::is_empty(header)){
         last = node_traits::get_next(header);
      }
      header_algo::reverse(header);
   }

   static void shift_backwards(node_ptr header, std::size_t n, node_ptr &last) BOOST_NOEXCEPT
   {
      const node_ptr l = header_algo::shift_backwards(header, n);
      if(l){
         last = l;
      }
   }

   static void shift_forward(node_ptr header, std::size_t n, node_ptr &last) BOOST_NOEXCEPT
   {
      const node_ptr l = header_algo::shift_forward(header, n);
      if(l){
         last = l;
      }
   }

   template<class NodePtrCompare>
   BOOST_INTRUSIVE_FORCEINLINE static void sort(node_ptr header, NodePtrCompare comp, node_ptr &last)
   {  chain_sort::sort_after(header, base_t::end_node(header), comp, last);  }

   template<class NodePtrCompare>
   static node_ptr merge(node_ptr p, node_ptr x, NodePtrCompare comp, node_ptr &p_last, node_ptr &x_last)
   {
      //All the nodes of x's list are transferred, also if "comp" throws
      const node_ptr old_x_last = x_last;
      x_last = x;
      return chain_sort::merge_after
         (p, base_t::end_node(p), p_last, x, base_t::end_node(x), old_x_last, comp, p_last);
   }

   template<class Disposer>
   static std::size_t detach_and_dispose(node_ptr header, Disposer disposer, node_ptr &last) BOOST_NOEXCEPT
   {
      last = header;
      return base_t::detach_and_dispose(header, disposer);
   }

   private:
   //Precondition: this_node's list is not empty
   static void priv_swap_non_empty
      (node_ptr this_node, node_ptr &this_last, node_ptr other_node, node_ptr &other_last, bool other_was_empty) BOOST_NOEXCEPT
   {
      const node_ptr other_old_last(other_last);
      const node_ptr this_old_last(this_last);

      //Move all nodes from this to other's beginning
      base_t::transfer_after(other_node, this_node, this_old_last);
      other_last = this_old_last;

      if(other_was_empty){
         this_last = this_node;
      }
      else{
         //Move trailing nodes from other to this
         base_t::transfer_after(this_node, this_old_last, other_old_last);
         this_last = other_old_last;
      }
   }
};

}  //namespace detail
}  //namespace intrusive
}  //namespace boost

#include <boost/intrusive/detail/config_end.hpp>

#endif //BOOST_INTRUSIVE_DETAIL_CACHED_SLIST_ALGORITHMS_IMPL_HPP
