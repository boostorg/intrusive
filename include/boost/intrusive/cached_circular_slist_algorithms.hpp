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

#ifndef BOOST_INTRUSIVE_CACHED_CIRCULAR_SLIST_ALGORITHMS_HPP
#define BOOST_INTRUSIVE_CACHED_CIRCULAR_SLIST_ALGORITHMS_HPP

#include <boost/intrusive/detail/config_begin.hpp>
#include <boost/intrusive/intrusive_fwd.hpp>
#include <boost/intrusive/circular_slist_algorithms.hpp>
#include <boost/intrusive/detail/cached_slist_algorithms_impl.hpp>
#include <cstddef>

#if defined(BOOST_HAS_PRAGMA_ONCE)
#  pragma once
#endif

namespace boost {
namespace intrusive {

//! cached_circular_slist_algorithms provides basic algorithms to manipulate nodes
//! forming a circular singly linked list that has a header node and a cached pointer
//! to its last node. The nodes are the same as the nodes used by
//! circular_slist_algorithms, and the cached last node can be stored anywhere
//! (it is not part of the nodes).
//!
//! The cached last node of a list is the last node of the list, or the header node
//! if the list is empty. Functions that change the links take a reference to the
//! cached last node ("last") and update it. Functions that can not update the cached
//! last node of circular_slist_algorithms are not available.
//!
//! cached_circular_slist_algorithms is configured with a NodeTraits class, which encapsulates the
//! information about the node to be manipulated. NodeTraits must support the
//! following interface:
//!
//! <b>Typedefs</b>:
//!
//! <tt>node</tt>: The type of the node that forms the circular list
//!
//! <tt>node_ptr</tt>: A pointer to a node
//!
//! <tt>const_node_ptr</tt>: A pointer to a const node
//!
//! <b>Static functions</b>:
//!
//! <tt>static node_ptr get_next(const_node_ptr n);</tt>
//!
//! <tt>static void set_next(node_ptr n, node_ptr next);</tt>
template<class NodeTraits>
class cached_circular_slist_algorithms
   /// @cond
   : public detail::cached_slist_algorithms_impl<circular_slist_algorithms<NodeTraits>, false>
   /// @endcond
{
   /// @cond
   typedef detail::cached_slist_algorithms_impl<circular_slist_algorithms<NodeTraits>, false> base_t;
   /// @endcond

   public:
   typedef typename NodeTraits::node            node;
   typedef typename NodeTraits::node_ptr        node_ptr;
   typedef typename NodeTraits::const_node_ptr  const_node_ptr;
   typedef NodeTraits                           node_traits;

   #if defined(BOOST_INTRUSIVE_DOXYGEN_INVOKED)

   //! <b>Requires</b>: this_node must be in a circular list or be an empty circular list.
   //!
   //! <b>Effects</b>: Returns the previous node of the previous node of this_node in the circular list.
   //!
   //! <b>Complexity</b>: Linear to the number of elements in the circular list.
   //!
   //! <b>Throws</b>: Nothing.
   static node_ptr get_previous_previous_node(node_ptr this_node) noexcept;

   //! <b>Requires</b>: this_node and p must be in the same circular list.
   //!
   //! <b>Effects</b>: Returns the previous node of the previous node of this_node in the
   //!   circular list starting the search from p. The first node checked
   //!   for equality is NodeTraits::get_next((NodeTraits::get_next(p)).
   //!
   //! <b>Complexity</b>: Linear to the number of elements in the circular list.
   //!
   //! <b>Throws</b>: Nothing.
   static node_ptr get_previous_previous_node(node_ptr p, node_ptr this_node) noexcept;

   //! <b>Effects</b>: Constructs a non-used list element, putting the next
   //!   pointer to null:
   //!  <tt>NodeTraits::get_next(this_node) == node_ptr()</tt>
   //!
   //! <b>Complexity</b>: Constant
   //!
   //! <b>Throws</b>: Nothing.
   static void init(node_ptr this_node) noexcept;

   //! <b>Effects</b>: Returns true if "this_node" is not followed by other nodes
   //!  or if it is a non-used node.
   //!
   //! <b>Complexity</b>: Constant
   //!
   //! <b>Throws</b>: Nothing.
   static bool unique(const_node_ptr this_node) noexcept;

   //! <b>Effects</b>: Returns true if "this_node" has the same state as if
   //!  it was inited using "init(node_ptr)"
   //!
   //! <b>Complexity</b>: Constant
   //!
   //! <b>Throws</b>: Nothing.
   static bool inited(const_node_ptr this_node) noexcept;

   //! <b>Effects</b>: Returns a pointer to a node that represents the "end" (one past end) node
   //!
   //! <b>Complexity</b>: Constant time.
   //!
   //! <b>Throws</b>: Nothing.
   static node_ptr end_node(const_node_ptr header) noexcept;

   //! <b>Effects</b>: Returns true if "header" is the header of an empty list.
   //!
   //! <b>Complexity</b>: Constant
   //!
   //! <b>Throws</b>: Nothing.
   static bool is_empty(const_node_ptr header) noexcept;

   //! <b>Requires</b>: this_node and prev_init_node must be in the same circular list.
   //!
   //! <b>Effects</b>: Returns the previous node of this_node in the circular list starting
   //!   the search from prev_init_node. The first node checked for equality
   //!   is NodeTraits::get_next(prev_init_node).
   //!
   //! <b>Complexity</b>: Linear to the number of elements between prev_init_node and this_node.
   //!
   //! <b>Throws</b>: Nothing.
   static node_ptr get_previous_node(node_ptr prev_init_node, node_ptr this_node) noexcept;

   //! <b>Requires</b>: this_node must be in a circular list or be an empty circular list.
   //!
   //! <b>Effects</b>: Returns the previous node of this_node in the circular list.
   //!
   //! <b>Complexity</b>: Linear to the number of elements in the circular list.
   //!
   //! <b>Throws</b>: Nothing.
   static node_ptr get_previous_node(node_ptr this_node) noexcept;

   //! <b>Effects</b>: Returns the number of nodes of the circular list that contains
   //!   this_node (this_node included).
   //!
   //! <b>Complexity</b>: Linear
   //!
   //! <b>Throws</b>: Nothing.
   static std::size_t count(const_node_ptr this_node) noexcept;

   //! <b>Effects</b>: Constructs an empty list, with "header" as the header node.
   //!
   //! <b>Postcondition</b>: <tt>last == header</tt>.
   //!
   //! <b>Complexity</b>: Constant
   //!
   //! <b>Throws</b>: Nothing.
   static void init_header(node_ptr header, node_ptr &last) noexcept;

   //! <b>Requires</b>: prev_node must be a node of a circular list (or its header) whose
   //!   cached last node is "last".
   //!
   //! <b>Effects</b>: Links this_node after prev_node in the circular list and updates "last".
   //!
   //! <b>Complexity</b>: Constant
   //!
   //! <b>Throws</b>: Nothing.
   static void link_after(node_ptr prev_node, node_ptr this_node, node_ptr &last) noexcept;

   //! <b>Requires</b>: prev_node must be a node of a circular list (or its header) whose
   //!   cached last node is "last", and it must not be the last node.
   //!
   //! <b>Effects</b>: Unlinks the next node of prev_node from the circular list and updates "last".
   //!
   //! <b>Complexity</b>: Constant
   //!
   //! <b>Throws</b>: Nothing.
   static void unlink_after(node_ptr prev_node, node_ptr &last) noexcept;

   //! <b>Requires</b>: (prev_node, e) must be a valid range of a circular list whose
   //!   cached last node is "last". "e" can be the end node.
   //!
   //! <b>Effects</b>: Unlinks the range (prev_node, e) from the circular list and updates "last".
   //!
   //! <b>Complexity</b>: Constant
   //!
   //! <b>Throws</b>: Nothing.
   static void unlink_after(node_ptr prev_node, node_ptr e, node_ptr &last) noexcept;

   //! <b>Requires</b>: bp must be a node of a circular list (or its header) whose
   //!   cached last node is "last". [b, be] must be a chain of nodes that are not in a list.
   //!
   //! <b>Effects</b>: Links the nodes [b, be] after bp and updates "last".
   //!
   //! <b>Complexity</b>: Constant
   //!
   //! <b>Throws</b>: Nothing.
   static void incorporate_after(node_ptr bp, node_ptr b, node_ptr be, node_ptr &last) noexcept;

   //! <b>Requires</b>: p must be a node of a circular list (or its header) whose cached last
   //!   node is "p_last". (b, e] must be a range of a circular list whose cached last
   //!   node is "b_last". If both are in the same list, p must not be in (b, e], and p_last
   //!   and b_last can be the same object.
   //!
   //! <b>Effects</b>: Removes the nodes (b, e] from their list and inserts
   //!   them after p in p's list. Updates "p_last" and "b_last".
   //!
   //! <b>Complexity</b>: Constant
   //!
   //! <b>Throws</b>: Nothing.
   static void transfer_after(node_ptr p, node_ptr b, node_ptr e, node_ptr &p_last, node_ptr &b_last) noexcept;

   //! <b>Requires</b>: p must be a node of a circular list (or its header) whose cached last
   //!   node is "p_last". "other" must be the header of a different circular list whose
   //!   cached last node is "other_last".
   //!
   //! <b>Effects</b>: Transfers all nodes of other's list after p in p's list.
   //!   Updates "p_last" and "other_last".
   //!
   //! <b>Complexity</b>: Constant
   //!
   //! <b>Throws</b>: Nothing.
   static void transfer_after(node_ptr p, node_ptr other, node_ptr &p_last, node_ptr &other_last) noexcept;

   //! <b>Requires</b>: this_node and other_node must be the headers of two circular lists
   //!   whose cached last nodes are "this_last" and "other_last".
   //!
   //! <b>Effects</b>: Swaps the nodes of both lists and their cached last nodes.
   //!
   //! <b>Complexity</b>: Constant
   //!
   //! <b>Throws</b>: Nothing.
   static void swap_trailing_nodes(node_ptr this_node, node_ptr other_node, node_ptr &this_last, node_ptr &other_last) noexcept;

   //! <b>Requires</b>: header must be the header of a circular list whose cached last node is "last".
   //!
   //! <b>Effects</b>: Reverses the order of the nodes of the list and updates "last".
   //!
   //! <b>Complexity</b>: Linear to the number of nodes.
   //!
   //! <b>Throws</b>: Nothing.
   static void reverse(node_ptr header, node_ptr &last) noexcept;

   //! <b>Requires</b>: header must be the header of a circular list whose cached last node is "last".
   //!
   //! <b>Effects</b>: Moves backwards all the nodes, so that the first node becomes
   //!   the second, the second becomes the third... the last node becomes the first one.
   //!   Repeats this n times. Updates "last".
   //!
   //! <b>Complexity</b>: Linear to the number of nodes plus the number of moved positions.
   //!
   //! <b>Throws</b>: Nothing.
   static void shift_backwards(node_ptr header, std::size_t n, node_ptr &last) noexcept;

   //! <b>Requires</b>: header must be the header of a circular list whose cached last node is "last".
   //!
   //! <b>Effects</b>: Moves forward all the nodes, so that the second node becomes
   //!   the first, the third becomes the second... the first node becomes the last one.
   //!   Repeats this n times. Updates "last".
   //!
   //! <b>Complexity</b>: Linear to the number of nodes plus the number of moved positions.
   //!
   //! <b>Throws</b>: Nothing.
   static void shift_forward(node_ptr header, std::size_t n, node_ptr &last) noexcept;

   //! <b>Requires</b>: header must be the header of a circular list whose cached last node is "last".
   //!   "comp" must be a function object that induces a strict weak ordering on node_ptrs:
   //!   <tt>comp(a, b)</tt> returns true if node a must be placed before node b.
   //!
   //! <b>Effects</b>: Stable sorts, according to "comp", all the nodes of the list and updates "last".
   //!
   //! <b>Complexity</b>: Approximately N log N comparisons, where N is the number of nodes.
   //!   Uses a fixed amount of stack memory.
   //!
   //! <b>Throws</b>: If "comp" throws. Basic guarantee: all nodes remain in the list
   //!   in unspecified order and "last" is the last node of the list.
   template<class NodePtrCompare>
   static void sort(node_ptr header, NodePtrCompare comp, node_ptr &last);

   //! <b>Requires</b>: p and x must be the headers of two different circular lists whose
   //!   cached last nodes are "p_last" and "x_last". The nodes of both lists must be sorted
   //!   according to "comp", a function object that induces a strict weak ordering on node_ptrs:
   //!   <tt>comp(a, b)</tt> returns true if node a must be placed before node b.
   //!
   //! <b>Effects</b>: Removes all the nodes of x's list and inserts them in order in p's list.
   //!   The merge is stable: if a node of p's list is equivalent to a node of x's list,
   //!   the node of p's list goes first. Updates "p_last" and "x_last".
   //!
   //! <b>Returns</b>: The last node that was transferred from x's list, or a null pointer
   //!   if x's list was empty.
   //!
   //! <b>Complexity</b>: Linear: it performs at most N + M - 1 comparisons, where N and M
   //!   are the number of nodes of each list.
   //!
   //! <b>Throws</b>: If "comp" throws. Basic guarantee: all the nodes of x's list
   //!   are moved to p's list in unspecified order, and "p_last" and "x_last" are updated.
   template<class NodePtrCompare>
   static node_ptr merge(node_ptr p, node_ptr x, NodePtrCompare comp, node_ptr &p_last, node_ptr &x_last);

   //! <b>Requires</b>: header must be the header of a circular list whose cached last node is "last".
   //!   "disposer" must be an object function taking a node_ptr parameter. Its call operator
   //!   and its copy and move constructors shouldn't throw.
   //!
   //! <b>Effects</b>: Unlinks all nodes of the list and calls
   //!   <tt>void disposer::operator()(node_ptr)</tt> for every node. The list is left empty.
   //!
   //! <b>Returns</b>: The number of disposed nodes
   //!
   //! <b>Complexity</b>: Linear to the number of nodes.
   //!
   //! <b>Throws</b>: Nothing.
   template<class Disposer>
   static std::size_t detach_and_dispose(node_ptr header, Disposer disposer, node_ptr &last) noexcept;

   #else

   //Only available in circular lists, so it is not made public in cached_slist_algorithms_impl
   using base_t::get_previous_previous_node;

   #endif   //#if defined(BOOST_INTRUSIVE_DOXYGEN_INVOKED)
};

} //namespace intrusive
} //namespace boost

#include <boost/intrusive/detail/config_end.hpp>

#endif //BOOST_INTRUSIVE_CACHED_CIRCULAR_SLIST_ALGORITHMS_HPP
