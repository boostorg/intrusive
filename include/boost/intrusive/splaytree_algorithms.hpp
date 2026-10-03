/////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Ion Gaztanaga  2007-2014
//
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/intrusive for documentation.
//
/////////////////////////////////////////////////////////////////////////////
// The implementation of splay trees is based on the article and code published
// in C++ Users Journal "Implementing Splay Trees in C++" (September 1, 2005).
//
// The splay code has been modified and (supposedly) improved by Ion Gaztanaga.
//
// Here is the copyright notice of the original file containing the splay code:
//
//  splay_tree.h -- implementation of a STL compatible splay tree.
//
//  Copyright (c) 2004 Ralf Mattethat
//
//  Permission to copy, use, modify, sell and distribute this software
//  is granted provided this copyright notice appears in all copies.
//  This software is provided "as is" without express or implied
//  warranty, and with no claim as to its suitability for any purpose.
//
/////////////////////////////////////////////////////////////////////////////

#ifndef BOOST_INTRUSIVE_SPLAYTREE_ALGORITHMS_HPP
#define BOOST_INTRUSIVE_SPLAYTREE_ALGORITHMS_HPP

#include <boost/intrusive/detail/config_begin.hpp>
#include <boost/intrusive/intrusive_fwd.hpp>
#include <boost/intrusive/detail/assert.hpp>
#include <boost/intrusive/detail/algo_type.hpp>
#include <boost/intrusive/detail/uncast.hpp>
#include <boost/intrusive/bstree_algorithms.hpp>

#include <cstddef>

#if defined(BOOST_HAS_PRAGMA_ONCE)
#  pragma once
#endif

namespace boost {
namespace intrusive {

/// @cond
namespace detail {

template<class NodeTraits>
struct splaydown_assemble_and_fix_header
{
   typedef typename NodeTraits::node_ptr node_ptr;

   splaydown_assemble_and_fix_header(node_ptr t, node_ptr header, node_ptr leftmost, node_ptr rightmost) BOOST_NOEXCEPT
      : t_(t)
      , null_node_(header)
      , l_(null_node_)
      , r_(null_node_)
      , leftmost_(leftmost)
      , rightmost_(rightmost)
   {}

   ~splaydown_assemble_and_fix_header()
   {
      this->assemble();

      //Now recover the original header except for the
      //splayed root node.
      //"t_" is the current root and "null_node_" is the header node
      NodeTraits::set_parent(null_node_, t_);
      NodeTraits::set_parent(t_, null_node_);
      //Recover leftmost/rightmost pointers
      NodeTraits::set_left (null_node_, leftmost_);
      NodeTraits::set_right(null_node_, rightmost_);
   }

   private:

   void assemble() BOOST_NOEXCEPT
   {
      //procedure assemble;
      //    left(r), right(l) := right(t), left(t);
      //    left(t), right(t) := right(null), left(null);
      //end assemble;
      {  //    left(r), right(l) := right(t), left(t);

         node_ptr const old_t_left  = NodeTraits::get_left(t_);
         node_ptr const old_t_right = NodeTraits::get_right(t_);
         NodeTraits::set_right(l_, old_t_left);
         NodeTraits::set_left (r_, old_t_right);
         if(old_t_left){
            NodeTraits::set_parent(old_t_left, l_);
         }
         if(old_t_right){
            NodeTraits::set_parent(old_t_right, r_);
         }
      }
      {  //    left(t), right(t) := right(null), left(null);
         node_ptr const null_right = NodeTraits::get_right(null_node_);
         node_ptr const null_left  = NodeTraits::get_left(null_node_);
         NodeTraits::set_left (t_, null_right);
         NodeTraits::set_right(t_, null_left);
         if(null_right){
            NodeTraits::set_parent(null_right, t_);
         }
         if(null_left){
            NodeTraits::set_parent(null_left, t_);
         }
      }
   }

   public:
   node_ptr t_, null_node_, l_, r_, leftmost_, rightmost_;
};

}  //namespace detail {
/// @endcond

//!   A splay tree is an implementation of a binary search tree. The tree is
//!   self balancing using the splay algorithm as described in
//!
//!      "Self-Adjusting Binary Search Trees
//!      by Daniel Dominic Sleator and Robert Endre Tarjan
//!      AT&T Bell Laboratories, Murray Hill, NJ
//!      Journal of the ACM, Vol 32, no 3, July 1985, pp 652-686
//!
//! splaytree_algorithms is configured with a NodeTraits class, which encapsulates the
//! information about the node to be manipulated. NodeTraits must support the
//! following interface:
//!
//! <b>Typedefs</b>:
//!
//! <tt>node</tt>: The type of the node that forms the binary search tree
//!
//! <tt>node_ptr</tt>: A pointer to a node
//!
//! <tt>const_node_ptr</tt>: A pointer to a const node
//!
//! <b>Static functions</b>:
//!
//! <tt>static node_ptr get_parent(const_node_ptr n);</tt>
//!
//! <tt>static void set_parent(node_ptr n, node_ptr parent);</tt>
//!
//! <tt>static node_ptr get_left(const_node_ptr n);</tt>
//!
//! <tt>static void set_left(node_ptr n, node_ptr left);</tt>
//!
//! <tt>static node_ptr get_right(const_node_ptr n);</tt>
//!
//! <tt>static void set_right(node_ptr n, node_ptr right);</tt>
template<class NodeTraits>
class splaytree_algorithms
   #ifndef BOOST_INTRUSIVE_DOXYGEN_INVOKED
   : public bstree_algorithms<NodeTraits>
   #endif
{
   /// @cond
   private:
   typedef bstree_algorithms<NodeTraits> bstree_algo;
   /// @endcond

   public:
   typedef typename NodeTraits::node            node;
   typedef NodeTraits                           node_traits;
   typedef typename NodeTraits::node_ptr        node_ptr;
   typedef typename NodeTraits::const_node_ptr  const_node_ptr;

   //! This type is the information that will be
   //! filled by insert_unique_check
   typedef BOOST_INTRUSIVE_IMPDEF(typename bstree_algo::insert_commit_data)   insert_commit_data;

   public:
   #ifdef BOOST_INTRUSIVE_DOXYGEN_INVOKED
   //! @copydoc ::boost::intrusive::bstree_algorithms::get_header(const_node_ptr)
   static node_ptr get_header(const_node_ptr n) BOOST_NOEXCEPT;

   //! @copydoc ::boost::intrusive::bstree_algorithms::begin_node
   static node_ptr begin_node(const_node_ptr header) BOOST_NOEXCEPT;

   //! @copydoc ::boost::intrusive::bstree_algorithms::end_node
   static node_ptr end_node(const_node_ptr header) BOOST_NOEXCEPT;

   //! @copydoc ::boost::intrusive::bstree_algorithms::swap_tree
   static void swap_tree(node_ptr header1, node_ptr header2);

   //! @copydoc ::boost::intrusive::bstree_algorithms::swap_nodes(node_ptr,node_ptr)
   static void swap_nodes(node_ptr node1, node_ptr node2) BOOST_NOEXCEPT;

   //! @copydoc ::boost::intrusive::bstree_algorithms::swap_nodes(node_ptr,node_ptr,node_ptr,node_ptr)
   static void swap_nodes(node_ptr node1, node_ptr header1, node_ptr node2, node_ptr header2) BOOST_NOEXCEPT;

   //! @copydoc ::boost::intrusive::bstree_algorithms::replace_node(node_ptr,node_ptr)
   static void replace_node(node_ptr node_to_be_replaced, node_ptr new_node) BOOST_NOEXCEPT;

   //! @copydoc ::boost::intrusive::bstree_algorithms::replace_node(node_ptr,node_ptr,node_ptr)
   static void replace_node(node_ptr node_to_be_replaced, node_ptr header, node_ptr new_node) BOOST_NOEXCEPT;

   //! @copydoc ::boost::intrusive::bstree_algorithms::unlink(node_ptr)
   static void unlink(node_ptr n) BOOST_NOEXCEPT;

   //! @copydoc ::boost::intrusive::bstree_algorithms::unlink_leftmost_without_rebalance
   static node_ptr unlink_leftmost_without_rebalance(node_ptr header) BOOST_NOEXCEPT;

   //! @copydoc ::boost::intrusive::bstree_algorithms::unique(const_node_ptr)
   static bool unique(const_node_ptr n) BOOST_NOEXCEPT;

   //! @copydoc ::boost::intrusive::bstree_algorithms::size(const_node_ptr)
   static std::size_t size(const_node_ptr header) BOOST_NOEXCEPT;

   //! @copydoc ::boost::intrusive::bstree_algorithms::next_node(node_ptr)
   static node_ptr next_node(node_ptr n) BOOST_NOEXCEPT;

   //! @copydoc ::boost::intrusive::bstree_algorithms::prev_node(node_ptr)
   static node_ptr prev_node(node_ptr n) BOOST_NOEXCEPT;

   //! @copydoc ::boost::intrusive::bstree_algorithms::init(node_ptr)
   static void init(node_ptr n) BOOST_NOEXCEPT;

   //! @copydoc ::boost::intrusive::bstree_algorithms::init_header(node_ptr)
   static void init_header(node_ptr header) BOOST_NOEXCEPT;

   #endif   //#ifdef BOOST_INTRUSIVE_DOXYGEN_INVOKED

   //! @copydoc ::boost::intrusive::bstree_algorithms::erase(node_ptr,node_ptr)
   //!
   //! <b>Note</b>: If z has a left child, the previous node of z is splayed to speed up
   //!   range deletions.
   static void erase(node_ptr header, node_ptr z) BOOST_NOEXCEPT
   {
      //posibility 1

      //z is not header, so when z has a left child maximum(get_left(z)) is
      //the previous node so we can avoid the more expensive prev_node()
      node_ptr const z_left(NodeTraits::get_left(z));
      if(z_left){
         splay_up(bstree_algo::maximum(z_left), header);
      }

      //possibility 2
      //if(NodeTraits::get_left(z)){
      //   node_ptr l = NodeTraits::get_left(z);
      //   splay_up(l, header);
      //}

      //if(NodeTraits::get_left(z)){
      //   node_ptr l = bstree_algo::prev_node(z);
      //   splay_up_impl(l, z);
      //}

      //possibility 4
      //splay_up(z, header);

      bstree_algo::erase(header, z);
   }

   //! @copydoc ::boost::intrusive::bstree_algorithms::transfer_unique
   template<class NodePtrCompare>
   static bool transfer_unique
      (node_ptr header1, NodePtrCompare comp, node_ptr header2, node_ptr z)
   {
      typename bstree_algo::insert_commit_data commit_data;
      bool const transferable = bstree_algo::insert_unique_check(header1, z, comp, commit_data).second;
      if(transferable){
         erase(header2, z);
         bstree_algo::insert_commit(header1, z, commit_data);
         splay_up(z, header1);
      }
      return transferable;
   }

   //! @copydoc ::boost::intrusive::bstree_algorithms::transfer_equal
   template<class NodePtrCompare>
   static void transfer_equal
      (node_ptr header1, NodePtrCompare comp, node_ptr header2, node_ptr z)
   {
      insert_commit_data commit_data;
      priv_insert_equal_check<true>(header1, z, comp, commit_data);
      erase(header2, z);
      bstree_algo::insert_commit(header1, z, commit_data);
   }

   #ifdef BOOST_INTRUSIVE_DOXYGEN_INVOKED
   //! @copydoc ::boost::intrusive::bstree_algorithms::clone(const_node_ptr,node_ptr,Cloner,Disposer)
   template <class Cloner, class Disposer>
   static void clone
      (const_node_ptr source_header, node_ptr target_header, Cloner cloner, Disposer disposer);

   //! @copydoc ::boost::intrusive::bstree_algorithms::clear_and_dispose(node_ptr,Disposer)
   template<class Disposer>
   static void clear_and_dispose(node_ptr header, Disposer disposer) BOOST_NOEXCEPT;

   #endif   //#ifdef BOOST_INTRUSIVE_DOXYGEN_INVOKED
   //! @copydoc ::boost::intrusive::bstree_algorithms::count(const_node_ptr,const KeyType&,KeyNodePtrCompare)
   //!
   //! <b>Note</b>: A node with a key equivalent to `key` is splayed. If there is no such
   //!   node, the node immediately before or after the position of `key` is splayed.
   template<class KeyType, class KeyNodePtrCompare>
   static std::size_t count
      (node_ptr header, const KeyType &key, KeyNodePtrCompare comp)
   {
      std::pair<node_ptr, node_ptr> ret = equal_range(header, key, comp);
      std::size_t n = 0;
      while(ret.first != ret.second){
         ++n;
         ret.first = bstree_algo::next_node(ret.first);
      }
      return n;
   }

   //! @copydoc ::boost::intrusive::bstree_algorithms::count(const_node_ptr,const KeyType&,KeyNodePtrCompare)
   //!
   //! <b>Note</b>: No splaying is performed.
   template<class KeyType, class KeyNodePtrCompare>
   static std::size_t count
      (const_node_ptr header, const KeyType &key, KeyNodePtrCompare comp)
   {  return bstree_algo::count(header, key, comp);  }

   //! @copydoc ::boost::intrusive::bstree_algorithms::lower_bound(const_node_ptr,const KeyType&,KeyNodePtrCompare)
   //!
   //! <b>Note</b>: A node with a key equivalent to `key` is splayed. If there is no such
   //!   node, the node immediately before or after the position of `key` is splayed.
   template<class KeyType, class KeyNodePtrCompare>
   static node_ptr lower_bound
      (node_ptr header, const KeyType &key, KeyNodePtrCompare comp)
   {
      bool found, before;
      node_ptr const r = priv_splay_down(detail::uncast(header), key, comp, found, before);
      return found ? bstree_algo::lower_bound_loop(NodeTraits::get_left(r), r, key, comp)
                   : priv_bound_from_root(header, r, before);
   }

   //! @copydoc ::boost::intrusive::bstree_algorithms::lower_bound(const_node_ptr,const KeyType&,KeyNodePtrCompare)
   //!
   //! <b>Note</b>: No splaying is performed.
   template<class KeyType, class KeyNodePtrCompare>
   static node_ptr lower_bound
      (const_node_ptr header, const KeyType &key, KeyNodePtrCompare comp)
   {  return bstree_algo::lower_bound(header, key, comp);  }

   //! @copydoc ::boost::intrusive::bstree_algorithms::upper_bound(const_node_ptr,const KeyType&,KeyNodePtrCompare)
   //!
   //! <b>Note</b>: A node with a key equivalent to `key` is splayed. If there is no such
   //!   node, the node immediately before or after the position of `key` is splayed.
   template<class KeyType, class KeyNodePtrCompare>
   static node_ptr upper_bound
      (node_ptr header, const KeyType &key, KeyNodePtrCompare comp)
   {
      bool found, before;
      node_ptr const r = priv_splay_down(detail::uncast(header), key, comp, found, before);
      return found ? bstree_algo::upper_bound_loop(NodeTraits::get_right(r), header, key, comp)
                   : priv_bound_from_root(header, r, before);
   }

   //! @copydoc ::boost::intrusive::bstree_algorithms::upper_bound(const_node_ptr,const KeyType&,KeyNodePtrCompare)
   //!
   //! <b>Note</b>: No splaying is performed.
   template<class KeyType, class KeyNodePtrCompare>
   static node_ptr upper_bound
      (const_node_ptr header, const KeyType &key, KeyNodePtrCompare comp)
   {  return bstree_algo::upper_bound(header, key, comp);  }

   //! @copydoc ::boost::intrusive::bstree_algorithms::find(const_node_ptr, const KeyType&,KeyNodePtrCompare)
   //!
   //! <b>Note</b>: A node with a key equivalent to `key` is splayed. If there is no such
   //!   node, the node immediately before or after the position of `key` is splayed.
   template<class KeyType, class KeyNodePtrCompare>
   static node_ptr find
      (node_ptr header, const KeyType &key, KeyNodePtrCompare comp)
   {
      bool found, before;
      node_ptr const r = priv_splay_down(detail::uncast(header), key, comp, found, before);
      return found ? bstree_algo::lower_bound_loop(NodeTraits::get_left(r), r, key, comp) : header;
   }

   //! @copydoc ::boost::intrusive::bstree_algorithms::find(const_node_ptr, const KeyType&,KeyNodePtrCompare)
   //!
   //! <b>Note</b>: No splaying is performed.
   template<class KeyType, class KeyNodePtrCompare>
   static node_ptr find
      (const_node_ptr header, const KeyType &key, KeyNodePtrCompare comp)
   {  return bstree_algo::find(header, key, comp);  }

   //! @copydoc ::boost::intrusive::bstree_algorithms::equal_range(const_node_ptr,const KeyType&,KeyNodePtrCompare)
   //!
   //! <b>Note</b>: A node with a key equivalent to `key` is splayed. If there is no such
   //!   node, the node immediately before or after the position of `key` is splayed.
   template<class KeyType, class KeyNodePtrCompare>
   static std::pair<node_ptr, node_ptr> equal_range
      (node_ptr header, const KeyType &key, KeyNodePtrCompare comp)
   {
      bool found, before;
      node_ptr const r = priv_splay_down(detail::uncast(header), key, comp, found, before);
      if(found)
         return std::pair<node_ptr, node_ptr>
            ( bstree_algo::lower_bound_loop(NodeTraits::get_left(r), r, key, comp)
            , bstree_algo::upper_bound_loop(NodeTraits::get_right(r), header, key, comp));
      node_ptr const b = priv_bound_from_root(header, r, before);
      return std::pair<node_ptr, node_ptr>(b, b);
   }

   //! @copydoc ::boost::intrusive::bstree_algorithms::equal_range(const_node_ptr,const KeyType&,KeyNodePtrCompare)
   //!
   //! <b>Note</b>: No splaying is performed.
   template<class KeyType, class KeyNodePtrCompare>
   static std::pair<node_ptr, node_ptr> equal_range
      (const_node_ptr header, const KeyType &key, KeyNodePtrCompare comp)
   {  return bstree_algo::equal_range(header, key, comp);  }

   //! @copydoc ::boost::intrusive::bstree_algorithms::lower_bound_range(const_node_ptr,const KeyType&,KeyNodePtrCompare)
   //!
   //! <b>Note</b>: A node with a key equivalent to `key` is splayed. If there is no such
   //!   node, the node immediately before or after the position of `key` is splayed.
   template<class KeyType, class KeyNodePtrCompare>
   static std::pair<node_ptr, node_ptr> lower_bound_range
      (node_ptr header, const KeyType &key, KeyNodePtrCompare comp)
   {
      bool found, before;
      node_ptr const r = priv_splay_down(detail::uncast(header), key, comp, found, before);
      if(found){
         node_ptr const lb = bstree_algo::lower_bound_loop(NodeTraits::get_left(r), r, key, comp);
         return std::pair<node_ptr, node_ptr>(lb, lb == r ? priv_next_of_root(header, r) : bstree_algo::next_node(lb));
      }
      node_ptr const b = priv_bound_from_root(header, r, before);
      return std::pair<node_ptr, node_ptr>(b, b);
   }

   //! @copydoc ::boost::intrusive::bstree_algorithms::lower_bound_range(const_node_ptr,const KeyType&,KeyNodePtrCompare)
   //!
   //! <b>Note</b>: No splaying is performed.
   template<class KeyType, class KeyNodePtrCompare>
   static std::pair<node_ptr, node_ptr> lower_bound_range
      (const_node_ptr header, const KeyType &key, KeyNodePtrCompare comp)
   {  return bstree_algo::lower_bound_range(header, key, comp);  }

   //! @copydoc ::boost::intrusive::bstree_algorithms::find(const_node_ptr, const KeyType&,KeyNodePtrCompare)
   //!
   //! <b>Note</b>: The tree must not contain equivalent keys. The node with a key equivalent
   //!   to `key` is splayed. If there is no such node, the node immediately before or after
   //!   the position of `key` is splayed.
   //!   This function can be more efficient than find.
   template<class KeyType, class KeyNodePtrCompare>
   static node_ptr find_unique
      (node_ptr header, const KeyType &key, KeyNodePtrCompare comp)
   {
      bool found, before;
      node_ptr const r = priv_splay_down(header, key, comp, found, before);
      return found ? r : header;
   }

   //! @copydoc ::boost::intrusive::bstree_algorithms::lower_bound(const_node_ptr,const KeyType&,KeyNodePtrCompare)
   //!
   //! <b>Note</b>: The tree must not contain equivalent keys. The node with a key equivalent
   //!   to `key` is splayed. If there is no such node, the node immediately before or after
   //!   the position of `key` is splayed.
   //!   This function can be more efficient than lower_bound.
   template<class KeyType, class KeyNodePtrCompare>
   static node_ptr lower_bound_unique
      (node_ptr header, const KeyType &key, KeyNodePtrCompare comp)
   {
      bool found, before;
      node_ptr const r = priv_splay_down(header, key, comp, found, before);
      return found ? r : priv_bound_from_root(header, r, before);
   }

   //! @copydoc ::boost::intrusive::bstree_algorithms::upper_bound(const_node_ptr,const KeyType&,KeyNodePtrCompare)
   //!
   //! <b>Note</b>: The tree must not contain equivalent keys. The node with a key equivalent
   //!   to `key` is splayed. If there is no such node, the node immediately before or after
   //!   the position of `key` is splayed.
   //!   This function can be more efficient than upper_bound.
   template<class KeyType, class KeyNodePtrCompare>
   static node_ptr upper_bound_unique
      (node_ptr header, const KeyType &key, KeyNodePtrCompare comp)
   {
      bool found, before;
      node_ptr const r = priv_splay_down(header, key, comp, found, before);
      return found ? priv_next_of_root(header, r) : priv_bound_from_root(header, r, before);
   }

   //! @copydoc ::boost::intrusive::bstree_algorithms::equal_range(const_node_ptr,const KeyType&,KeyNodePtrCompare)
   //!
   //! <b>Note</b>: The tree must not contain equivalent keys. The node with a key equivalent
   //!   to `key` is splayed. If there is no such node, the node immediately before or after
   //!   the position of `key` is splayed.
   //!   This function can be more efficient than equal_range.
   template<class KeyType, class KeyNodePtrCompare>
   static std::pair<node_ptr, node_ptr> equal_range_unique
      (node_ptr header, const KeyType &key, KeyNodePtrCompare comp)
   {
      bool found, before;
      node_ptr const r = priv_splay_down(header, key, comp, found, before);
      if(found)
         return std::pair<node_ptr, node_ptr>(r, priv_next_of_root(header, r));
      node_ptr const b = priv_bound_from_root(header, r, before);
      return std::pair<node_ptr, node_ptr>(b, b);
   }

   //! @copydoc ::boost::intrusive::bstree_algorithms::bounded_range(const_node_ptr,const KeyType&,const KeyType&,KeyNodePtrCompare,bool,bool)
   //!
   //! <b>Note</b>: The first node of the range, or the node immediately before it,
   //!   is splayed.
   template<class KeyType, class KeyNodePtrCompare>
   static std::pair<node_ptr, node_ptr> bounded_range
      (node_ptr header, const KeyType &lower_key, const KeyType &upper_key, KeyNodePtrCompare comp
      , bool left_closed, bool right_closed)
   {
      //Splay the node before or after the first node of the range (lower bound if
      //left_closed, upper bound otherwise)
      bool before;
      node_ptr const r = left_closed
         ? priv_splay_down_bound<false>(header, lower_key, comp, before)
         : priv_splay_down_bound<true> (header, lower_key, comp, before);
      if(r == header)   //Empty tree
         return std::pair<node_ptr, node_ptr>(header, header);
      node_ptr const first = before ? r : priv_next_of_root(header, r);
      //The end of the range is not before first, so it is r or a node of the right subtree of r
      if(before && (right_closed ? comp(upper_key, r) : !comp(r, upper_key)))
         return std::pair<node_ptr, node_ptr>(first, r);
      node_ptr const r_right = NodeTraits::get_right(r);
      node_ptr const last = right_closed
         ? bstree_algo::upper_bound_loop(r_right, header, upper_key, comp)
         : bstree_algo::lower_bound_loop(r_right, header, upper_key, comp);
      return std::pair<node_ptr, node_ptr>(first, last);
   }

   //! @copydoc ::boost::intrusive::bstree_algorithms::bounded_range(const_node_ptr,const KeyType&,const KeyType&,KeyNodePtrCompare,bool,bool)
   //!
   //! <b>Note</b>: No splaying is performed.
   template<class KeyType, class KeyNodePtrCompare>
   static std::pair<node_ptr, node_ptr> bounded_range
      (const_node_ptr header, const KeyType &lower_key, const KeyType &upper_key, KeyNodePtrCompare comp
      , bool left_closed, bool right_closed)
   {  return bstree_algo::bounded_range(header, lower_key, upper_key, comp, left_closed, right_closed);  }

   //! @copydoc ::boost::intrusive::bstree_algorithms::insert_equal_upper_bound(node_ptr,node_ptr,NodePtrCompare)
   //!
   //! <b>Note</b>: Before the insertion, the node immediately before or after the
   //!   insertion position is splayed.
   template<class NodePtrCompare>
   static node_ptr insert_equal_upper_bound
      (node_ptr header, node_ptr new_node, NodePtrCompare comp)
   {
      insert_commit_data commit_data;
      priv_insert_equal_check<true>(header, new_node, comp, commit_data);
      bstree_algo::insert_commit(header, new_node, commit_data);
      return new_node;
   }

   //! @copydoc ::boost::intrusive::bstree_algorithms::insert_equal_lower_bound(node_ptr,node_ptr,NodePtrCompare)
   //!
   //! <b>Note</b>: Before the insertion, the node immediately before or after the
   //!   insertion position is splayed.
   template<class NodePtrCompare>
   static node_ptr insert_equal_lower_bound
      (node_ptr header, node_ptr new_node, NodePtrCompare comp)
   {
      insert_commit_data commit_data;
      priv_insert_equal_check<false>(header, new_node, comp, commit_data);
      bstree_algo::insert_commit(header, new_node, commit_data);
      return new_node;
   }

   //! @copydoc ::boost::intrusive::bstree_algorithms::insert_equal(node_ptr,node_ptr,node_ptr,NodePtrCompare)
   //!
   //! <b>Note</b>: If "hint" is correct, "hint" (or its previous node if "hint" is the header)
   //!   is splayed. Otherwise, the nodes are splayed as in insert_equal_upper_bound or
   //!   insert_equal_lower_bound.
   template<class NodePtrCompare>
   static node_ptr insert_equal
      (node_ptr header, node_ptr hint, node_ptr new_node, NodePtrCompare comp)
   {
      //The hint is correct if prev(hint) <= new_node <= hint (header acts as +infinity for hint
      //and as -infinity for prev). First check new_node <= hint.
      if(hint == header || !comp(hint, new_node)){
         node_ptr const prev = priv_prev_of_hint(header, hint);
         if(prev == header || !comp(new_node, prev)){
            //Correct hint: link new_node between prev and hint without new comparisons
            insert_commit_data commit_data;
            priv_splay_hint_and_commit_data(header, hint, prev, commit_data);
            bstree_algo::insert_commit(header, new_node, commit_data);
            return new_node;
         }
         //new_node < prev: wrong hint. The nearest position to hint is the upper bound
         return insert_equal_upper_bound(header, new_node, comp);
      }
      //hint < new_node: wrong hint. The nearest position to hint is the lower bound
      return insert_equal_lower_bound(header, new_node, comp);
   }

   //! @copydoc ::boost::intrusive::bstree_algorithms::insert_before(node_ptr,node_ptr,node_ptr)
   //!
   //! <b>Note</b>: The inserted node is splayed.
   static node_ptr insert_before
      (node_ptr header, node_ptr pos, node_ptr new_node) BOOST_NOEXCEPT
   {
      bstree_algo::insert_before(header, pos, new_node);
      splay_up(new_node, header);
      return new_node;
   }

   //! @copydoc ::boost::intrusive::bstree_algorithms::push_back(node_ptr,node_ptr)
   //!
   //! <b>Note</b>: The inserted node is splayed.
   static void push_back(node_ptr header, node_ptr new_node) BOOST_NOEXCEPT
   {
      bstree_algo::push_back(header, new_node);
      splay_up(new_node, header);
   }

   //! @copydoc ::boost::intrusive::bstree_algorithms::push_front(node_ptr,node_ptr)
   //!
   //! <b>Note</b>: The inserted node is splayed.
   static void push_front(node_ptr header, node_ptr new_node) BOOST_NOEXCEPT
   {
      bstree_algo::push_front(header, new_node);
      splay_up(new_node, header);
   }

   //! @copydoc ::boost::intrusive::bstree_algorithms::insert_unique_check(const_node_ptr,const KeyType&,KeyNodePtrCompare,insert_commit_data&)
   //!
   //! <b>Note</b>: A node with a key equivalent to `key` is splayed. If there is no such
   //!   node, the node immediately before or after the position of `key` is splayed.
   template<class KeyType, class KeyNodePtrCompare>
   static std::pair<node_ptr, bool> insert_unique_check
      (node_ptr header, const KeyType &key
      ,KeyNodePtrCompare comp, insert_commit_data &commit_data)
   {
      bool found, before;
      node_ptr const r = priv_splay_down(header, key, comp, found, before);
      if(found)
         return std::pair<node_ptr, bool>(r, false);
      priv_insert_commit_data_from_root(header, r, before, commit_data);
      return std::pair<node_ptr, bool>(node_ptr(), true);
   }

   //! @copydoc ::boost::intrusive::bstree_algorithms::insert_unique_check(const_node_ptr,node_ptr,const KeyType&,KeyNodePtrCompare,insert_commit_data&)
   //!
   //! <b>Note</b>: If "hint" is correct, "hint" (or its previous node if "hint" is the header)
   //!   is splayed. If "hint" or its previous node are equivalent to "key", that node is splayed.
   //!   Otherwise, the nodes are splayed as in insert_unique_check without hint.
   template<class KeyType, class KeyNodePtrCompare>
   static std::pair<node_ptr, bool> insert_unique_check
      (node_ptr header, node_ptr hint, const KeyType &key
      ,KeyNodePtrCompare comp, insert_commit_data &commit_data)
   {
      //The hint is correct if prev(hint) < key < hint (header acts as +infinity for hint
      //and as -infinity for prev). First check key < hint.
      if(hint == header || comp(key, hint)){
         node_ptr const prev = priv_prev_of_hint(header, hint);
         if(prev == header || comp(prev, key)){
            //Correct hint: prev < key < hint
            priv_splay_hint_and_commit_data(header, hint, prev, commit_data);
            return std::pair<node_ptr, bool>(node_ptr(), true);
         }
         else if(!comp(key, prev)){
            //prev is equivalent to key, no insertion but splay it as it is the accessed node
            splay_up(prev, header);
            return std::pair<node_ptr, bool>(prev, false);
         }
         //key < prev: wrong hint, fallthrough to hintless insertion
      }
      else if(!comp(hint, key)){
         //hint is equivalent to key, no insertion but splay it as it is the accessed node
         splay_up(hint, header);
         return std::pair<node_ptr, bool>(hint, false);
      }
      //Wrong hint, search from the root
      return insert_unique_check(header, key, comp, commit_data);
   }

   #ifdef BOOST_INTRUSIVE_DOXYGEN_INVOKED
   //! @copydoc ::boost::intrusive::bstree_algorithms::insert_unique_commit(node_ptr,node_ptr,const insert_commit_data&)
   static void insert_unique_commit
      (node_ptr header, node_ptr new_value, const insert_commit_data &commit_data) BOOST_NOEXCEPT;

   //! @copydoc ::boost::intrusive::bstree_algorithms::is_header
   static bool is_header(const_node_ptr p) BOOST_NOEXCEPT;

   //! @copydoc ::boost::intrusive::bstree_algorithms::rebalance
   static void rebalance(node_ptr header) BOOST_NOEXCEPT;

   //! @copydoc ::boost::intrusive::bstree_algorithms::rebalance_subtree
   static node_ptr rebalance_subtree(node_ptr old_root) BOOST_NOEXCEPT;

   #endif   //#ifdef BOOST_INTRUSIVE_DOXYGEN_INVOKED

   // bottom-up splay, use data_ as parent for n    | complexity : logarithmic    | exception : nothrow
   static void splay_up(node_ptr n, node_ptr header) BOOST_NOEXCEPT
   {  priv_splay_up(n, header); }

   // top-down splay | complexity : logarithmic    | exception : strong, note A
   template<class KeyType, class KeyNodePtrCompare>
   static node_ptr splay_down(node_ptr header, const KeyType &key, KeyNodePtrCompare comp, bool *pfound = 0)
   {
      bool found, before;
      node_ptr const r = priv_splay_down(header, key, comp, found, before);
      if(pfound)
         *pfound = found;
      return r;
   }

   private:

   /// @cond

   //After a priv_splay_down that did not find key, root r is its predecessor or successor
   //(or header if the tree is empty), so lower_bound(key) == upper_bound(key)
   static node_ptr priv_bound_from_root(node_ptr header, node_ptr r, bool before) BOOST_NOEXCEPT
   {  return (r == header || before) ? r : priv_next_of_root(header, r);  }

   //Splays the node immediately before or after the upper bound (if UpperBound) or
   //the lower bound of new_node and fills commit_data to insert new_node in that position.
   template<bool UpperBound, class NodePtrCompare>
   static void priv_insert_equal_check
      (node_ptr header, node_ptr new_node, NodePtrCompare comp, insert_commit_data &commit_data)
   {
      bool before_r;
      node_ptr const r = priv_splay_down_bound<UpperBound>(header, new_node, comp, before_r);
      priv_insert_commit_data_from_root(header, r, before_r, commit_data);
   }

   static node_ptr priv_next_of_root(node_ptr header, node_ptr r) BOOST_NOEXCEPT
   {
      node_ptr const r_right(NodeTraits::get_right(r));
      return r_right ? bstree_algo::minimum(r_right) : header;
   }

   //Returns prev(hint), or header if there is no previous node (hint is the leftmost node
   //or the tree is empty). Avoids prev_node's climb for the leftmost node and the header.
   static node_ptr priv_prev_of_hint(node_ptr header, node_ptr hint) BOOST_NOEXCEPT
   {
      return hint == NodeTraits::get_left(header) ? header
           : hint == header ? NodeTraits::get_right(header)
           : bstree_algo::prev_node(hint);
   }

   //For a correct hint, the new node's insertion point is between prev & hint.
   //Splaying hint makes that node root, so commit_data is filled without comparisons.
   //splay_up(header) splays the rightmost node (prev) or does nothing if the tree is empty.
   static void priv_splay_hint_and_commit_data
      (node_ptr header, node_ptr hint, node_ptr prev, insert_commit_data &commit_data) BOOST_NOEXCEPT
   {
      splay_up(hint, header);
      //r is now the root (or header if the tree is empty). The new node goes before r
      //if r is hint and after r if r is prev.
      node_ptr const r = hint != header ? hint : prev;
      priv_insert_commit_data_from_root(header, r, r == hint, commit_data);
   }

   //Fills commit_data so that insert_commit links a new node immediately before
   //(if before_r) or immediately after (otherwise) the root r in the in-order sequence.
   //This requires no comparisons:
   //
   // - Empty tree (r == header): the new node becomes the root (insert_commit links
   //   it as the left child of the header).
   // - Before r: the new node goes between prev(r) and r. If r has no left child, it is
   //   the left child of r. Otherwise prev(r) = maximum(left(r)), which has no right
   //   child, so the new node is the right child of prev(r).
   // - After r: the new node goes between r and next(r). If r has no right child, it is
   //   the right child of r. Otherwise next(r) = minimum(right(r)), which has no left
   //   child, so the new node is the left child of next(r).
   static void priv_insert_commit_data_from_root
      (node_ptr header, node_ptr r, bool before_r, insert_commit_data &commit_data) BOOST_NOEXCEPT
   {
      if(r == header){
         commit_data.link_left = true;
         commit_data.node      = header;
      }
      else if(before_r){
         node_ptr const r_left(NodeTraits::get_left(r));
         commit_data.link_left = !r_left;
         commit_data.node      = r_left ? bstree_algo::maximum(r_left) : r;
      }
      else{
         node_ptr const r_right(NodeTraits::get_right(r));
         commit_data.link_left = !!r_right;
         commit_data.node      = r_right ? bstree_algo::minimum(r_right) : r;
      }
   }

   // bottom-up splay, use data_ as parent for n    | complexity : logarithmic    | exception : nothrow
   static void priv_splay_up(node_ptr n, node_ptr header) BOOST_NOEXCEPT
   {
      // If (node == header) do a splay for the right most node instead
      // this is to boost performance of equal_range/count on equivalent containers in the case
      // where there are many equal elements at the end
      if(n == header)
         n = NodeTraits::get_right(header);

      node_ptr t(header);

      if( n == t ) return;

      for( ;; ){
         node_ptr p(NodeTraits::get_parent(n));
         node_ptr g(NodeTraits::get_parent(p));

         if( p == t )   break;

         if( g == t ){
            // zig
            rotate(n, t);
         }
         else if ((NodeTraits::get_left(p) == n && NodeTraits::get_left(g) == p)    ||
                  (NodeTraits::get_right(p) == n && NodeTraits::get_right(g) == p)  ){
            // zig-zig
            rotate(p, t);
            rotate(n, t);
         }
         else {
            // simple zig-zag: only one rotation, the next iteration continues with g
            rotate(n, t);
         }
      }
   }

   // Comparison adaptors for priv_splay_down_impl.
   //
   // - comp_left(key, n) is true if key (or its insertion position) is before n.
   // - comp_right(n, key) is true if it is after n.
   //
   // If is_total, an equivalent node is never found: "after n" is !comp_left,
   // so comp_right is never called and a node never needs both comparisons.

   //Equivalent nodes are found (comp_left== false && comp_right== false)
   template<class KeyType, class KeyNodePtrCompare>
   struct splay_three_way_comp
   {
      static const bool is_total = false;

      explicit splay_three_way_comp(KeyNodePtrCompare &comp)
         : comp_(comp)
      {}

      KeyNodePtrCompare &comp_;

      bool comp_left(const KeyType &key, node_ptr n) const
      {  return comp_(key, n);  }

      bool comp_right(node_ptr n, const KeyType &key) const
      {  return comp_(n, key);  }
   };

      
   template<bool UpperBound, class KeyType, class KeyNodePtrCompare>
   struct splay_bound_comp
   {
      static const bool is_total = true;

      explicit splay_bound_comp(KeyNodePtrCompare &comp)
         : comp_(comp)
      {}

      KeyNodePtrCompare &comp_;

      bool comp_left(const KeyType &key, node_ptr n) const
      {  return UpperBound ? comp_(key, n) : !comp_(n, key);  }

      bool comp_right(node_ptr, const KeyType &) const
      {
         BOOST_INTRUSIVE_INVARIANT_ASSERT(false); //Should never be called
         return false;
      }
   };

   //Splays a node equivalent to key. If there is no such node, splays the node
   //immediately before or after the position of key. Returns the new root
   //(header if the tree is empty). If not found, "before" is true if the position
   //of key is immediately before the returned root. If found, "before" is false.
   template<class KeyType, class KeyNodePtrCompare>
   static node_ptr priv_splay_down(node_ptr header, const KeyType &key, KeyNodePtrCompare comp, bool &found, bool &before)
   {
      splay_three_way_comp<KeyType, KeyNodePtrCompare> const c(comp);
      return priv_splay_down_impl(header, key, c, found, before);
   }

   //Splays the node immediately before or after the upper bound (if UpperBound) or the
   //lower bound of key. Returns the new root (header if the tree is empty). before_r is
   //true if the bound is immediately before the returned root.
   //
   //Splaying an equivalent node and then searching the bound would not splay the
   //bound position: inserting many equivalent keys would create a degenerate tree.
   template<bool UpperBound, class KeyType, class KeyNodePtrCompare>
   static node_ptr priv_splay_down_bound
      (node_ptr header, const KeyType &key, KeyNodePtrCompare comp, bool &before_r)
   {
      splay_bound_comp<UpperBound, KeyType, KeyNodePtrCompare> const c(comp);
      bool found;
      return priv_splay_down_impl(header, key, c, found, before_r);
   }

   enum splay_dir {  splay_left, splay_right, splay_found  };

   //Position of key relative to n
   template<class KeyType, class Comp>
   static splay_dir priv_splay_dir(const KeyType &key, node_ptr n, const Comp &c)
   {
      return c.comp_left(key, n) ? splay_left
           : (Comp::is_total || c.comp_right(n, key)) ? splay_right : splay_found;
   }

   //Top-down simple splay. Each node is compared at most once with comp_left and at
   //most once with comp_right. If Comp::is_total, each node is compared at most once.
   template<class KeyType, class Comp>
   static node_ptr priv_splay_down_impl(node_ptr header, const KeyType &key, const Comp &c, bool &found, bool &before)
   {
      //Most splay tree implementations use a dummy/null node to implement.
      //this function. This has some problems for a generic library like Intrusive:
      //
      // * The node might not have a default constructor.
      // * The default constructor could throw.
      //
      //We already have a header node. Leftmost and rightmost nodes of the tree
      //are not changed when splaying (because the invariants of the tree don't
      //change) We can back up them, use the header as the null node and
      //reassign old values after the function has been completed.
      node_ptr const old_root  = NodeTraits::get_parent(header);
      node_ptr const leftmost  = NodeTraits::get_left(header);
      node_ptr const rightmost = NodeTraits::get_right(header);
      if(leftmost == rightmost){ //Empty or unique node
         if(!old_root){
            found  = false;
            before = true;
            return header;
         }
         splay_dir const dir = priv_splay_dir(key, old_root, c);
         found  = dir == splay_found;
         before = dir == splay_left;
         return old_root;
      }
      else{
         //Initialize "null node" (the header in our case)
         NodeTraits::set_left (header, node_ptr());
         NodeTraits::set_right(header, node_ptr());
         //Class that will backup leftmost/rightmost from header, commit the assemble(),
         //and will restore leftmost/rightmost to header even if "comp" throws
         detail::splaydown_assemble_and_fix_header<NodeTraits> commit(old_root, header, leftmost, rightmost);

         //dir is the position of key relative to commit.t_. A full comparison is only
         //needed after a zig-zig. After a simple zig-zag one comparison is already known.
         splay_dir dir = priv_splay_dir(key, commit.t_, c);
         for( ;; ){
            if(dir == splay_left){
               node_ptr const t_left = NodeTraits::get_left(commit.t_);
               if(!t_left)
                  break;
               if(c.comp_left(key, t_left)){ //zig-zig
                  bstree_algo::rotate_right_no_parent_fix(commit.t_, t_left);
                  commit.t_ = t_left;
                  if( !NodeTraits::get_left(commit.t_) )
                     break;
                  link_right(commit.t_, commit.r_);
                  dir = priv_splay_dir(key, commit.t_, c);
               }
               else{
                  //simple zig-zag: key is not before t_left, the new commit.t_
                  link_right(commit.t_, commit.r_);
                  dir = (Comp::is_total || c.comp_right(commit.t_, key)) ? splay_right : splay_found;
               }
            }
            else if(Comp::is_total || dir == splay_right){
               node_ptr const t_right = NodeTraits::get_right(commit.t_);
               if(!t_right)
                  break;

               if(Comp::is_total ? !c.comp_left(key, t_right) : c.comp_right(t_right, key)){  //zig-zig
                     bstree_algo::rotate_left_no_parent_fix(commit.t_, t_right);
                     commit.t_ = t_right;
                     if( !NodeTraits::get_right(commit.t_) )
                        break;
                     link_left(commit.t_, commit.l_);
                     dir = priv_splay_dir(key, commit.t_, c);
               }
               else{
                  //simple zig-zag: key is not after t_right, the new commit.t_
                  link_left(commit.t_, commit.l_);
                  dir = (Comp::is_total || c.comp_left(key, commit.t_)) ? splay_left : splay_found;
               }
            }
            else{
               break;
            }
         }
         found  = dir == splay_found;
         before = dir == splay_left;
         //commit.~splaydown_assemble_and_fix_header<NodeTraits>() will first
         //"assemble()" + link the new root & recover header's leftmost & rightmost
         return commit.t_;
      }
   }

   // break link to left child node and attach it to left tree pointed to by l   | complexity : constant | exception : nothrow
   static void link_left(node_ptr & t, node_ptr & l) BOOST_NOEXCEPT
   {
      //procedure link_left;
      //    t, l, right(l) := right(t), t, t
      //end link_left
      NodeTraits::set_right(l, t);
      NodeTraits::set_parent(t, l);
      l = t;
      t = NodeTraits::get_right(t);
   }

   // break link to right child node and attach it to right tree pointed to by r | complexity : constant | exception : nothrow
   static void link_right(node_ptr & t, node_ptr & r) BOOST_NOEXCEPT
   {
      //procedure link_right;
      //    t, r, left(r) := left(t), t, t
      //end link_right;
      NodeTraits::set_left(r, t);
      NodeTraits::set_parent(t, r);
      r = t;
      t = NodeTraits::get_left(t);
   }

   // rotate n with its parent                     | complexity : constant    | exception : nothrow
   static void rotate(node_ptr n, node_ptr header) BOOST_NOEXCEPT
   {
      //procedure rotate_left;
      //    t, right(t), left(right(t)) := right(t), left(right(t)), t
      //end rotate_left;
      node_ptr p = NodeTraits::get_parent(n);
      node_ptr g = NodeTraits::get_parent(p);
      bool const g_is_header = g == header;

      if(NodeTraits::get_left(p) == n){
         NodeTraits::set_left(p, NodeTraits::get_right(n));
         if(NodeTraits::get_left(p))
            NodeTraits::set_parent(NodeTraits::get_left(p), p);
         NodeTraits::set_right(n, p);
      }
      else{ // must be ( p->right == n )
         NodeTraits::set_right(p, NodeTraits::get_left(n));
         if(NodeTraits::get_right(p))
            NodeTraits::set_parent(NodeTraits::get_right(p), p);
         NodeTraits::set_left(n, p);
      }

      NodeTraits::set_parent(p, n);
      NodeTraits::set_parent(n, g);

      if(g_is_header){
         //p was the root, so it's the parent of the header
         BOOST_INTRUSIVE_INVARIANT_ASSERT(NodeTraits::get_parent(g) == p);
         NodeTraits::set_parent(g, n);
      }
      else{
         if(NodeTraits::get_left(g) == p)
            NodeTraits::set_left(g, n);
         else  //must be ( g->right == p )
            NodeTraits::set_right(g, n);
      }
   }

   /// @endcond
};

/// @cond

template<class NodeTraits>
struct get_algo<SplayTreeAlgorithms, NodeTraits>
{
   typedef splaytree_algorithms<NodeTraits> type;
};

template <class ValueTraits, class NodePtrCompare, class ExtraChecker>
struct get_node_checker<SplayTreeAlgorithms, ValueTraits, NodePtrCompare, ExtraChecker>
{
   typedef detail::bstree_node_checker<ValueTraits, NodePtrCompare, ExtraChecker> type;
};

/// @endcond

} //namespace intrusive
} //namespace boost

#include <boost/intrusive/detail/config_end.hpp>

#endif //BOOST_INTRUSIVE_SPLAYTREE_ALGORITHMS_HPP
