/////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Olaf Krzikalla 2004-2006.
// (C) Copyright Ion Gaztanaga  2006-2014
//
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/intrusive for documentation.
//
/////////////////////////////////////////////////////////////////////////////

#ifndef BOOST_INTRUSIVE_SLIST_HPP
#define BOOST_INTRUSIVE_SLIST_HPP

#include <boost/intrusive/detail/config_begin.hpp>
#include <boost/intrusive/detail/workaround.hpp>
#include <boost/intrusive/intrusive_fwd.hpp>

#include <boost/intrusive/detail/assert.hpp>
#include <boost/intrusive/slist_hook.hpp>
#include <boost/intrusive/circular_slist_algorithms.hpp>
#include <boost/intrusive/linear_slist_algorithms.hpp>
#include <boost/intrusive/cached_circular_slist_algorithms.hpp>
#include <boost/intrusive/cached_linear_slist_algorithms.hpp>
#include <boost/intrusive/pointer_traits.hpp>
#include <boost/intrusive/link_mode.hpp>
#include <boost/intrusive/detail/get_value_traits.hpp>
#include <boost/intrusive/detail/is_stateful_value_traits.hpp>
#include <boost/intrusive/detail/default_header_holder.hpp>
#include <boost/intrusive/detail/uncast.hpp>
#include <boost/intrusive/detail/mpl.hpp>
#include <boost/intrusive/detail/iterator.hpp>
#include <boost/intrusive/detail/slist_iterator.hpp>
#include <boost/intrusive/detail/exception_disposer.hpp>
#include <boost/intrusive/detail/equal_to_value.hpp>
#include <boost/intrusive/detail/key_nodeptr_comp.hpp>
#include <boost/intrusive/detail/simple_disposers.hpp>
#include <boost/intrusive/detail/size_holder.hpp>
#include <boost/intrusive/detail/parent_from_member.hpp>
#include <boost/intrusive/detail/value_traits_holder.hpp>
#include <boost/intrusive/detail/algorithm.hpp>
#include <boost/intrusive/detail/value_functors.hpp>
#include <boost/intrusive/detail/node_cloner_disposer.hpp>

#include <boost/move/utility_core.hpp>
#include <boost/move/adl_move_swap.hpp>

#include <cstddef>   //std::size_t

#if defined(BOOST_HAS_PRAGMA_ONCE)
#  pragma once
#endif

namespace boost {
namespace intrusive {

/// @cond

namespace detail {

//Operations on a list, identified by its header node, with the same interface if the
//last node is cached or not. "plast" points to the cached last node of the list if
//CacheLast is true, and it is null and ignored if CacheLast is false.
template<class NodeTraits, bool Linear, bool CacheLast>
struct slist_last_algorithms;

template<class NodeTraits, bool Linear>
struct slist_last_algorithms<NodeTraits, Linear, true>
{
   typedef typename detail::if_c
      < Linear
      , cached_linear_slist_algorithms<NodeTraits>
      , cached_circular_slist_algorithms<NodeTraits>
      >::type                                      cached_algorithms;
   typedef typename detail::if_c
      < Linear
      , linear_slist_algorithms<NodeTraits>
      , circular_slist_algorithms<NodeTraits>
      >::type                                      node_algorithms;
   typedef typename NodeTraits::node_ptr           node_ptr;
   typedef typename node_algorithms::stable_partition_info stable_partition_info;

   BOOST_INTRUSIVE_FORCEINLINE static void init_header(node_ptr header, node_ptr *plast) BOOST_NOEXCEPT
   {  cached_algorithms::init_header(header, *plast);  }

   BOOST_INTRUSIVE_FORCEINLINE static void link_after(node_ptr prev_n, node_ptr n, node_ptr *plast) BOOST_NOEXCEPT
   {  cached_algorithms::link_after(prev_n, n, *plast);  }

   BOOST_INTRUSIVE_FORCEINLINE static void unlink_after(node_ptr prev_n, node_ptr *plast) BOOST_NOEXCEPT
   {  cached_algorithms::unlink_after(prev_n, *plast);  }

   BOOST_INTRUSIVE_FORCEINLINE static void unlink_after(node_ptr prev_n, node_ptr e, node_ptr *plast) BOOST_NOEXCEPT
   {  cached_algorithms::unlink_after(prev_n, e, *plast);  }

   BOOST_INTRUSIVE_FORCEINLINE static void incorporate_after(node_ptr bp, node_ptr b, node_ptr be, node_ptr *plast) BOOST_NOEXCEPT
   {  cached_algorithms::incorporate_after(bp, b, be, *plast);  }

   BOOST_INTRUSIVE_FORCEINLINE static void transfer_after
      (node_ptr p, node_ptr b, node_ptr e, node_ptr *plast, node_ptr *bplast) BOOST_NOEXCEPT
   {  cached_algorithms::transfer_after(p, b, e, *plast, *bplast);  }

   //Transfers all the nodes of the list x (non-empty) after p, returns the previous last node of x
   static node_ptr transfer_all_after(node_ptr p, node_ptr x, node_ptr *plast, node_ptr *xplast) BOOST_NOEXCEPT
   {
      const node_ptr last_x = *xplast;
      cached_algorithms::transfer_after(p, x, *plast, *xplast);
      return last_x;
   }

   BOOST_INTRUSIVE_FORCEINLINE static void swap_trailing_nodes
      (node_ptr header, node_ptr xheader, node_ptr *plast, node_ptr *xplast) BOOST_NOEXCEPT
   {  cached_algorithms::swap_trailing_nodes(header, xheader, *plast, *xplast);  }

   BOOST_INTRUSIVE_FORCEINLINE static void reverse(node_ptr header, node_ptr *plast) BOOST_NOEXCEPT
   {  cached_algorithms::reverse(header, *plast);  }

   BOOST_INTRUSIVE_FORCEINLINE static void shift_backwards(node_ptr header, std::size_t n, node_ptr *plast) BOOST_NOEXCEPT
   {  cached_algorithms::shift_backwards(header, n, *plast);  }

   BOOST_INTRUSIVE_FORCEINLINE static void shift_forward(node_ptr header, std::size_t n, node_ptr *plast) BOOST_NOEXCEPT
   {  cached_algorithms::shift_forward(header, n, *plast);  }

   template<class NodePtrCompare>
   BOOST_INTRUSIVE_FORCEINLINE static void sort(node_ptr header, NodePtrCompare comp, node_ptr *plast)
   {  cached_algorithms::sort(header, comp, *plast);  }

   template<class NodePtrCompare>
   BOOST_INTRUSIVE_FORCEINLINE static node_ptr merge(node_ptr p, node_ptr x, NodePtrCompare comp, node_ptr *plast, node_ptr *xplast)
   {  return cached_algorithms::merge(p, x, comp, *plast, *xplast);  }

   //Moves the nodes for which pred is true before the others, keeping the relative order
   template<class Pred>
   static void stable_partition(node_ptr header, Pred pred, stable_partition_info &info, node_ptr *plast)
   {
      node_algorithms::stable_partition(header, node_algorithms::end_node(header), pred, info);
      *plast = info.new_last_node;
   }
};

template<class NodeTraits, bool Linear>
struct slist_last_algorithms<NodeTraits, Linear, false>
{
   typedef typename detail::if_c
      < Linear
      , linear_slist_algorithms<NodeTraits>
      , circular_slist_algorithms<NodeTraits>
      >::type                                      node_algorithms;
   typedef slist_header_algorithms<node_algorithms, Linear>  header_algorithms;
   typedef typename NodeTraits::node_ptr           node_ptr;
   typedef typename node_algorithms::stable_partition_info stable_partition_info;

   BOOST_INTRUSIVE_FORCEINLINE static void init_header(node_ptr header, node_ptr *) BOOST_NOEXCEPT
   {  node_algorithms::init_header(header);  }

   BOOST_INTRUSIVE_FORCEINLINE static void link_after(node_ptr prev_n, node_ptr n, node_ptr *) BOOST_NOEXCEPT
   {  node_algorithms::link_after(prev_n, n);  }

   BOOST_INTRUSIVE_FORCEINLINE static void unlink_after(node_ptr prev_n, node_ptr *) BOOST_NOEXCEPT
   {  node_algorithms::unlink_after(prev_n);  }

   BOOST_INTRUSIVE_FORCEINLINE static void unlink_after(node_ptr prev_n, node_ptr e, node_ptr *) BOOST_NOEXCEPT
   {  node_algorithms::unlink_after(prev_n, e);  }

   BOOST_INTRUSIVE_FORCEINLINE static void incorporate_after(node_ptr bp, node_ptr b, node_ptr be, node_ptr *) BOOST_NOEXCEPT
   {  node_algorithms::incorporate_after(bp, b, be);  }

   BOOST_INTRUSIVE_FORCEINLINE static void transfer_after
      (node_ptr p, node_ptr b, node_ptr e, node_ptr *, node_ptr *) BOOST_NOEXCEPT
   {  node_algorithms::transfer_after(p, b, e);  }

   //Transfers all the nodes of the list x (non-empty) after p, returns the previous last node of x
   static node_ptr transfer_all_after(node_ptr p, node_ptr x, node_ptr *, node_ptr *) BOOST_NOEXCEPT
   {
      const node_ptr last_x = node_algorithms::get_previous_node(x, node_algorithms::end_node(x));
      node_algorithms::transfer_after(p, x, last_x);
      return last_x;
   }

   BOOST_INTRUSIVE_FORCEINLINE static void swap_trailing_nodes
      (node_ptr header, node_ptr xheader, node_ptr *, node_ptr *) BOOST_NOEXCEPT
   {  header_algorithms::swap_trailing_nodes(header, xheader);  }

   BOOST_INTRUSIVE_FORCEINLINE static void reverse(node_ptr header, node_ptr *) BOOST_NOEXCEPT
   {  header_algorithms::reverse(header);  }

   BOOST_INTRUSIVE_FORCEINLINE static void shift_backwards(node_ptr header, std::size_t n, node_ptr *) BOOST_NOEXCEPT
   {  (void)header_algorithms::shift_backwards(header, n);  }

   BOOST_INTRUSIVE_FORCEINLINE static void shift_forward(node_ptr header, std::size_t n, node_ptr *) BOOST_NOEXCEPT
   {  (void)header_algorithms::shift_forward(header, n);  }

   template<class NodePtrCompare>
   BOOST_INTRUSIVE_FORCEINLINE static void sort(node_ptr header, NodePtrCompare comp, node_ptr *)
   {  (void)node_algorithms::sort(header, comp);  }

   template<class NodePtrCompare>
   BOOST_INTRUSIVE_FORCEINLINE static node_ptr merge(node_ptr p, node_ptr x, NodePtrCompare comp, node_ptr *, node_ptr *)
   {  return node_algorithms::merge(p, x, comp);  }

   //Moves the nodes for which pred is true before the others, keeping the relative order
   template<class Pred>
   BOOST_INTRUSIVE_FORCEINLINE static void stable_partition(node_ptr header, Pred pred, stable_partition_info &info, node_ptr *)
   {  node_algorithms::stable_partition(header, node_algorithms::end_node(header), pred, info);  }
};

//Operations of slist_impl that only depend on the node type, the size holder,
//the linear/cache_last options and the link mode
//"header" is the header node of the list, "plast" points to the cached last node
//(it's null if CacheLast is false) and "sz" is the size holder.
template<class NodeTraits, class SizeTraits, bool Linear, bool CacheLast, bool SafeModeOrAutoUnlink>
struct slist_node_ops
{
   typedef slist_last_algorithms<NodeTraits, Linear, CacheLast>   last_algorithms;
   typedef typename last_algorithms::node_algorithms  node_algorithms;
   typedef typename NodeTraits::node_ptr           node_ptr;
   typedef typename NodeTraits::const_node_ptr     const_node_ptr;
   typedef typename SizeTraits::size_type          size_type;
   static const bool constant_time_size = SizeTraits::constant_time_size;

   static void init(node_ptr header, node_ptr *plast, SizeTraits &sz) BOOST_NOEXCEPT
   {
      last_algorithms::init_header(header, plast);
      sz.set_size(size_type(0));
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
      node_ptr const to_erase(NodeTraits::get_next(prev_n));
      (void)to_erase;
      last_algorithms::unlink_after(prev_n, plast);
      BOOST_IF_CONSTEXPR(SafeModeOrAutoUnlink)
         node_algorithms::init(to_erase);
      sz.decrement();
   }

   //Unlinks (bfp, lp) and initializes the nodes, if needed
   static void erase_after(node_ptr bfp, node_ptr lp, node_ptr *plast, SizeTraits &sz) BOOST_NOEXCEPT
   {
      (void)sz;
      node_ptr fp(NodeTraits::get_next(bfp));
      last_algorithms::unlink_after(bfp, lp, plast);
      BOOST_IF_CONSTEXPR(SafeModeOrAutoUnlink || constant_time_size){
         while(fp != lp){
            node_ptr to_erase(fp);
            (void)to_erase;
            fp = NodeTraits::get_next(fp);
            BOOST_IF_CONSTEXPR(SafeModeOrAutoUnlink)
               node_algorithms::init(to_erase);
            sz.decrement();
         }
      }
   }

   //Unlinks (bfp, lp), with n == distance(next(bfp), lp), and initializes the nodes, if needed
   static void erase_after(node_ptr bfp, node_ptr lp, size_type n, node_ptr *plast, SizeTraits &sz) BOOST_NOEXCEPT
   {
      (void)n;
      BOOST_INTRUSIVE_INVARIANT_ASSERT(node_algorithms::distance(NodeTraits::get_next(bfp), lp) == n);
      BOOST_IF_CONSTEXPR(SafeModeOrAutoUnlink){
         slist_node_ops::erase_after(bfp, lp, plast, sz);
      }
      else{
         last_algorithms::unlink_after(bfp, lp, plast);
         sz.decrease(n);
      }
   }

   //Transfers all the nodes of the list x (non-empty) after prev_n,
   //returns the previous last node of x
   static node_ptr splice_after_all
      ( node_ptr prev_n, node_ptr *plast, SizeTraits &sz
      , node_ptr xheader, node_ptr *xplast, SizeTraits &xsz) BOOST_NOEXCEPT
   {
      const node_ptr last_x_n = last_algorithms::transfer_all_after(prev_n, xheader, plast, xplast);
      sz.increase(xsz.get_size());
      xsz.set_size(size_type(0));
      return last_x_n;
   }

   //Transfers (before_f_n, before_l_n] from x after prev_pos_n
   BOOST_INTRUSIVE_FORCEINLINE static void splice_after
      ( node_ptr prev_pos_n, node_ptr *plast
      , node_ptr before_f_n, node_ptr before_l_n, node_ptr *xplast) BOOST_NOEXCEPT
   {  last_algorithms::transfer_after(prev_pos_n, before_f_n, before_l_n, plast, xplast);  }

   //Transfers (before_f_n, before_l_n] from x after prev_pos_n, n == distance(before_f_n, before_l_n)
   static void splice_after
      ( node_ptr prev_pos_n, node_ptr *plast, SizeTraits &sz
      , node_ptr before_f_n, node_ptr before_l_n, size_type n
      , node_ptr *xplast, SizeTraits &xsz) BOOST_NOEXCEPT
   {
      (void)n;
      BOOST_INTRUSIVE_INVARIANT_ASSERT(node_algorithms::distance(before_f_n, before_l_n) == n);
      slist_node_ops::splice_after(prev_pos_n, plast, before_f_n, before_l_n, xplast);
      sz.increase(n);
      xsz.decrease(n);
   }

   //Transfers (before_f_n, before_l_n] from x after prev_pos_n
   static void splice_after
      ( node_ptr prev_pos_n, node_ptr *plast, SizeTraits &sz
      , node_ptr before_f_n, node_ptr before_l_n
      , node_ptr *xplast, SizeTraits &xsz) BOOST_NOEXCEPT
   {
      (void)sz; (void)xsz;
      BOOST_IF_CONSTEXPR(constant_time_size)
         slist_node_ops::splice_after
            ( prev_pos_n, plast, sz, before_f_n, before_l_n
            , size_type(node_algorithms::distance(before_f_n, before_l_n)), xplast, xsz);
      else
         slist_node_ops::splice_after(prev_pos_n, plast, before_f_n, before_l_n, xplast);
   }

   //Links [first_n, before_l_n] after prev_pos_n, n == distance(first_n, before_l_n) + 1
   static void incorporate_after(node_ptr prev_pos_n, node_ptr first_n, node_ptr before_l_n, size_type n, node_ptr *plast, SizeTraits &sz) BOOST_NOEXCEPT
   {
      if(n){
         BOOST_INTRUSIVE_INVARIANT_ASSERT(size_type(node_algorithms::distance(first_n, before_l_n) + 1u) == n);
         last_algorithms::incorporate_after(prev_pos_n, first_n, before_l_n, plast);
         sz.increase(n);
      }
   }

   //Links [first_n, before_l_n] after prev_pos_n
   static void incorporate_after(node_ptr prev_pos_n, node_ptr first_n, node_ptr before_l_n, node_ptr *plast, SizeTraits &sz) BOOST_NOEXCEPT
   {
      (void)sz;
      BOOST_IF_CONSTEXPR(constant_time_size)
         slist_node_ops::incorporate_after
            (prev_pos_n, first_n, before_l_n, size_type(node_algorithms::distance(first_n, before_l_n) + 1u), plast, sz);
      else
         last_algorithms::incorporate_after(prev_pos_n, first_n, before_l_n, plast);
   }

   //Swaps the nodes of both lists
   BOOST_INTRUSIVE_FORCEINLINE static void swap(node_ptr header, node_ptr *plast, node_ptr xheader, node_ptr *xplast) BOOST_NOEXCEPT
   {  last_algorithms::swap_trailing_nodes(header, xheader, plast, xplast);  }

   //Merges the nodes of x in the list. All the nodes of x are transferred, also if "comp" throws.
   //Returns the last node transferred from x, or null if x is empty
   template<class NodePtrCompare>
   static node_ptr merge
      ( node_ptr header, node_ptr *plast, SizeTraits &sz
      , node_ptr xheader, node_ptr *xplast, SizeTraits &xsz, NodePtrCompare comp)
   {
      sz.increase(xsz.get_size());
      xsz.set_size(size_type(0));
      return last_algorithms::merge(header, xheader, comp, plast, xplast);
   }

   //Transfers all the nodes of the list x to the empty list and leaves x empty.
   //Constant time, except in circular lists without a cached last node.
   static void move_all
      ( node_ptr header, node_ptr *plast, SizeTraits &sz
      , node_ptr xheader, node_ptr *xplast, SizeTraits &xsz) BOOST_NOEXCEPT
   {
      BOOST_INTRUSIVE_INVARIANT_ASSERT(node_algorithms::is_empty(header));
      if(!node_algorithms::is_empty(xheader)){
         BOOST_IF_CONSTEXPR(Linear && !CacheLast){
            //The last node is not needed: it already points to the end (null)
            NodeTraits::set_next(header, NodeTraits::get_next(xheader));
            node_algorithms::init_header(xheader);
            sz.increase(xsz.get_size());
            xsz.set_size(size_type(0));
         }
         else{
            slist_node_ops::splice_after_all(header, plast, sz, xheader, xplast, xsz);
         }
      }
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
};

template<class HeaderHolder, class NodePtr, bool>
struct header_holder_plus_last
{
   HeaderHolder header_holder_;
   NodePtr  last_;
};

template<class HeaderHolder, class NodePtr>
struct header_holder_plus_last<HeaderHolder, NodePtr, false>
{
   HeaderHolder header_holder_;
};

//Base class of slist_impl that holds the header, the size and the cached last node.
//Its members only depend on the node type, the header holder, the size type, the
//linear/cache_last options and the link mode (not on ValueTraits), so that they are
//instantiated only once for all slists that share the same node type.
template< class NodeTraits, class HeaderHolder, class SizeType, bool ConstantTimeSize
        , bool Linear, bool CacheLast, bool SafeModeOrAutoUnlink>
class slist_node_base
{
   public:
   typedef SizeType                                            size_type;

   protected:
   typedef size_holder<ConstantTimeSize, SizeType>             size_traits;
   typedef slist_node_ops
      <NodeTraits, size_traits, Linear, CacheLast, SafeModeOrAutoUnlink>   node_ops;
   typedef typename node_ops::node_algorithms                  node_algorithms;
   typedef typename node_ops::last_algorithms                  last_algorithms;
   typedef typename NodeTraits::node_ptr                       node_ptr;
   typedef typename NodeTraits::const_node_ptr                 const_node_ptr;
   typedef HeaderHolder                                        header_holder_type;
   typedef header_holder_plus_last
      <header_holder_type, node_ptr, CacheLast>                header_holder_plus_last_t;

   struct BOOST_INTRUSIVE_EMPTY_BASES root_plus_size
      :  public size_traits
      ,  public header_holder_plus_last_t
   {} root_plus_size_;

   BOOST_INTRUSIVE_FORCEINLINE slist_node_base() BOOST_NOEXCEPT
   {  this->set_default_constructed_state();  }

   ~slist_node_base()
   #if defined(BOOST_INTRUSIVE_CONCEPTS_BASED_OVERLOADING)
      requires (SafeModeOrAutoUnlink)
   #endif
   {
      BOOST_IF_CONSTEXPR(SafeModeOrAutoUnlink){
         node_ops::clear(this->get_root_node(), this->priv_last_ptr(), this->priv_size_traits());
         node_algorithms::init(this->get_root_node());
      }
   }

   #if defined(BOOST_INTRUSIVE_CONCEPTS_BASED_OVERLOADING)
   //Default destructor for normal links (allows conditional triviality)
   ~slist_node_base() requires (!SafeModeOrAutoUnlink) = default;
   #endif

   BOOST_INTRUSIVE_FORCEINLINE void set_default_constructed_state() BOOST_NOEXCEPT
   {  node_ops::init(this->get_root_node(), this->priv_last_ptr(), this->priv_size_traits());  }

   BOOST_INTRUSIVE_FORCEINLINE node_ptr get_root_node() BOOST_NOEXCEPT
   { return root_plus_size_.header_holder_.get_node(); }

   BOOST_INTRUSIVE_FORCEINLINE const_node_ptr get_root_node() const BOOST_NOEXCEPT
   { return root_plus_size_.header_holder_.get_node(); }

   BOOST_INTRUSIVE_FORCEINLINE node_ptr get_end_node() BOOST_NOEXCEPT
   {  return node_algorithms::end_node(this->get_root_node());  }

   BOOST_INTRUSIVE_FORCEINLINE const_node_ptr get_end_node() const BOOST_NOEXCEPT
   {  return node_algorithms::end_node(this->get_root_node());  }

   BOOST_INTRUSIVE_FORCEINLINE size_traits &priv_size_traits() BOOST_NOEXCEPT
   {  return root_plus_size_;  }

   BOOST_INTRUSIVE_FORCEINLINE const size_traits &priv_size_traits() const BOOST_NOEXCEPT
   {  return root_plus_size_;  }

   BOOST_INTRUSIVE_FORCEINLINE node_ptr get_last_node() BOOST_NOEXCEPT
   {  return this->get_last_node(detail::bool_<CacheLast>());  }

   BOOST_INTRUSIVE_FORCEINLINE const_node_ptr get_last_node() const BOOST_NOEXCEPT
   {  return this->get_last_node(detail::bool_<CacheLast>());  }

   //Pointer to the cached last node, null if CacheLast is false
   BOOST_INTRUSIVE_FORCEINLINE node_ptr *priv_last_ptr() BOOST_NOEXCEPT
   {  return this->priv_last_ptr(detail::bool_<CacheLast>());  }

   //Obtains the base from the header holder of the end node
   static slist_node_base &priv_base_from_end_node(node_ptr p) BOOST_NOEXCEPT
   {
      header_holder_type* h = header_holder_type::get_holder(p);
      header_holder_plus_last_t* hpl = detail::parent_from_member< header_holder_plus_last_t, header_holder_type>
                                         (h, &header_holder_plus_last_t::header_holder_);
      root_plus_size* r = static_cast< root_plus_size* >(hpl);
      return *detail::parent_from_member<slist_node_base, root_plus_size>(r, &slist_node_base::root_plus_size_);
   }

   private:
   //noncopyable
   slist_node_base(const slist_node_base &);
   slist_node_base &operator=(const slist_node_base &);

   BOOST_INTRUSIVE_FORCEINLINE static node_ptr *priv_last_ptr(detail::bool_<false>) BOOST_NOEXCEPT
   {  return 0;  }

   BOOST_INTRUSIVE_FORCEINLINE node_ptr *priv_last_ptr(detail::bool_<true>) BOOST_NOEXCEPT
   {  return &root_plus_size_.last_;  }

   inline static node_ptr get_last_node(detail::bool_<false>) BOOST_NOEXCEPT
   {
      //This function shall not be used if cache_last is not true
      BOOST_INTRUSIVE_INVARIANT_ASSERT(CacheLast);
      return node_ptr();
   }

   BOOST_INTRUSIVE_FORCEINLINE node_ptr get_last_node(detail::bool_<true>) BOOST_NOEXCEPT
   {  return node_ptr(root_plus_size_.last_);  }

   BOOST_INTRUSIVE_FORCEINLINE const_node_ptr get_last_node(detail::bool_<true>) const BOOST_NOEXCEPT
   {  return const_node_ptr(root_plus_size_.last_);  }

   public:
   BOOST_INTRUSIVE_FORCEINLINE size_type size() const BOOST_NOEXCEPT
   {  return node_ops::size(this->get_root_node(), this->priv_size_traits());  }

   BOOST_INTRUSIVE_FORCEINLINE bool empty() const BOOST_NOEXCEPT
   {  return node_algorithms::is_empty(this->get_root_node()); }

   BOOST_INTRUSIVE_FORCEINLINE void clear() BOOST_NOEXCEPT
   {  node_ops::clear(this->get_root_node(), this->priv_last_ptr(), this->priv_size_traits());  }

   BOOST_INTRUSIVE_FORCEINLINE void shift_backwards(size_type n = 1) BOOST_NOEXCEPT
   {  last_algorithms::shift_backwards(this->get_root_node(), std::size_t(n), this->priv_last_ptr());  }

   BOOST_INTRUSIVE_FORCEINLINE void shift_forward(size_type n = 1) BOOST_NOEXCEPT
   {  last_algorithms::shift_forward(this->get_root_node(), std::size_t(n), this->priv_last_ptr());  }

   BOOST_INTRUSIVE_FORCEINLINE void reverse() BOOST_NOEXCEPT
   {  last_algorithms::reverse(this->get_root_node(), this->priv_last_ptr());  }

   BOOST_INTRUSIVE_FORCEINLINE void check() const
   {  node_ops::check(this->get_root_node(), CacheLast ? this->get_last_node() : const_node_ptr(), this->priv_size_traits());  }
};

}  //namespace detail{

struct default_slist_hook_applier
{  template <class T> struct apply{ typedef typename T::default_slist_hook type;  };  };

template<>
struct is_default_hook_tag<default_slist_hook_applier>
{  static const bool value = true;  };

struct slist_defaults
{
   typedef default_slist_hook_applier proto_value_traits;
   static const bool constant_time_size = true;
   static const bool linear = false;
   typedef std::size_t size_type;
   static const bool cache_last = false;
   typedef void header_holder_type;
};

struct slist_bool_flags
{
   static const std::size_t linear_pos             = 1u;
   static const std::size_t constant_time_size_pos = 2u;
   static const std::size_t cache_last_pos         = 4u;
};


/// @endcond

//! The class template slist is an intrusive container, that encapsulates
//! a singly-linked list. You can use such a list to squeeze the last bit
//! of performance from your application. Unfortunately, the little gains
//! come with some huge drawbacks. A lot of member functions can't be
//! implemented as efficiently as for standard containers. To overcome
//! this limitation some other member functions with rather unusual semantics
//! have to be introduced.
//!
//! The template parameter \c T is the type to be managed by the container.
//! The user can specify additional options and if no options are provided
//! default options are used.
//!
//! The container supports the following options:
//! \c base_hook<>/member_hook<>/value_traits<>,
//! \c constant_time_size<>, \c size_type<>,
//! \c linear<> and \c cache_last<>.
//!
//! The iterators of slist are forward iterators. slist provides a static
//! function called "previous" to compute the previous iterator of a given iterator.
//! This function has linear complexity. To improve the usability esp. with
//! the '*_after' functions, ++end() == begin() and previous(begin()) == end()
//! are defined. An new special function "before_begin()" is defined, which returns
//! an iterator that points one less the beginning of the list: ++before_begin() == begin()
#if defined(BOOST_INTRUSIVE_DOXYGEN_INVOKED)
template<class T, class ...Options>
#else
template<class ValueTraits, class SizeType, std::size_t BoolFlags, typename HeaderHolder>
#endif
class BOOST_INTRUSIVE_EMPTY_BASES slist_impl
   /// @cond
   : public detail::value_traits_holder<ValueTraits>
   , public detail::slist_node_base
      < typename ValueTraits::node_traits
      , typename detail::get_header_holder_type<ValueTraits, HeaderHolder>::type
      , SizeType
      , 0 != (BoolFlags & slist_bool_flags::constant_time_size_pos)
      , 0 != (BoolFlags & slist_bool_flags::linear_pos)
      , 0 != (BoolFlags & slist_bool_flags::cache_last_pos)
      , is_safe_autounlink<ValueTraits::link_mode>::value>
   /// @endcond
{
   //Public typedefs
   public:
   typedef ValueTraits                                               value_traits;
   typedef typename value_traits::pointer                            pointer;
   typedef typename value_traits::const_pointer                      const_pointer;
   typedef BOOST_INTRUSIVE_IMPDEF(typename pointer_traits<pointer>::element_type)   value_type;
   typedef typename pointer_traits<pointer>::reference               reference;
   typedef typename pointer_traits<const_pointer>::reference         const_reference;
   typedef typename pointer_traits<pointer>::difference_type         difference_type;
   typedef SizeType                                                  size_type;
   typedef slist_iterator<value_traits, false>                       iterator;
   typedef slist_iterator<value_traits, true>                        const_iterator;
   typedef typename value_traits::node_traits                        node_traits;
   typedef typename node_traits::node                                node;
   typedef typename node_traits::node_ptr                            node_ptr;
   typedef typename node_traits::const_node_ptr                      const_node_ptr;
   typedef typename detail::get_header_holder_type
      < value_traits, HeaderHolder >::type                           header_holder_type;

   static const bool constant_time_size = 0 != (BoolFlags & slist_bool_flags::constant_time_size_pos);
   static const bool stateful_value_traits = detail::is_stateful_value_traits<value_traits>::value;
   static const bool linear = 0 != (BoolFlags & slist_bool_flags::linear_pos);
   static const bool cache_last = 0 != (BoolFlags & slist_bool_flags::cache_last_pos);
   static const bool has_container_from_iterator =
        detail::is_same< header_holder_type, detail::default_header_holder< node_traits > >::value;

   typedef typename detail::if_c
      < linear
      , linear_slist_algorithms<node_traits>
      , circular_slist_algorithms<node_traits>
      >::type                                                        node_algorithms;

   /// @cond
   private:
   typedef detail::slist_node_base
      < node_traits, header_holder_type, SizeType, constant_time_size, linear, cache_last
      , is_safe_autounlink<ValueTraits::link_mode>::value>        base_t;
   typedef detail::value_traits_holder<ValueTraits>                vtraits_holder_t;
   typedef detail::size_holder<constant_time_size, size_type>        size_traits;

   //noncopyable
   BOOST_MOVABLE_BUT_NOT_COPYABLE(slist_impl)

   static const bool safemode_or_autounlink = is_safe_autounlink<value_traits::link_mode>::value;

   typedef detail::slist_node_ops<node_traits, size_traits, linear, cache_last, safemode_or_autounlink> node_ops;
   typedef typename node_ops::last_algorithms                     last_algorithms;

   //Constant-time size is incompatible with auto-unlink hooks!
   BOOST_INTRUSIVE_STATIC_ASSERT(!(constant_time_size && ((int)value_traits::link_mode == (int)auto_unlink)));
   //Linear singly linked lists are incompatible with auto-unlink hooks!
   BOOST_INTRUSIVE_STATIC_ASSERT(!(linear && ((int)value_traits::link_mode == (int)auto_unlink)));
   //A list with cached last node is incompatible with auto-unlink hooks!
   BOOST_INTRUSIVE_STATIC_ASSERT(!(cache_last && ((int)value_traits::link_mode == (int)auto_unlink)));

   BOOST_INTRUSIVE_FORCEINLINE const value_traits &priv_value_traits() const BOOST_NOEXCEPT
   {  return *this;  }

   BOOST_INTRUSIVE_FORCEINLINE value_traits &priv_value_traits() BOOST_NOEXCEPT
   {  return *this;  }

   typedef typename boost::intrusive::value_traits_pointers
      <ValueTraits>::const_value_traits_ptr const_value_traits_ptr;

   BOOST_INTRUSIVE_FORCEINLINE const_value_traits_ptr priv_value_traits_ptr() const BOOST_NOEXCEPT
   {  return pointer_traits<const_value_traits_ptr>::pointer_to(this->priv_value_traits());  }

   /// @endcond

   public:

   ///@cond

   //! <b>Requires</b>: f and before_l belong to another slist.
   //!
   //! <b>Effects</b>: Transfers the range [f, before_l] to this
   //!   list, after the element pointed by prev_pos.
   //!   No destructors or copy constructors are called.
   //!
   //! <b>Throws</b>: Nothing.
   //!
   //! <b>Complexity</b>: Linear to the number of elements transferred
   //!   if constant_time_size is true. Constant-time otherwise.
   //!
   //! <b>Note</b>: Iterators of values obtained from list x now point to elements of this
   //!   list. Iterators of this list and all the references are not invalidated.
   //!
   //! <b>Warning</b>: Experimental function, don't use it!
   slist_impl( node_ptr f, node_ptr before_l
             , size_type n, const value_traits &v_traits = value_traits())
      :  vtraits_holder_t(v_traits), base_t()
   {
      node_ops::incorporate_after(this->get_root_node(), f, before_l, n, this->priv_last_ptr(), this->priv_size_traits());
   }

   ///@endcond

   //! <b>Effects</b>: constructs an empty list.
   //!
   //! <b>Complexity</b>: Constant
   //!
   //! <b>Throws</b>: If value_traits::node_traits::node
   //!   constructor throws (this does not happen with predefined Boost.Intrusive hooks).
   BOOST_INTRUSIVE_FORCEINLINE slist_impl()
      BOOST_NOEXCEPT_IF(::boost::move_detail::is_nothrow_default_constructible<value_traits>::value)
      :  vtraits_holder_t(), base_t()
   {}

   //! <b>Effects</b>: constructs an empty list.
   //!
   //! <b>Complexity</b>: Constant
   //!
   //! <b>Throws</b>: If value_traits::node_traits::node
   //!   constructor throws (this does not happen with predefined Boost.Intrusive hooks).
   BOOST_INTRUSIVE_FORCEINLINE explicit slist_impl(const value_traits &v_traits)
      BOOST_NOEXCEPT_IF(::boost::move_detail::is_nothrow_copy_constructible<value_traits>::value)
      :  vtraits_holder_t(v_traits), base_t()
   {}

   //! <b>Requires</b>: Dereferencing iterator must yield an lvalue of type value_type.
   //!
   //! <b>Effects</b>: Constructs a list equal to [b ,e).
   //!
   //! <b>Complexity</b>: Linear in distance(b, e). No copy constructors are called.
   //!
   //! <b>Throws</b>: If value_traits::node_traits::node
   //!   constructor throws (this does not happen with predefined Boost.Intrusive hooks),
   //!   or if the copy constructor of value_traits or the operations of Iterator throw.
   template<class Iterator>
   slist_impl(Iterator b, Iterator e, const value_traits &v_traits = value_traits())
      BOOST_NOEXCEPT_IF(::boost::move_detail::is_nothrow_copy_constructible<value_traits>::value &&
                        (detail::is_nothrow_range_iterator<Iterator, reference>::value))
      :  vtraits_holder_t(v_traits), base_t()
   {
      this->set_default_constructed_state();
      //If the iterator throws, the destructor of base_t unlinks the
      //inserted elements (safe-mode and auto-unlink hooks)
      this->insert_after(this->cbefore_begin(), b, e);
   }

   //! <b>Effects</b>: Constructs a container moving resources from another container.
   //!   Internal value traits are move constructed and
   //!   nodes belonging to x (except the node representing the "end") are linked to *this.
   //!
   //! <b>Complexity</b>: Constant if linear<> and/or cache_last<> options are used.
   //!   Linear to the elements of x otherwise.
   //!
   //! <b>Throws</b>: If value_traits::node_traits::node's
   //!   move constructor throws (this does not happen with predefined Boost.Intrusive hooks)
   //!   or the move constructor of value traits throws.
   //!
   //! <b>Note</b>: Iterators, pointers and references to the elements of x are not
   //!   invalidated and refer to the same elements as members of *this.
   //!   If value traits are stateful, iterators are invalidated.
   slist_impl(BOOST_RV_REF(slist_impl) x)
      BOOST_NOEXCEPT_IF(::boost::move_detail::is_nothrow_move_constructible<value_traits>::value)
      :  vtraits_holder_t(::boost::move(x.priv_value_traits())), base_t()
   {
      this->set_default_constructed_state();
      //Only the nodes and the size are transferred
      node_ops::move_all( this->get_root_node(), this->priv_last_ptr(), this->priv_size_traits()
                        , x.get_root_node(), x.priv_last_ptr(), x.priv_size_traits());
   }

   //! <b>Effects</b>: Unlinks the elements of *this (as clear()), move assigns the value
   //!   traits of x and links the nodes of x to *this. x is left empty and keeps valid
   //!   value traits (see the requirements of the value traits). If &x == this, there
   //!   are no effects.
   //!
   //! <b>Complexity</b>: Linear to the elements of *this if it's a safe-mode or an
   //!   auto-unlink value, constant otherwise. Plus linear to the elements of x if
   //!   linear<> and cache_last<> options are not used.
   //!
   //! <b>Throws</b>: If the move assignment of the value traits throws. Then *this is
   //!   empty and x keeps its elements.
   //!
   //! <b>Note</b>: Invalidates the iterators (but not the references) to the elements
   //!   of *this, as clear() does. Iterators, pointers and references to the elements
   //!   of x behave as in the move constructor.
   //!
   //!   If BOOST_INTRUSIVE_MOVE_ASSIGNMENT_SWAPS is defined, it is equivalent to swap(x)
   //!   (the behavior before Boost 1.93).
   slist_impl& operator=(BOOST_RV_REF(slist_impl) x)
   #if !defined(BOOST_INTRUSIVE_MOVE_ASSIGNMENT_SWAPS)
      BOOST_NOEXCEPT_IF(::boost::move_detail::is_nothrow_move_assignable<value_traits>::value)
   #endif
   {
      #if defined(BOOST_INTRUSIVE_MOVE_ASSIGNMENT_SWAPS)
      this->swap(x);
      #else
      if(BOOST_LIKELY(this != &x)){
         this->clear();
         this->priv_value_traits() = ::boost::move(x.priv_value_traits());
         //*this is empty: x is left empty
         node_ops::move_all( this->get_root_node(), this->priv_last_ptr(), this->priv_size_traits()
                           , x.get_root_node(), x.priv_last_ptr(), x.priv_size_traits());
      }
      #endif
      return *this;
   }

   //! <b>Effects</b>: If it's not a safe-mode
   //!   or an auto-unlink value, the destructor does nothing
   //!   (ie. no code is generated). Otherwise it detaches all elements from this.
   //!   In this case the objects in the list are not deleted (i.e. no destructors
   //!   are called), but the hooks according to the value_traits template parameter
   //!   are set to their default value.
   //!
   //! <b>Complexity</b>: Linear to the number of elements in the list, if
   //!   it's a safe-mode or auto-unlink value. Otherwise constant.
   #if defined(BOOST_INTRUSIVE_DOXYGEN_INVOKED)
   ~slist_impl();
   #endif

   //! <b>Effects</b>: Erases all the elements of the container.
   //!
   //! <b>Throws</b>: Nothing.
   //!
   //! <b>Complexity</b>: Linear to the number of elements of the list.
   //!   if it's a safe-mode or auto-unlink value_type. Constant time otherwise.
   //!
   //! <b>Note</b>: Invalidates the iterators (but not the references) to the erased elements.
   #if defined(BOOST_INTRUSIVE_DOXYGEN_INVOKED)
   void clear() BOOST_NOEXCEPT;
   #else
   using base_t::clear;
   #endif

   //! <b>Requires</b>: Disposer::operator()(pointer) and the copy and move
   //!   constructors of Disposer shouldn't throw.
   //!
   //! <b>Effects</b>: Erases all the elements of the container
   //!   Disposer::operator()(pointer) is called for the removed elements.
   //!
   //! <b>Throws</b>: Nothing.
   //!
   //! <b>Complexity</b>: Linear to the number of elements of the list.
   //!
   //! <b>Note</b>: Invalidates the iterators to the erased elements.
   template <class Disposer>
   void clear_and_dispose(Disposer disposer) BOOST_NOEXCEPT
   {
      node_algorithms::detach_and_dispose
         ( this->get_root_node()
         , typename detail::get_node_disposer<Disposer, value_traits, CommonSListAlgorithms>::type
               (disposer, &this->priv_value_traits()) );
      this->set_default_constructed_state();
   }

   //! <b>Requires</b>: value must be an lvalue.
   //!
   //! <b>Effects</b>: Inserts the value in the front of the list.
   //!   No copy constructors are called.
   //!
   //! <b>Throws</b>: Nothing.
   //!
   //! <b>Complexity</b>: Constant.
   //!
   //! <b>Note</b>: Does not affect the validity of iterators and references.
   void push_front(reference value) BOOST_NOEXCEPT
   {
      node_ptr to_insert = priv_value_traits().to_node_ptr(value);
      BOOST_INTRUSIVE_SAFE_HOOK_DEFAULT_ASSERT(!safemode_or_autounlink || node_algorithms::inited(to_insert));
      last_algorithms::link_after(this->get_root_node(), to_insert, this->priv_last_ptr());
      this->priv_size_traits().increment();
   }

   //! <b>Requires</b>: value must be an lvalue.
   //!
   //! <b>Effects</b>: Inserts the value in the back of the list.
   //!   No copy constructors are called.
   //!
   //! <b>Throws</b>: Nothing.
   //!
   //! <b>Complexity</b>: Constant.
   //!
   //! <b>Note</b>: Does not affect the validity of iterators and references.
   //!   This function is only available is cache_last<> is true.
   void push_back(reference value) BOOST_NOEXCEPT
   {
      BOOST_INTRUSIVE_STATIC_ASSERT((cache_last));
      node_ptr n = priv_value_traits().to_node_ptr(value);
      BOOST_INTRUSIVE_SAFE_HOOK_DEFAULT_ASSERT(!safemode_or_autounlink || node_algorithms::inited(n));
      last_algorithms::link_after(this->get_last_node(), n, this->priv_last_ptr());
      this->priv_size_traits().increment();
   }

   //! <b>Effects</b>: Erases the first element of the list.
   //!   No destructors are called.
   //!
   //! <b>Throws</b>: Nothing.
   //!
   //! <b>Complexity</b>: Constant.
   //!
   //! <b>Note</b>: Invalidates the iterators (but not the references) to the erased element.
   BOOST_INTRUSIVE_FORCEINLINE void pop_front() BOOST_NOEXCEPT
   {  node_ops::erase_after(this->get_root_node(), this->priv_last_ptr(), this->priv_size_traits());  }

   //! <b>Requires</b>: Disposer::operator()(pointer) and the copy and move
   //!   constructors of Disposer shouldn't throw.
   //!
   //! <b>Effects</b>: Erases the first element of the list.
   //!   Disposer::operator()(pointer) is called for the removed element.
   //!
   //! <b>Throws</b>: Nothing.
   //!
   //! <b>Complexity</b>: Constant.
   //!
   //! <b>Note</b>: Invalidates the iterators to the erased element.
   template<class Disposer>
   void pop_front_and_dispose(Disposer disposer) BOOST_NOEXCEPT
   {
      node_ptr to_erase = node_traits::get_next(this->get_root_node());
      last_algorithms::unlink_after(this->get_root_node(), this->priv_last_ptr());
      this->priv_size_traits().decrement();
      BOOST_IF_CONSTEXPR(safemode_or_autounlink)
         node_algorithms::init(to_erase);
      disposer(priv_value_traits().to_value_ptr(to_erase));
   }

   //! <b>Effects</b>: Returns a reference to the first element of the list.
   //!
   //! <b>Throws</b>: Nothing.
   //!
   //! <b>Complexity</b>: Constant.
   BOOST_INTRUSIVE_FORCEINLINE reference front() BOOST_NOEXCEPT
   { return *this->priv_value_traits().to_value_ptr(node_traits::get_next(this->get_root_node())); }

   //! <b>Effects</b>: Returns a const_reference to the first element of the list.
   //!
   //! <b>Throws</b>: Nothing.
   //!
   //! <b>Complexity</b>: Constant.
   BOOST_INTRUSIVE_FORCEINLINE const_reference front() const BOOST_NOEXCEPT
   { return *this->priv_value_traits().to_value_ptr(detail::uncast(node_traits::get_next(this->get_root_node()))); }

   //! <b>Effects</b>: Returns a reference to the last element of the list.
   //!
   //! <b>Throws</b>: Nothing.
   //!
   //! <b>Complexity</b>: Constant.
   //!
   //! <b>Note</b>: Does not affect the validity of iterators and references.
   //!   This function is only available is cache_last<> is true.
   reference back() BOOST_NOEXCEPT
   {
      BOOST_INTRUSIVE_STATIC_ASSERT((cache_last));
      return *this->priv_value_traits().to_value_ptr(this->get_last_node());
   }

   //! <b>Effects</b>: Returns a const_reference to the last element of the list.
   //!
   //! <b>Throws</b>: Nothing.
   //!
   //! <b>Complexity</b>: Constant.
   //!
   //! <b>Note</b>: Does not affect the validity of iterators and references.
   //!   This function is only available is cache_last<> is true.
   inline const_reference back() const BOOST_NOEXCEPT
   {
      BOOST_INTRUSIVE_STATIC_ASSERT((cache_last));
      return *this->priv_value_traits().to_value_ptr(this->get_last_node());
   }

   //! <b>Effects</b>: Returns an iterator to the first element contained in the list.
   //!
   //! <b>Throws</b>: Nothing.
   //!
   //! <b>Complexity</b>: Constant.
   BOOST_INTRUSIVE_FORCEINLINE iterator begin() BOOST_NOEXCEPT
   { return iterator (node_traits::get_next(this->get_root_node()), this->priv_value_traits_ptr()); }

   //! <b>Effects</b>: Returns a const_iterator to the first element contained in the list.
   //!
   //! <b>Throws</b>: Nothing.
   //!
   //! <b>Complexity</b>: Constant.
   BOOST_INTRUSIVE_FORCEINLINE const_iterator begin() const BOOST_NOEXCEPT
   { return const_iterator (node_traits::get_next(this->get_root_node()), this->priv_value_traits_ptr()); }

   //! <b>Effects</b>: Returns a const_iterator to the first element contained in the list.
   //!
   //! <b>Throws</b>: Nothing.
   //!
   //! <b>Complexity</b>: Constant.
   BOOST_INTRUSIVE_FORCEINLINE const_iterator cbegin() const BOOST_NOEXCEPT
   { return const_iterator(node_traits::get_next(this->get_root_node()), this->priv_value_traits_ptr()); }

   //! <b>Effects</b>: Returns an iterator to the end of the list.
   //!
   //! <b>Throws</b>: Nothing.
   //!
   //! <b>Complexity</b>: Constant.
   BOOST_INTRUSIVE_FORCEINLINE iterator end() BOOST_NOEXCEPT
   { return iterator(this->get_end_node(), this->priv_value_traits_ptr()); }

   //! <b>Effects</b>: Returns a const_iterator to the end of the list.
   //!
   //! <b>Throws</b>: Nothing.
   //!
   //! <b>Complexity</b>: Constant.
   BOOST_INTRUSIVE_FORCEINLINE const_iterator end() const BOOST_NOEXCEPT
   { return const_iterator(detail::uncast(this->get_end_node()), this->priv_value_traits_ptr()); }

   //! <b>Effects</b>: Returns a const_iterator to the end of the list.
   //!
   //! <b>Throws</b>: Nothing.
   //!
   //! <b>Complexity</b>: Constant.
   BOOST_INTRUSIVE_FORCEINLINE const_iterator cend() const BOOST_NOEXCEPT
   { return this->end(); }

   //! <b>Effects</b>: Returns an iterator that points to a position
   //!   before the first element. Equivalent to "end()" if linear<> is false.
   //!
   //! <b>Throws</b>: Nothing.
   //!
   //! <b>Complexity</b>: Constant.
   BOOST_INTRUSIVE_FORCEINLINE iterator before_begin() BOOST_NOEXCEPT
   { return iterator(this->get_root_node(), this->priv_value_traits_ptr()); }

   //! <b>Effects</b>: Returns a const_iterator that points to a position
   //!   before the first element. Equivalent to "end()" if linear<> is false.
   //!
   //! <b>Throws</b>: Nothing.
   //!
   //! <b>Complexity</b>: Constant.
   BOOST_INTRUSIVE_FORCEINLINE const_iterator before_begin() const BOOST_NOEXCEPT
   { return const_iterator(detail::uncast(this->get_root_node()), this->priv_value_traits_ptr()); }

   //! <b>Effects</b>: Returns a const_iterator that points to a position
   //!   before the first element. Equivalent to "end()" if linear<> is false.
   //!
   //! <b>Throws</b>: Nothing.
   //!
   //! <b>Complexity</b>: Constant.
   BOOST_INTRUSIVE_FORCEINLINE const_iterator cbefore_begin() const BOOST_NOEXCEPT
   { return this->before_begin(); }

   //! <b>Effects</b>: Returns an iterator to the last element contained in the list.
   //!
   //! <b>Throws</b>: Nothing.
   //!
   //! <b>Complexity</b>: Constant.
   //!
   //! <b>Note</b>: This function is present only if cache_last<> option is true.
   inline iterator last() BOOST_NOEXCEPT
   {
      //This function shall not be used if cache_last is not true
      BOOST_INTRUSIVE_INVARIANT_ASSERT(cache_last);
      return iterator (this->get_last_node(), this->priv_value_traits_ptr());
   }

   //! <b>Effects</b>: Returns a const_iterator to the last element contained in the list.
   //!
   //! <b>Throws</b>: Nothing.
   //!
   //! <b>Complexity</b>: Constant.
   //!
   //! <b>Note</b>: This function is present only if cache_last<> option is true.
   inline const_iterator last() const BOOST_NOEXCEPT
   {
      //This function shall not be used if cache_last is not true
      BOOST_INTRUSIVE_INVARIANT_ASSERT(cache_last);
      return const_iterator (detail::uncast(this->get_last_node()), this->priv_value_traits_ptr());
   }

   //! <b>Effects</b>: Returns a const_iterator to the last element contained in the list.
   //!
   //! <b>Throws</b>: Nothing.
   //!
   //! <b>Complexity</b>: Constant.
   //!
   //! <b>Note</b>: This function is present only if cache_last<> option is true.
   BOOST_INTRUSIVE_FORCEINLINE const_iterator clast() const BOOST_NOEXCEPT
   {  return this->last();  }

   //! <b>Precondition</b>: end_iterator must be a valid end iterator
   //!   of slist.
   //!
   //! <b>Effects</b>: Returns a reference to the slist associated to the end iterator
   //!
   //! <b>Throws</b>: Nothing.
   //!
   //! <b>Complexity</b>: Constant.
   BOOST_INTRUSIVE_NO_DANGLING
   BOOST_INTRUSIVE_FORCEINLINE static slist_impl &container_from_end_iterator(iterator end_iterator) BOOST_NOEXCEPT
   {  return slist_impl::priv_container_from_end_iterator(end_iterator);   }

   //! <b>Precondition</b>: end_iterator must be a valid end const_iterator
   //!   of slist.
   //!
   //! <b>Effects</b>: Returns a const reference to the slist associated to the end iterator
   //!
   //! <b>Throws</b>: Nothing.
   //!
   //! <b>Complexity</b>: Constant.
   BOOST_INTRUSIVE_NO_DANGLING
   BOOST_INTRUSIVE_FORCEINLINE static const slist_impl &container_from_end_iterator(const_iterator end_iterator) BOOST_NOEXCEPT
   {  return slist_impl::priv_container_from_end_iterator(end_iterator);   }

   //! <b>Effects</b>: Returns the number of the elements contained in the list.
   //!
   //! <b>Throws</b>: Nothing.
   //!
   //! <b>Complexity</b>: Linear to the number of elements contained in the list.
   //!   if constant_time_size is false. Constant time otherwise.
   //!
   //! <b>Note</b>: Does not affect the validity of iterators and references.
   #if defined(BOOST_INTRUSIVE_DOXYGEN_INVOKED)
   size_type size() const BOOST_NOEXCEPT;
   #else
   using base_t::size;
   #endif

   //! <b>Effects</b>: Returns true if the list contains no elements.
   //!
   //! <b>Throws</b>: Nothing.
   //!
   //! <b>Complexity</b>: Constant.
   //!
   //! <b>Note</b>: Does not affect the validity of iterators and references.
   #if defined(BOOST_INTRUSIVE_DOXYGEN_INVOKED)
   bool empty() const BOOST_NOEXCEPT;
   #else
   using base_t::empty;
   #endif

   //! <b>Effects</b>: Swaps the elements and the value traits of x and *this.
   //!
   //! <b>Throws</b>: Nothing.
   //!
   //! <b>Complexity</b>: Linear to the number of elements of both lists.
   //!  Constant-time if linear<> and/or cache_last<> options are used.
   //!
   //! <b>Note</b>: Does not affect the validity of iterators and references.
   //!   If value traits are stateful, iterators are invalidated.
   void swap(slist_impl& other) BOOST_NOEXCEPT
   {
      node_ops::swap(this->get_root_node(), this->priv_last_ptr(), other.get_root_node(), other.priv_last_ptr());
      ::boost::adl_move_swap(this->priv_value_traits(), other.priv_value_traits());
      this->priv_size_traits().swap(other.priv_size_traits());
   }

   //! <b>Effects</b>: Moves backwards all the elements, so that the first
   //!   element becomes the second, the second becomes the third...
   //!   the last element becomes the first one.
   //!
   //! <b>Throws</b>: Nothing.
   //!
   //! <b>Complexity</b>: Linear to the number of elements plus the number shifts.
   //!
   //! <b>Note</b>: Does not affect the validity of iterators and references.
   #if defined(BOOST_INTRUSIVE_DOXYGEN_INVOKED)
   void shift_backwards(size_type n = 1) BOOST_NOEXCEPT;
   #else
   using base_t::shift_backwards;
   #endif

   //! <b>Effects</b>: Moves forward all the elements, so that the second
   //!   element becomes the first, the third becomes the second...
   //!   the first element becomes the last one.
   //!
   //! <b>Throws</b>: Nothing.
   //!
   //! <b>Complexity</b>: Linear to the number of elements plus the number shifts.
   //!
   //! <b>Note</b>: Does not affect the validity of iterators and references.
   #if defined(BOOST_INTRUSIVE_DOXYGEN_INVOKED)
   void shift_forward(size_type n = 1) BOOST_NOEXCEPT;
   #else
   using base_t::shift_forward;
   #endif

   //! <b>Requires</b>: Disposer::operator()(pointer) and the copy and move
   //!   constructors of Disposer shouldn't throw.
   //!   Cloner should yield to nodes equivalent to the original nodes.
   //!
   //! <b>Effects</b>: Erases all the elements from *this
   //!   calling Disposer::operator()(pointer), clones all the
   //!   elements from src calling Cloner::operator()(const_reference )
   //!   and inserts them on *this. If &src == this, there are no effects.
   //!
   //!   If cloner throws, all cloned elements are unlinked and disposed
   //!   calling Disposer::operator()(pointer).
   //!
   //! <b>Complexity</b>: Linear to erased plus inserted elements.
   //!
   //! <b>Throws</b>: If cloner throws.
   //!
   //! <b>Note</b>: Invalidates the iterators to the erased elements of *this.
   template <class Cloner, class Disposer>
   void clone_from(const slist_impl &src, Cloner cloner, Disposer disposer)
   {
      if(BOOST_UNLIKELY(&src == this))
         return;
      this->clear_and_dispose(disposer);
      detail::exception_disposer<slist_impl, Disposer>
         rollback(*this, disposer);
      const_iterator prev(this->cbefore_begin());
      const_iterator b(src.begin()), e(src.end());
      for(; b != e; ++b){
         prev = this->insert_after(prev, *cloner(*b));
      }
      rollback.release();
   }

   //! <b>Requires</b>: Disposer::operator()(pointer) and the copy and move
   //!   constructors of Disposer shouldn't throw.
   //!   Cloner should yield to nodes equivalent to the original nodes.
   //!
   //! <b>Effects</b>: Erases all the elements from *this
   //!   calling Disposer::operator()(pointer), clones all the
   //!   elements from src calling Cloner::operator()(reference)
   //!   and inserts them on *this. If &src == this, there are no effects.
   //!
   //!   If cloner throws, all cloned elements are unlinked and disposed
   //!   calling Disposer::operator()(pointer).
   //!
   //! <b>Complexity</b>: Linear to erased plus inserted elements.
   //!
   //! <b>Throws</b>: If cloner throws.
   //!
   //! <b>Note</b>: Invalidates the iterators to the erased elements of *this.
   template <class Cloner, class Disposer>
   void clone_from(BOOST_RV_REF(slist_impl) src, Cloner cloner, Disposer disposer)
   {
      if(BOOST_UNLIKELY(&src == this))
         return;
      this->clear_and_dispose(disposer);
      detail::exception_disposer<slist_impl, Disposer>
         rollback(*this, disposer);
      iterator prev(this->before_begin());
      iterator b(src.begin()), e(src.end());
      for(; b != e; ++b){
         prev = this->insert_after(prev, *cloner(*b));
      }
      rollback.release();
   }

   //! <b>Requires</b>: value must be an lvalue and prev_p must point to an element
   //!   contained by the list or to end().
   //!
   //! <b>Effects</b>: Inserts the value after the position pointed by prev_p.
   //!    No copy constructor is called.
   //!
   //! <b>Returns</b>: An iterator to the inserted element.
   //!
   //! <b>Throws</b>: Nothing.
   //!
   //! <b>Complexity</b>: Constant.
   //!
   //! <b>Note</b>: Does not affect the validity of iterators and references.
   iterator insert_after(const_iterator prev_p, reference value) BOOST_NOEXCEPT
   {
      node_ptr n = priv_value_traits().to_node_ptr(value);
      BOOST_INTRUSIVE_SAFE_HOOK_DEFAULT_ASSERT(!safemode_or_autounlink || node_algorithms::inited(n));
      last_algorithms::link_after(prev_p.pointed_node(), n, this->priv_last_ptr());
      this->priv_size_traits().increment();
      return iterator (n, this->priv_value_traits_ptr());
   }

   //! <b>Requires</b>: Dereferencing iterator must yield
   //!   an lvalue of type value_type and prev_p must point to an element
   //!   contained by the list or to the end node.
   //!
   //! <b>Effects</b>: Inserts the [f, l)
   //!   after the position prev_p.
   //!
   //! <b>Throws</b>: If the operations of Iterator throw. Basic guarantee: the elements
   //!   inserted before the exception stay in the list.
   //!
   //! <b>Complexity</b>: Linear to the number of elements inserted.
   //!
   //! <b>Note</b>: Does not affect the validity of iterators and references.
   template<class Iterator>
   void insert_after(const_iterator prev_p, Iterator f, Iterator l)
      BOOST_NOEXCEPT_IF((detail::is_nothrow_range_iterator<Iterator, reference>::value))
   {
      //Link the nodes in a chain [first_n, last_n]. The fixer inserts the chain
      //after prev_p at the end, also if the iterator throws.
      size_type count = 0;
      node_ptr first_n = node_ptr(), last_n = node_ptr();
      const range_insertion_fixer fixer(*this, prev_p.pointed_node(), first_n, last_n, count);
      for (; f != l; ++f){
         const node_ptr n = priv_value_traits().to_node_ptr(*f);
         BOOST_INTRUSIVE_SAFE_HOOK_DEFAULT_ASSERT(!safemode_or_autounlink || node_algorithms::inited(n));
         if(count){
            node_traits::set_next(last_n, n);
         }
         else{
            first_n = n;
         }
         last_n = n;
         ++count;
      }
   }

   //! <b>Requires</b>: value must be an lvalue and p must point to an element
   //!   contained by the list or to end().
   //!
   //! <b>Effects</b>: Inserts the value before the position pointed by p.
   //!   No copy constructor is called.
   //!
   //! <b>Throws</b>: Nothing.
   //!
   //! <b>Complexity</b>: Linear to the number of elements before p.
   //!  Constant-time if cache_last<> is true and p == end().
   //!
   //! <b>Note</b>: Does not affect the validity of iterators and references.
   BOOST_INTRUSIVE_FORCEINLINE iterator insert(const_iterator p, reference value) BOOST_NOEXCEPT
   {  return this->insert_after(this->previous(p), value);  }

   //! <b>Requires</b>: Dereferencing iterator must yield
   //!   an lvalue of type value_type and p must point to an element
   //!   contained by the list or to the end node.
   //!
   //! <b>Effects</b>: Inserts the pointed by b and e
   //!   before the position p. No copy constructors are called.
   //!
   //! <b>Throws</b>: If the operations of Iterator throw. Basic guarantee: the elements
   //!   inserted before the exception stay in the list.
   //!
   //! <b>Complexity</b>: Linear to the number of elements inserted plus linear
   //!   to the elements before p.
   //!   Linear to the number of elements to insert if cache_last<> option is true and p == end().
   //!
   //! <b>Note</b>: Does not affect the validity of iterators and references.
   template<class Iterator>
   BOOST_INTRUSIVE_FORCEINLINE void insert(const_iterator p, Iterator b, Iterator e)
      BOOST_NOEXCEPT_IF((detail::is_nothrow_range_iterator<Iterator, reference>::value))
   {  return this->insert_after(this->previous(p), b, e);  }

   //! <b>Effects</b>: Erases the element after the element pointed by prev of
   //!   the list. No destructors are called.
   //!
   //! <b>Returns</b>: the first element remaining beyond the removed elements,
   //!   or end() if no such element exists.
   //!
   //! <b>Throws</b>: Nothing.
   //!
   //! <b>Complexity</b>: Constant.
   //!
   //! <b>Note</b>: Invalidates the iterators (but not the references) to the
   //!   erased element.
   iterator erase_after(const_iterator prev) BOOST_NOEXCEPT
   {
      const_iterator it(prev);
      ++it;
      ++it;
      node_ops::erase_after(prev.pointed_node(), this->priv_last_ptr(), this->priv_size_traits());
      return it.unconst();
   }

   //! <b>Effects</b>: Erases the range (before_f, l) from
   //!   the list. No destructors are called.
   //!
   //! <b>Returns</b>: the first element remaining beyond the removed elements,
   //!   or end() if no such element exists.
   //!
   //! <b>Throws</b>: Nothing.
   //!
   //! <b>Complexity</b>: Linear to the number of erased elements if it's a safe-mode
   //!   , auto-unlink value or constant-time size is activated. Constant time otherwise.
   //!
   //! <b>Note</b>: Invalidates the iterators (but not the references) to the
   //!   erased element.
   iterator erase_after(const_iterator before_f, const_iterator l) BOOST_NOEXCEPT
   {
      node_ops::erase_after(before_f.pointed_node(), l.pointed_node(), this->priv_last_ptr(), this->priv_size_traits());
      return l.unconst();
   }

   //! <b>Effects</b>: Erases the range (before_f, l) from
   //!   the list. n must be distance(before_f, l) - 1.
   //!   No destructors are called.
   //!
   //! <b>Returns</b>: the first element remaining beyond the removed elements,
   //!   or end() if no such element exists.
   //!
   //! <b>Throws</b>: Nothing.
   //!
   //! <b>Complexity</b>: constant-time if link_mode is normal_link.
   //!   Linear to the elements (l - before_f) otherwise.
   //!
   //! <b>Note</b>: Invalidates the iterators (but not the references) to the
   //!   erased element.
   iterator erase_after(const_iterator before_f, const_iterator l, size_type n) BOOST_NOEXCEPT
   {
      node_ops::erase_after(before_f.pointed_node(), l.pointed_node(), n, this->priv_last_ptr(), this->priv_size_traits());
      return l.unconst();
   }

   //! <b>Effects</b>: Erases the element pointed by i of the list.
   //!   No destructors are called.
   //!
   //! <b>Returns</b>: the first element remaining beyond the removed element,
   //!   or end() if no such element exists.
   //!
   //! <b>Throws</b>: Nothing.
   //!
   //! <b>Complexity</b>: Linear to the elements before i.
   //!
   //! <b>Note</b>: Invalidates the iterators (but not the references) to the
   //!   erased element.
   BOOST_INTRUSIVE_FORCEINLINE iterator erase(const_iterator i) BOOST_NOEXCEPT
   {  return this->erase_after(this->previous(i));  }

   //! <b>Requires</b>: f and l must be valid iterator to elements in *this.
   //!
   //! <b>Effects</b>: Erases the range [f, l).
   //!   No destructors are called.
   //!
   //! <b>Returns</b>: the first element remaining beyond the removed elements,
   //!   or end() if no such element exists.
   //!
   //! <b>Throws</b>: Nothing.
   //!
   //! <b>Complexity</b>: Linear to the elements before l.
   //!
   //! <b>Note</b>: Invalidates the iterators (but not the references) to the
   //!   erased elements.
   BOOST_INTRUSIVE_FORCEINLINE iterator erase(const_iterator f, const_iterator l) BOOST_NOEXCEPT
   {  return this->erase_after(this->previous(f), l);  }

   //! <b>Effects</b>: Erases the range [f, l) from
   //!   the list. n must be distance(f, l).
   //!   No destructors are called.
   //!
   //! <b>Returns</b>: the first element remaining beyond the removed elements,
   //!   or end() if no such element exists.
   //!
   //! <b>Throws</b>: Nothing.
   //!
   //! <b>Complexity</b>: linear to the elements before f if link_mode is normal_link
   //!   and constant_time_size is activated. Linear to the elements before l otherwise.
   //!
   //! <b>Note</b>: Invalidates the iterators (but not the references) to the
   //!   erased element.
   BOOST_INTRUSIVE_FORCEINLINE iterator erase(const_iterator f, const_iterator l, size_type n) BOOST_NOEXCEPT
   {  return this->erase_after(this->previous(f), l, n);  }

   //! <b>Requires</b>: Disposer::operator()(pointer) and the copy and move
   //!   constructors of Disposer shouldn't throw.
   //!
   //! <b>Effects</b>: Erases the element after the element pointed by prev of
   //!   the list.
   //!   Disposer::operator()(pointer) is called for the removed element.
   //!
   //! <b>Returns</b>: the first element remaining beyond the removed elements,
   //!   or end() if no such element exists.
   //!
   //! <b>Throws</b>: Nothing.
   //!
   //! <b>Complexity</b>: Constant.
   //!
   //! <b>Note</b>: Invalidates the iterators to the erased element.
   template<class Disposer>
   iterator erase_after_and_dispose(const_iterator prev, Disposer disposer) BOOST_NOEXCEPT
   {
      const_iterator it(prev);
      ++it;
      node_ptr to_erase(it.pointed_node());
      ++it;
      last_algorithms::unlink_after(prev.pointed_node(), this->priv_last_ptr());
      BOOST_IF_CONSTEXPR(safemode_or_autounlink)
         node_algorithms::init(to_erase);
      disposer(priv_value_traits().to_value_ptr(to_erase));
      this->priv_size_traits().decrement();
      return it.unconst();
   }

   /// @cond

   static iterator s_insert_after(const_iterator const prev_p, reference value) BOOST_NOEXCEPT
   {
      BOOST_INTRUSIVE_STATIC_ASSERT(((!cache_last)&&(!constant_time_size)&&(!stateful_value_traits)));
      node_ptr const n = value_traits::to_node_ptr(value);
      BOOST_INTRUSIVE_SAFE_HOOK_DEFAULT_ASSERT(!safemode_or_autounlink || node_algorithms::inited(n));
      node_algorithms::link_after(prev_p.pointed_node(), n);
      return iterator (n, const_value_traits_ptr());
   }

   template<class Disposer>
   static iterator s_erase_after_and_dispose(const_iterator prev, Disposer disposer) BOOST_NOEXCEPT
   {
      BOOST_INTRUSIVE_STATIC_ASSERT(((!cache_last)&&(!constant_time_size)&&(!stateful_value_traits)));
      const_iterator it(prev);
      ++it;
      node_ptr to_erase(it.pointed_node());
      ++it;
      node_ptr prev_n(prev.pointed_node());
      node_algorithms::unlink_after(prev_n);
      BOOST_IF_CONSTEXPR(safemode_or_autounlink)
         node_algorithms::init(to_erase);
      disposer(value_traits::to_value_ptr(to_erase));
      return it.unconst();
   }

   template<class Disposer>
   static iterator s_erase_after_and_dispose(const_iterator before_f, const_iterator l, Disposer disposer) BOOST_NOEXCEPT
   {
      BOOST_INTRUSIVE_STATIC_ASSERT(((!cache_last)&&(!constant_time_size)&&(!stateful_value_traits)));
      node_ptr bfp(before_f.pointed_node()), lp(l.pointed_node());
      node_ptr fp(node_traits::get_next(bfp));
      node_algorithms::unlink_after(bfp, lp);
      while(fp != lp){
         node_ptr to_erase(fp);
         fp = node_traits::get_next(fp);
         BOOST_IF_CONSTEXPR(safemode_or_autounlink)
            node_algorithms::init(to_erase);
         disposer(value_traits::to_value_ptr(to_erase));
      }
      return l.unconst();
   }

   BOOST_INTRUSIVE_FORCEINLINE static iterator s_erase_after(const_iterator prev) BOOST_NOEXCEPT
   {  return s_erase_after_and_dispose(prev, detail::null_disposer());  }

   /// @endcond

   //! <b>Requires</b>: Disposer::operator()(pointer) and the copy and move
   //!   constructors of Disposer shouldn't throw.
   //!
   //! <b>Effects</b>: Erases the range (before_f, l) from
   //!   the list.
   //!   Disposer::operator()(pointer) is called for the removed elements.
   //!
   //! <b>Returns</b>: the first element remaining beyond the removed elements,
   //!   or end() if no such element exists.
   //!
   //! <b>Throws</b>: Nothing.
   //!
   //! <b>Complexity</b>: Linear to the number of erased elements.
   //!
   //! <b>Note</b>: Invalidates the iterators to the erased element.
   template<class Disposer>
   iterator erase_after_and_dispose(const_iterator before_f, const_iterator l, Disposer disposer) BOOST_NOEXCEPT
   {
      node_ptr bfp(before_f.pointed_node()), lp(l.pointed_node());
      node_ptr fp(node_traits::get_next(bfp));
      last_algorithms::unlink_after(bfp, lp, this->priv_last_ptr());
      while(fp != lp){
         node_ptr to_erase(fp);
         fp = node_traits::get_next(fp);
         BOOST_IF_CONSTEXPR(safemode_or_autounlink)
            node_algorithms::init(to_erase);
         disposer(priv_value_traits().to_value_ptr(to_erase));
         this->priv_size_traits().decrement();
      }
      return l.unconst();
   }

   //! <b>Requires</b>: Disposer::operator()(pointer) and the copy and move
   //!   constructors of Disposer shouldn't throw.
   //!
   //! <b>Effects</b>: Erases the element pointed by i of the list.
   //!   No destructors are called.
   //!   Disposer::operator()(pointer) is called for the removed element.
   //!
   //! <b>Returns</b>: the first element remaining beyond the removed element,
   //!   or end() if no such element exists.
   //!
   //! <b>Throws</b>: Nothing.
   //!
   //! <b>Complexity</b>: Linear to the elements before i.
   //!
   //! <b>Note</b>: Invalidates the iterators (but not the references) to the
   //!   erased element.
   template<class Disposer>
   BOOST_INTRUSIVE_FORCEINLINE iterator erase_and_dispose(const_iterator i, Disposer disposer) BOOST_NOEXCEPT
   {  return this->erase_after_and_dispose(this->previous(i), disposer);  }

   #if !defined(BOOST_INTRUSIVE_DOXYGEN_INVOKED)
   template<class Disposer>
   BOOST_INTRUSIVE_FORCEINLINE iterator erase_and_dispose(iterator i, Disposer disposer) BOOST_NOEXCEPT
   {  return this->erase_and_dispose(const_iterator(i), disposer);   }
   #endif

   //! <b>Requires</b>: f and l must be valid iterator to elements in *this.
   //!                  Disposer::operator()(pointer) and the copy and move
   //!   constructors of Disposer shouldn't throw.
   //!
   //! <b>Effects</b>: Erases the range [f, l).
   //!   No destructors are called.
   //!   Disposer::operator()(pointer) is called for the removed elements.
   //!
   //! <b>Returns</b>: the first element remaining beyond the removed elements,
   //!   or end() if no such element exists.
   //!
   //! <b>Throws</b>: Nothing.
   //!
   //! <b>Complexity</b>: Linear to the number of erased elements plus linear
   //!   to the elements before f.
   //!
   //! <b>Note</b>: Invalidates the iterators (but not the references) to the
   //!   erased elements.
   template<class Disposer>
   BOOST_INTRUSIVE_FORCEINLINE iterator erase_and_dispose(const_iterator f, const_iterator l, Disposer disposer) BOOST_NOEXCEPT
   {  return this->erase_after_and_dispose(this->previous(f), l, disposer);  }

   //! <b>Requires</b>: Dereferencing iterator must yield
   //!   an lvalue of type value_type.
   //!
   //! <b>Effects</b>: Clears the list and inserts the range pointed by b and e.
   //!   No destructors or copy constructors are called.
   //!
   //! <b>Throws</b>: If the operations of Iterator throw. Basic guarantee: the list
   //!   holds the elements inserted before the exception.
   //!
   //! <b>Complexity</b>: Linear to the number of elements inserted plus
   //!   linear to the elements contained in the list if it's a safe-mode
   //!   or auto-unlink value.
   //!   Linear to the number of elements inserted in the list otherwise.
   //!
   //! <b>Note</b>: Invalidates the iterators (but not the references)
   //!   to the erased elements.
   template<class Iterator>
   void assign(Iterator b, Iterator e)
      BOOST_NOEXCEPT_IF((detail::is_nothrow_range_iterator<Iterator, reference>::value))
   {
      this->clear();
      this->insert_after(this->cbefore_begin(), b, e);
   }

   //! <b>Requires</b>: Disposer::operator()(pointer) and the copy and move
   //!   constructors of Disposer shouldn't throw.
   //!   Dereferencing iterator must yield an lvalue of type value_type.
   //!
   //! <b>Effects</b>: Clears the list and inserts the range pointed by b and e.
   //!   No destructors or copy constructors are called.
   //!   Disposer::operator()(pointer) is called for the removed elements.
   //!
   //! <b>Throws</b>: If the operations of Iterator throw. Basic guarantee.
   //!
   //! <b>Complexity</b>: Linear to the number of elements inserted plus
   //!   linear to the elements contained in the list.
   //!
   //! <b>Note</b>: Invalidates the iterators (but not the references)
   //!   to the erased elements.
   template<class Iterator, class Disposer>
   void dispose_and_assign(Disposer disposer, Iterator b, Iterator e)
   {
      this->clear_and_dispose(disposer);
      this->insert_after(this->cbefore_begin(), b, e);
   }

   //! <b>Requires</b>: prev must point to an element contained by this list or
   //!   to the before_begin() element. &x != this.
   //!
   //! <b>Effects</b>: Transfers all the elements of list x to this list, after the
   //! the element pointed by prev. No destructors or copy constructors are called.
   //!
   //! <b>Returns</b>: Nothing.
   //!
   //! <b>Throws</b>: Nothing.
   //!
   //! <b>Complexity</b>: In general, linear to the elements contained in x.
   //!   Constant-time if cache_last<> option is true and also constant-time if
   //!   linear<> option is true "this" is empty and "l" is not used.
   //!
   //! <b>Note</b>: Iterators of values obtained from list x now point to elements of this
   //! list. Iterators of this list and all the references are not invalidated.
   //!
   //! <b>Additional note</b>: If the optional parameter "l" is provided, it will be
   //!   assigned to the last spliced element or prev if x is empty.
   //!   This iterator can be used as new "prev" iterator for a new splice_after call.
   //!   that will splice new values after the previously spliced values.
   void splice_after(const_iterator prev, slist_impl &x, const_iterator *l = 0) BOOST_NOEXCEPT
   {
      if(x.empty()){
         if(l) *l = prev;
      }
      else if(linear && this->empty()){
         node_ops::move_all( this->get_root_node(), this->priv_last_ptr(), this->priv_size_traits()
                           , x.get_root_node(), x.priv_last_ptr(), x.priv_size_traits());
         if(l) *l = this->previous(this->cend());
      }
      else{
         BOOST_INTRUSIVE_INVARIANT_ASSERT(&x != this);
         node_ptr const last_x_n = node_ops::splice_after_all
            ( prev.pointed_node(), this->priv_last_ptr(), this->priv_size_traits()
            , x.get_root_node(), x.priv_last_ptr(), x.priv_size_traits());
         if(l) *l = const_iterator(last_x_n, this->priv_value_traits_ptr());
      }
   }

   //! <b>Requires</b>: prev_pos must point to an element contained by this list or
   //!   to the before_begin() element. prev_ele must point to an element contained in list
   //!   x or must be x.before_begin().
   //!
   //! <b>Effects</b>: Transfers the element after prev_ele, from list x to this list,
   //!   after the element pointed by prev_pos. No destructors or copy constructors are called.
   //!
   //! <b>Throws</b>: Nothing.
   //!
   //! <b>Complexity</b>: Constant.
   //!
   //! <b>Note</b>: Iterators of values obtained from list x now point to elements of this
   //! list. Iterators of this list and all the references are not invalidated.
   void splice_after(const_iterator prev_pos, slist_impl &x, const_iterator prev_ele) BOOST_NOEXCEPT
   {
      const_iterator elem = prev_ele;
      this->splice_after(prev_pos, x, prev_ele, ++elem, 1);
   }

   //! <b>Requires</b>: prev_pos must be a dereferenceable iterator in *this or be
   //!   before_begin(), and before_f and before_l belong to x and
   //!   ++before_f != x.end() && before_l != x.end().
   //!
   //! <b>Effects</b>: Transfers the range (before_f, before_l] from list x to this
   //!   list, after the element pointed by prev_pos.
   //!   No destructors or copy constructors are called.
   //!
   //! <b>Throws</b>: Nothing.
   //!
   //! <b>Complexity</b>: Linear to the number of elements transferred
   //!   if constant_time_size is true. Constant-time otherwise.
   //!
   //! <b>Note</b>: Iterators of values obtained from list x now point to elements of this
   //!   list. Iterators of this list and all the references are not invalidated.
   BOOST_INTRUSIVE_FORCEINLINE void splice_after(const_iterator prev_pos, slist_impl &x, const_iterator before_f, const_iterator before_l) BOOST_NOEXCEPT
   {
      node_ops::splice_after
         ( prev_pos.pointed_node(), this->priv_last_ptr(), this->priv_size_traits(), before_f.pointed_node(), before_l.pointed_node()
         , x.priv_last_ptr(), x.priv_size_traits());
   }

   //! <b>Requires</b>: prev_pos must be a dereferenceable iterator in *this or be
   //!   before_begin(), and before_f and before_l belong to x and
   //!   ++before_f != x.end() && before_l != x.end() and
   //!   n == distance(before_f, before_l).
   //!
   //! <b>Effects</b>: Transfers the range (before_f, before_l] from list x to this
   //!   list, after the element pointed by prev_pos. No destructors or copy constructors are called.
   //!
   //! <b>Throws</b>: Nothing.
   //!
   //! <b>Complexity</b>: Constant time.
   //!
   //! <b>Note</b>: Iterators of values obtained from list x now point to elements of this
   //!   list. Iterators of this list and all the references are not invalidated.
   BOOST_INTRUSIVE_FORCEINLINE void splice_after(const_iterator prev_pos, slist_impl &x, const_iterator before_f, const_iterator before_l, size_type n) BOOST_NOEXCEPT
   {
      node_ops::splice_after
         ( prev_pos.pointed_node(), this->priv_last_ptr(), this->priv_size_traits(), before_f.pointed_node(), before_l.pointed_node(), n
         , x.priv_last_ptr(), x.priv_size_traits());
   }

   //! <b>Requires</b>: it is an iterator to an element in *this. &x != this.
   //!
   //! <b>Effects</b>: Transfers all the elements of list x to this list, before the
   //! the element pointed by it. No destructors or copy constructors are called.
   //!
   //! <b>Returns</b>: Nothing.
   //!
   //! <b>Throws</b>: Nothing.
   //!
   //! <b>Complexity</b>: Linear to the elements contained in x plus linear to
   //!   the elements before it.
   //!   Linear to the elements before it if cache_last<> option is true.
   //!   Constant-time if cache_last<> option is true and it == end().
   //!
   //! <b>Note</b>: Iterators of values obtained from list x now point to elements of this
   //! list. Iterators of this list and all the references are not invalidated.
   //!
   //! <b>Additional note</b>: If the optional parameter "l" is provided, it will be
   //!   assigned to the last spliced element or to previous(it) if x is empty.
   //!   This iterator can be used as new "prev" iterator for a new splice_after call.
   //!   that will splice new values after the previously spliced values.
   BOOST_INTRUSIVE_FORCEINLINE void splice(const_iterator it, slist_impl &x, const_iterator *l = 0) BOOST_NOEXCEPT
   {  this->splice_after(this->previous(it), x, l);   }

   //! <b>Requires</b>: pos must be a valid iterator of *this.
   //!   elem must point to an element contained in list
   //!   x.
   //!
   //! <b>Effects</b>: Transfers the element elem, from list x to this list,
   //!   before the element pointed by pos. No destructors or copy constructors are called.
   //!
   //! <b>Throws</b>: Nothing.
   //!
   //! <b>Complexity</b>: Linear to the elements before pos and before elem.
   //!   Linear to the elements before elem if cache_last<> option is true and pos == end().
   //!
   //! <b>Note</b>: Iterators of values obtained from list x now point to elements of this
   //! list. Iterators of this list and all the references are not invalidated.
   BOOST_INTRUSIVE_FORCEINLINE void splice(const_iterator pos, slist_impl &x, const_iterator elem) BOOST_NOEXCEPT
   {  return this->splice_after(this->previous(pos), x, x.previous(elem));  }

   //! <b>Requires</b>: pos must be a dereferenceable iterator in *this
   //!   and f and l belong to x and [f, l) is a valid range on x.
   //!
   //! <b>Effects</b>: Transfers the range [f, l) from list x to this
   //!   list, before the element pointed by pos.
   //!   No destructors or copy constructors are called.
   //!
   //! <b>Throws</b>: Nothing.
   //!
   //! <b>Complexity</b>: Linear to the sum of elements before pos, f, and l
   //!   plus linear to the number of elements transferred if constant_time_size is true.
   //!   Linear to the sum of elements before f, and l
   //!   plus linear to the number of elements transferred if constant_time_size is true
   //!   if cache_last<> is true and pos == end()
   //!
   //! <b>Note</b>: Iterators of values obtained from list x now point to elements of this
   //!   list. Iterators of this list and all the references are not invalidated.
   BOOST_INTRUSIVE_FORCEINLINE void splice(const_iterator pos, slist_impl &x, const_iterator f, const_iterator l) BOOST_NOEXCEPT
   {  return this->splice_after(this->previous(pos), x, x.previous(f), x.previous(l));  }

   //! <b>Requires</b>: pos must be a dereferenceable iterator in *this
   //!   and f and l belong to x and [f, l) is a valid range on x.
   //!   n == distance(f, l).
   //!
   //! <b>Effects</b>: Transfers the range [f, l) from list x to this
   //!   list, before the element pointed by pos.
   //!   No destructors or copy constructors are called.
   //!
   //! <b>Throws</b>: Nothing.
   //!
   //! <b>Complexity</b>: Linear to the sum of elements before pos, f, and l.
   //!   Linear to the sum of elements before f and l
   //!   if cache_last<> is true and pos == end().
   //!
   //! <b>Note</b>: Iterators of values obtained from list x now point to elements of this
   //!   list. Iterators of this list and all the references are not invalidated.
   BOOST_INTRUSIVE_FORCEINLINE void splice(const_iterator pos, slist_impl &x, const_iterator f, const_iterator l, size_type n) BOOST_NOEXCEPT
   {  return this->splice_after(this->previous(pos), x, x.previous(f), x.previous(l), n);  }

   //! <b>Requires</b>: p must be a comparison function that induces a strict weak ordering.
   //!
   //! <b>Effects</b>: This function sorts the list *this according to p.
   //!   The sort is stable, that is, the relative order of equivalent elements is preserved.
   //!
   //! <b>Throws</b>: If value_traits::node_traits::node
   //!   constructor throws (this does not happen with predefined Boost.Intrusive hooks)
   //!   or the predicate throws. Basic guarantee.
   //!
   //! <b>Complexity</b>: The number of comparisons is approximately N log N, where N
   //!   is the list's size.
   //!
   //! <b>Note</b>: Iterators and references are not invalidated
   template<class Predicate>
   void sort(Predicate p)
   {
      detail::key_nodeptr_comp<Predicate, value_traits> comp(p, &this->priv_value_traits());
      last_algorithms::sort(this->get_root_node(), comp, this->priv_last_ptr());
   }

   //! <b>Effects</b>: This function sorts the list *this according to operator<.
   //!   The sort is stable, that is, the relative order of equivalent elements is preserved.
   //!
   //! <b>Throws</b>: If value_traits::node_traits::node
   //!   constructor throws (this does not happen with predefined Boost.Intrusive hooks)
   //!   or operator< throws. Basic guarantee.
   //!
   //! <b>Complexity</b>: The number of comparisons is approximately N log N, where N
   //!   is the list's size.
   //!
   //! <b>Note</b>: Iterators and references are not invalidated.
   BOOST_INTRUSIVE_FORCEINLINE void sort()
   { this->sort(value_less<value_type>()); }

   //! <b>Requires</b>: p must be a comparison function that induces a strict weak
   //!   ordering and both *this and x must be sorted according to that ordering
   //!
   //! <b>Effects</b>: If &x == this, there are no effects. Otherwise, this function
   //!   removes all of x's elements and inserts them in order into *this.
   //!   The merge is stable; that is, if an element from *this is
   //!   equivalent to one from x, then the element from *this will precede the one from x.
   //!
   //! <b>Returns</b>: Nothing.
   //!
   //! <b>Throws</b>: If the predicate throws. Basic guarantee.
   //!
   //! <b>Complexity</b>: This function is linear time: it performs at most
   //!   size() + x.size() - 1 comparisons.
   //!
   //! <b>Note</b>: Iterators and references are not invalidated.
   //!
   //! <b>Additional note</b>: If optional "l" argument is passed, it is assigned
   //! to an iterator to the last transferred value or end() if x is empty or &x == this.
   template<class Predicate>
   void merge(slist_impl& x, Predicate p, const_iterator *l = 0)
   {
      if(BOOST_UNLIKELY(&x == this)){
         if(l) *l = this->cend();
         return;
      }
      detail::key_nodeptr_comp<Predicate, value_traits> comp(p, &this->priv_value_traits());
      node_ptr last_x = node_ptr();
      BOOST_INTRUSIVE_TRY{
         //All nodes of x are moved to *this even if the predicate throws
         last_x = node_ops::merge
            ( this->get_root_node(), this->priv_last_ptr(), this->priv_size_traits()
            , x.get_root_node(), x.priv_last_ptr(), x.priv_size_traits(), comp);
      }
      BOOST_INTRUSIVE_CATCH(...){
         if(l) *l = this->cend().unconst();
         BOOST_INTRUSIVE_RETHROW;
      }
      BOOST_INTRUSIVE_CATCH_END
      if(l) *l = last_x != node_ptr() ? const_iterator(last_x, this->priv_value_traits_ptr()) : this->cend();
   }

   //! <b>Effects</b>: If &x == this, there are no effects. Otherwise, this function
   //!   removes all of x's elements and inserts them
   //!   in order into *this according to operator<. The merge is stable;
   //!   that is, if an element from *this is equivalent to one from x, then the element
   //!   from *this will precede the one from x.
   //!
   //! <b>Throws</b>: if operator< throws. Basic guarantee.
   //!
   //! <b>Complexity</b>: This function is linear time: it performs at most
   //!   size() + x.size() - 1 comparisons.
   //!
   //! <b>Note</b>: Iterators and references are not invalidated
   BOOST_INTRUSIVE_FORCEINLINE void merge(slist_impl& x)
   {  this->merge(x, value_less<value_type>());  }

   //! <b>Effects</b>: Reverses the order of elements in the list.
   //!
   //! <b>Throws</b>: Nothing.
   //!
   //! <b>Complexity</b>: This function is linear to the contained elements.
   //!
   //! <b>Note</b>: Iterators and references are not invalidated
   #if defined(BOOST_INTRUSIVE_DOXYGEN_INVOKED)
   void reverse() BOOST_NOEXCEPT;
   #else
   using base_t::reverse;
   #endif

   //! <b>Effects</b>: Removes all the elements that compare equal to value.
   //!   No destructors are called.
   //!
   //! <b>Throws</b>: If operator== throws. Basic guarantee.
   //!
   //! <b>Complexity</b>: Linear time. It performs exactly size() comparisons for equality.
   //!
   //! <b>Note</b>: The relative order of elements that are not removed is unchanged,
   //!   and iterators to elements that are not removed remain valid. This function is
   //!   linear time: it performs exactly size() comparisons for equality.
   BOOST_INTRUSIVE_FORCEINLINE size_type remove(const_reference value)
   {  return this->remove_if(detail::equal_to_value<const_reference>(value));  }

   //! <b>Requires</b>: Disposer::operator()(pointer) and the copy and move
   //!   constructors of Disposer shouldn't throw.
   //!
   //! <b>Effects</b>: Removes all the elements that compare equal to value.
   //!   Disposer::operator()(pointer) is called for every removed element.
   //!
   //! <b>Throws</b>: If operator== throws. Basic guarantee.
   //!
   //! <b>Complexity</b>: Linear time. It performs exactly size() comparisons for equality.
   //!
   //! <b>Note</b>: The relative order of elements that are not removed is unchanged,
   //!   and iterators to elements that are not removed remain valid.
   template<class Disposer>
   BOOST_INTRUSIVE_FORCEINLINE size_type remove_and_dispose(const_reference value, Disposer disposer)
   {  return this->remove_and_dispose_if(detail::equal_to_value<const_reference>(value), disposer);  }

   //! <b>Effects</b>: Removes all the elements for which a specified
   //!   predicate is satisfied. No destructors are called.
   //!
   //! <b>Throws</b>: If pred throws. Basic guarantee.
   //!
   //! <b>Complexity</b>: Linear time. It performs exactly size() calls to the predicate.
   //!
   //! <b>Note</b>: The relative order of elements that are not removed is unchanged,
   //!   and iterators to elements that are not removed remain valid.
   template<class Pred>
   size_type remove_if(Pred pred)
   {
      const node_ptr bbeg = this->get_root_node();
      typename last_algorithms::stable_partition_info info;
      //The cached last node is updated, so slist invariants are preserved...
      last_algorithms::stable_partition
         (bbeg, detail::key_nodeptr_comp<Pred, value_traits>(pred, &this->priv_value_traits()), info, this->priv_last_ptr());
      //...and erase can be safely called
      node_ops::erase_after(bbeg, info.beg_2st_partition, info.num_1st_partition, this->priv_last_ptr(), this->priv_size_traits());
      return info.num_1st_partition;
   }

   //! <b>Requires</b>: Disposer::operator()(pointer) and the copy and move
   //!   constructors of Disposer shouldn't throw.
   //!
   //! <b>Effects</b>: Removes all the elements for which a specified
   //!   predicate is satisfied.
   //!   Disposer::operator()(pointer) is called for every removed element.
   //!
   //! <b>Throws</b>: If pred throws. Basic guarantee.
   //!
   //! <b>Complexity</b>: Linear time. It performs exactly size() calls to the predicate.
   //!
   //! <b>Note</b>: The relative order of elements that are not removed is unchanged,
   //!   and iterators to elements that are not removed remain valid.
   template<class Pred, class Disposer>
   size_type remove_and_dispose_if(Pred pred, Disposer disposer)
   {
      const node_ptr bbeg = this->get_root_node();
      typename last_algorithms::stable_partition_info info;
      //The cached last node is updated, so slist invariants are preserved...
      last_algorithms::stable_partition
         (bbeg, detail::key_nodeptr_comp<Pred, value_traits>(pred, &this->priv_value_traits()), info, this->priv_last_ptr());
      //...and erase can be safely called
      this->erase_after_and_dispose( const_iterator(bbeg, this->priv_value_traits_ptr())
                                   , const_iterator(info.beg_2st_partition, this->priv_value_traits_ptr())
                                   , disposer);
      return info.num_1st_partition;
   }

   //! <b>Effects</b>: Removes adjacent duplicate elements or adjacent
   //!   elements that are equal from the list. No destructors are called.
   //!
   //! <b>Returns</b>: The number of removed elements.
   //!
   //! <b>Throws</b>: If operator== throws. Basic guarantee.
   //!
   //! <b>Complexity</b>: Linear time (size()-1 equality comparisons).
   //!
   //! <b>Note</b>: The relative order of elements that are not removed is unchanged,
   //!   and iterators to elements that are not removed remain valid.
   BOOST_INTRUSIVE_FORCEINLINE size_type unique()
   {  return this->unique_and_dispose(value_equal<value_type>(), detail::null_disposer());  }

   //! <b>Effects</b>: Removes adjacent duplicate elements or adjacent
   //!   elements that satisfy some binary predicate from the list.
   //!   No destructors are called.
   //!
   //! <b>Returns</b>: The number of removed elements.
   //!
   //! <b>Throws</b>: If the predicate throws. Basic guarantee.
   //!
   //! <b>Complexity</b>: Linear time (size()-1) comparisons equality comparisons.
   //!
   //! <b>Note</b>: The relative order of elements that are not removed is unchanged,
   //!   and iterators to elements that are not removed remain valid.
   template<class BinaryPredicate>
   BOOST_INTRUSIVE_FORCEINLINE size_type unique(BinaryPredicate pred)
   {  return this->unique_and_dispose(pred, detail::null_disposer());  }

   //! <b>Requires</b>: Disposer::operator()(pointer) and the copy and move
   //!   constructors of Disposer shouldn't throw.
   //!
   //! <b>Effects</b>: Removes adjacent duplicate elements or adjacent
   //!   elements that satisfy some binary predicate from the list.
   //!   Disposer::operator()(pointer) is called for every removed element.
   //!
   //! <b>Returns</b>: The number of removed elements.
   //!
   //! <b>Throws</b>: If operator== throws. Basic guarantee.
   //!
   //! <b>Complexity</b>: Linear time (size()-1) comparisons equality comparisons.
   //!
   //! <b>Note</b>: The relative order of elements that are not removed is unchanged,
   //!   and iterators to elements that are not removed remain valid.
   template<class Disposer>
   BOOST_INTRUSIVE_FORCEINLINE size_type unique_and_dispose(Disposer disposer)
   {  return this->unique_and_dispose(value_equal<value_type>(), disposer);  }

   //! <b>Requires</b>: Disposer::operator()(pointer) and the copy and move
   //!   constructors of Disposer shouldn't throw.
   //!
   //! <b>Effects</b>: Removes adjacent duplicate elements or adjacent
   //!   elements that satisfy some binary predicate from the list.
   //!   Disposer::operator()(pointer) is called for every removed element.
   //!
   //! <b>Throws</b>: If the predicate throws. Basic guarantee.
   //!
   //! <b>Complexity</b>: Linear time (size()-1) comparisons equality comparisons.
   //!
   //! <b>Note</b>: The relative order of elements that are not removed is unchanged,
   //!   and iterators to elements that are not removed remain valid.
   template<class BinaryPredicate, class Disposer>
   size_type unique_and_dispose(BinaryPredicate pred, Disposer disposer)
   {
      const_iterator end_n(this->cend());
      const_iterator bcur(this->cbegin());
      size_type n = 0;

      if(bcur != end_n){
         const_iterator cur(bcur);
         ++cur;
         while(cur != end_n) {
            if (pred(*bcur, *cur)){
               cur = this->erase_after_and_dispose(bcur, disposer);
               ++n;
            }
            else{
               bcur = cur;
               ++cur;
            }
         }
      }
      return n;
   }

   //! <b>Requires</b>: `value` must be a reference to a value inserted in an instance of this container type.
   //!
   //! <b>Effects</b>: This function returns an iterator pointing to the element.
   //!
   //! <b>Throws</b>: Nothing.
   //!
   //! <b>Complexity</b>: Constant time.
   //!
   //! <b>Note</b>: Iterators and references are not invalidated.
   //!   This static function is available only if the <i>value traits</i>
   //!   is stateless.
   static iterator s_iterator_to(reference value) BOOST_NOEXCEPT
   {
      BOOST_INTRUSIVE_STATIC_ASSERT((!stateful_value_traits));
      return iterator (value_traits::to_node_ptr(value), const_value_traits_ptr());
   }

   //! <b>Requires</b>: value must be a const reference to a value inserted in a list.
   //!
   //! <b>Effects</b>: This function returns a const_iterator pointing to the element.
   //!
   //! <b>Throws</b>: Nothing.
   //!
   //! <b>Complexity</b>: Constant time.
   //!
   //! <b>Note</b>: Iterators and references are not invalidated.
   //!   This static function is available only if the <i>value traits</i>
   //!   is stateless.
   static const_iterator s_iterator_to(const_reference value) BOOST_NOEXCEPT
   {
      BOOST_INTRUSIVE_STATIC_ASSERT((!stateful_value_traits));
      reference r =*detail::uncast(pointer_traits<const_pointer>::pointer_to(value));
      return const_iterator(value_traits::to_node_ptr(r), const_value_traits_ptr());
   }

   //! <b>Requires</b>: `value` must be a reference to a value inserted in an instance of this container type.
   //!
   //! <b>Effects</b>: This function returns an iterator pointing to the element.
   //!
   //! <b>Throws</b>: Nothing.
   //!
   //! <b>Complexity</b>: Constant time.
   //!
   //! <b>Note</b>: Iterators and references are not invalidated.
   iterator iterator_to(reference value) BOOST_NOEXCEPT
   {
      BOOST_INTRUSIVE_INVARIANT_ASSERT(linear || !node_algorithms::inited(this->priv_value_traits().to_node_ptr(value)));
      return iterator (this->priv_value_traits().to_node_ptr(value), this->priv_value_traits_ptr());
   }

   //! <b>Requires</b>: value must be a const reference to a value inserted in a list.
   //!
   //! <b>Effects</b>: This function returns a const_iterator pointing to the element.
   //!
   //! <b>Throws</b>: Nothing.
   //!
   //! <b>Complexity</b>: Constant time.
   //!
   //! <b>Note</b>: Iterators and references are not invalidated.
   const_iterator iterator_to(const_reference value) const BOOST_NOEXCEPT
   {
      reference r =*detail::uncast(pointer_traits<const_pointer>::pointer_to(value));
      BOOST_INTRUSIVE_INVARIANT_ASSERT (linear || !node_algorithms::inited(this->priv_value_traits().to_node_ptr(r)));
      return const_iterator(this->priv_value_traits().to_node_ptr(r), this->priv_value_traits_ptr());
   }

   //! <b>Returns</b>: The iterator to the element before i in the list.
   //!   Returns before_begin(), if either i is the begin-iterator or the
   //!   list is empty.
   //!
   //! <b>Throws</b>: Nothing.
   //!
   //! <b>Complexity</b>: Linear to the number of elements before i.
   //!   Constant if cache_last<> is true and i == end().
   BOOST_INTRUSIVE_FORCEINLINE iterator previous(iterator i) BOOST_NOEXCEPT
   {  return this->previous(this->cbefore_begin(), i); }

   //! <b>Returns</b>: The const_iterator to the element before i in the list.
   //!   Returns before_begin(), if either i is the begin-const_iterator or
   //!   the list is empty.
   //!
   //! <b>Throws</b>: Nothing.
   //!
   //! <b>Complexity</b>: Linear to the number of elements before i.
   //!   Constant if cache_last<> is true and i == end().
   BOOST_INTRUSIVE_FORCEINLINE const_iterator previous(const_iterator i) const BOOST_NOEXCEPT
   {  return this->previous(this->cbefore_begin(), i); }

   //! <b>Returns</b>: The iterator to the element before i in the list,
   //!   starting the search on element after prev_from.
   //!   Returns before_begin(), if either i is the begin-iterator or the
   //!   list is empty.
   //!
   //! <b>Throws</b>: Nothing.
   //!
   //! <b>Complexity</b>: Linear to the number of elements before i.
   //!   Constant if cache_last<> is true and i == end().
   BOOST_INTRUSIVE_FORCEINLINE iterator previous(const_iterator prev_from, iterator i) BOOST_NOEXCEPT
   {  return this->previous(prev_from, const_iterator(i)).unconst(); }

   //! <b>Returns</b>: The const_iterator to the element before i in the list,
   //!   starting the search on element after prev_from.
   //!   Returns before_begin(), if either i is the begin-const_iterator or
   //!   the list is empty.
   //!
   //! <b>Throws</b>: Nothing.
   //!
   //! <b>Complexity</b>: Linear to the number of elements before i.
   //!   Constant if cache_last<> is true and i == end().
   const_iterator previous(const_iterator prev_from, const_iterator i) const BOOST_NOEXCEPT
   {
      if(cache_last && (i.pointed_node() == this->get_end_node())){
         return const_iterator(detail::uncast(this->get_last_node()), this->priv_value_traits_ptr());
      }
      return const_iterator
         (node_algorithms::get_previous_node
            (prev_from.pointed_node(), i.pointed_node()), this->priv_value_traits_ptr());
   }

   ///@cond

   //! <b>Requires</b>: prev_pos must be a dereferenceable iterator in *this or be
   //!   before_begin(), and f and before_l belong to another slist.
   //!
   //! <b>Effects</b>: Transfers the range [f, before_l] to this
   //!   list, after the element pointed by prev_pos.
   //!   No destructors or copy constructors are called.
   //!
   //! <b>Throws</b>: Nothing.
   //!
   //! <b>Complexity</b>: Linear to the number of elements transferred
   //!   if constant_time_size is true. Constant-time otherwise.
   //!
   //! <b>Note</b>: Iterators of values obtained from the list that owned f and before_l now
   //!   point to elements of this list. Iterators of this list and all the references are not invalidated.
   //!
   //! <b>Warning</b>: Experimental function, don't use it!
   BOOST_INTRUSIVE_FORCEINLINE void incorporate_after(const_iterator prev_pos, node_ptr f, node_ptr before_l) BOOST_NOEXCEPT
   {  node_ops::incorporate_after(prev_pos.pointed_node(), f, before_l, this->priv_last_ptr(), this->priv_size_traits());  }

   //! <b>Requires</b>: prev_pos must be a dereferenceable iterator in *this or be
   //!   before_begin(), and f and before_l belong to another slist.
   //!   n == distance(f, before_l) + 1.
   //!
   //! <b>Effects</b>: Transfers the range [f, before_l] to this
   //!   list, after the element pointed by prev_pos.
   //!   No destructors or copy constructors are called.
   //!
   //! <b>Throws</b>: Nothing.
   //!
   //! <b>Complexity</b>: Constant time.
   //!
   //! <b>Note</b>: Iterators of values obtained from the list that owned f and before_l now
   //!   point to elements of this list. Iterators of this list and all the references are not invalidated.
   //!
   //! <b>Warning</b>: Experimental function, don't use it!
   BOOST_INTRUSIVE_FORCEINLINE void incorporate_after(const_iterator prev_pos, node_ptr f, node_ptr before_l, size_type n) BOOST_NOEXCEPT
   {  node_ops::incorporate_after(prev_pos.pointed_node(), f, before_l, n, this->priv_last_ptr(), this->priv_size_traits());  }

   ///@endcond

   //! <b>Effects</b>: Asserts the integrity of the container.
   //!
   //! <b>Complexity</b>: Linear time.
   //!
   //! <b>Note</b>: The method has no effect when asserts are turned off (e.g., with NDEBUG).
   //!   Experimental function, interface might change in future versions.
   #if defined(BOOST_INTRUSIVE_DOXYGEN_INVOKED)
   void check() const;
   #else
   using base_t::check;
   #endif


   friend bool operator==(const slist_impl &x, const slist_impl &y)
   {
      BOOST_IF_CONSTEXPR(constant_time_size)
      if(x.size() != y.size()){
         return false;
      }
      return ::boost::intrusive::algo_equal(x.cbegin(), x.cend(), y.cbegin(), y.cend());
   }

   BOOST_INTRUSIVE_FORCEINLINE friend bool operator!=(const slist_impl &x, const slist_impl &y)
   {  return !(x == y); }

   BOOST_INTRUSIVE_FORCEINLINE friend bool operator<(const slist_impl &x, const slist_impl &y)
   {  return ::boost::intrusive::algo_lexicographical_compare(x.begin(), x.end(), y.begin(), y.end());  }

   BOOST_INTRUSIVE_FORCEINLINE friend bool operator>(const slist_impl &x, const slist_impl &y)
   {  return y < x;  }

   BOOST_INTRUSIVE_FORCEINLINE friend bool operator<=(const slist_impl &x, const slist_impl &y)
   {  return !(y < x);  }

   BOOST_INTRUSIVE_FORCEINLINE friend bool operator>=(const slist_impl &x, const slist_impl &y)
   {  return !(x < y);  }

   BOOST_INTRUSIVE_FORCEINLINE friend void swap(slist_impl &x, slist_impl &y) BOOST_NOEXCEPT
   {  x.swap(y);  }

   private:
   BOOST_INTRUSIVE_NO_DANGLING
   static slist_impl &priv_container_from_end_iterator(const const_iterator &end_iterator) BOOST_NOEXCEPT
   {
      //Obtaining the container from the end iterator is not possible with linear
      //singly linked lists (because "end" is represented by the null pointer)
      BOOST_INTRUSIVE_STATIC_ASSERT(!linear);
      BOOST_INTRUSIVE_STATIC_ASSERT((has_container_from_iterator));
      return static_cast<slist_impl&>(base_t::priv_base_from_end_node(end_iterator.pointed_node()));
   }

   //Used by insert_after(const_iterator, Iterator, Iterator) to implement rollback
   class range_insertion_fixer
   {
      public:
      range_insertion_fixer
         ( slist_impl &c, node_ptr prev_n, const node_ptr &first_n
         , const node_ptr &last_n, const size_type &count) BOOST_NOEXCEPT
         : c_(c), prev_n_(prev_n), first_n_(first_n), last_n_(last_n), count_(count)
      {}

      ~range_insertion_fixer()
      {
         node_ops::incorporate_after
            (prev_n_, first_n_, last_n_, count_, c_.priv_last_ptr(), c_.priv_size_traits());
      }

      private:
      slist_impl &c_;
      const node_ptr prev_n_;
      const node_ptr &first_n_;
      const node_ptr &last_n_;
      const size_type &count_;
   };
};

//! Helper metafunction to define a \c slist that yields to the same type when the
//! same options (either explicitly or implicitly) are used.
#if defined(BOOST_INTRUSIVE_DOXYGEN_INVOKED) || defined(BOOST_INTRUSIVE_VARIADIC_TEMPLATES)
template<class T, class ...Options>
#else
template<class T, class O1 = void, class O2 = void, class O3 = void, class O4 = void, class O5 = void, class O6 = void>
#endif
struct make_slist
{
   /// @cond
   typedef typename pack_options
      < slist_defaults,
         #if !defined(BOOST_INTRUSIVE_VARIADIC_TEMPLATES)
         O1, O2, O3, O4, O5, O6
         #else
         Options...
         #endif
      >::type packed_options;

   typedef typename detail::get_value_traits
      <T, typename packed_options::proto_value_traits>::type value_traits;
   typedef slist_impl
      < value_traits
      , typename packed_options::size_type
      ,  (std::size_t(packed_options::linear)*slist_bool_flags::linear_pos)
        |(std::size_t(packed_options::constant_time_size)*slist_bool_flags::constant_time_size_pos)
        |(std::size_t(packed_options::cache_last)*slist_bool_flags::cache_last_pos)
      , typename packed_options::header_holder_type
      > implementation_defined;
   /// @endcond
   typedef implementation_defined type;
};


#ifndef BOOST_INTRUSIVE_DOXYGEN_INVOKED

#if !defined(BOOST_INTRUSIVE_VARIADIC_TEMPLATES)
template<class T, class O1, class O2, class O3, class O4, class O5, class O6>
#else
template<class T, class ...Options>
#endif
class slist
   :  public make_slist<T,
         #if !defined(BOOST_INTRUSIVE_VARIADIC_TEMPLATES)
         O1, O2, O3, O4, O5, O6
         #else
         Options...
         #endif
      >::type
{
   typedef typename make_slist
      <T,
      #if !defined(BOOST_INTRUSIVE_VARIADIC_TEMPLATES)
      O1, O2, O3, O4, O5, O6
      #else
      Options...
      #endif
      >::type   Base;
   //Assert if passed value traits are compatible with the type
   BOOST_INTRUSIVE_STATIC_ASSERT((detail::is_same<typename Base::value_traits::value_type, T>::value));
   BOOST_MOVABLE_BUT_NOT_COPYABLE(slist)

   public:
   typedef typename Base::value_traits       value_traits;
   typedef typename Base::iterator           iterator;
   typedef typename Base::const_iterator     const_iterator;
   typedef typename Base::size_type          size_type;
   typedef typename Base::node_ptr           node_ptr;

   struct incorporate_t{};

   #if defined(BOOST_INTRUSIVE_HAS_INHERITING_CONSTRUCTORS)
   //The inherited constructors and the defaulted operations get the noexcept of Base
   using Base::Base;
   slist() = default;
   slist(slist &&) = default;
   slist& operator=(slist &&) = default;
   #else
   BOOST_INTRUSIVE_FORCEINLINE slist()
      BOOST_NOEXCEPT_IF(::boost::move_detail::is_nothrow_default_constructible<Base>::value)
      :  Base()
   {}

   BOOST_INTRUSIVE_FORCEINLINE explicit slist(const value_traits &v_traits)
      BOOST_NOEXCEPT_IF(::boost::move_detail::is_nothrow_copy_constructible<value_traits>::value)
      :  Base(v_traits)
   {}

   inline slist( node_ptr f, node_ptr before_l
             , size_type n, const value_traits &v_traits = value_traits())
      :  Base(f, before_l, n, v_traits)
   {}

   template<class Iterator>
   BOOST_INTRUSIVE_FORCEINLINE slist(Iterator b, Iterator e, const value_traits &v_traits = value_traits())
      BOOST_NOEXCEPT_IF(::boost::move_detail::is_nothrow_copy_constructible<value_traits>::value &&
                        (detail::is_nothrow_range_iterator<Iterator, typename Base::reference>::value))
      :  Base(b, e, v_traits)
   {}

   BOOST_INTRUSIVE_FORCEINLINE slist(BOOST_RV_REF(slist) x)
      BOOST_NOEXCEPT_IF(::boost::move_detail::is_nothrow_move_constructible<Base>::value)
      :  Base(BOOST_MOVE_BASE(Base, x))
   {}

   BOOST_INTRUSIVE_FORCEINLINE slist& operator=(BOOST_RV_REF(slist) x)
   #if !defined(BOOST_INTRUSIVE_MOVE_ASSIGNMENT_SWAPS)
      BOOST_NOEXCEPT_IF(::boost::move_detail::is_nothrow_move_assignable<Base>::value)
   #endif
   {  return static_cast<slist &>(this->Base::operator=(BOOST_MOVE_BASE(Base, x)));  }
   #endif

   /// @cond
   BOOST_INTRUSIVE_FORCEINLINE friend void swap(slist &x, slist &y)
      BOOST_NOEXCEPT
   {  x.swap(y);  }
   /// @endcond

   template <class Cloner, class Disposer>
   BOOST_INTRUSIVE_FORCEINLINE void clone_from(const slist &src, Cloner cloner, Disposer disposer)
   {  Base::clone_from(src, cloner, disposer);  }

   template <class Cloner, class Disposer>
   BOOST_INTRUSIVE_FORCEINLINE void clone_from(BOOST_RV_REF(slist) src, Cloner cloner, Disposer disposer)
   {  Base::clone_from(BOOST_MOVE_BASE(Base, src), cloner, disposer);  }

   BOOST_INTRUSIVE_NO_DANGLING
   BOOST_INTRUSIVE_FORCEINLINE static slist &container_from_end_iterator(iterator end_iterator) BOOST_NOEXCEPT
   {  return static_cast<slist &>(Base::container_from_end_iterator(end_iterator));   }

   BOOST_INTRUSIVE_NO_DANGLING
   BOOST_INTRUSIVE_FORCEINLINE static const slist &container_from_end_iterator(const_iterator end_iterator) BOOST_NOEXCEPT
   {  return static_cast<const slist &>(Base::container_from_end_iterator(end_iterator));   }
};

#endif

} //namespace intrusive
} //namespace boost

#include <boost/intrusive/detail/config_end.hpp>

#endif //BOOST_INTRUSIVE_SLIST_HPP
