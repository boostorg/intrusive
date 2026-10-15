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

#ifndef BOOST_INTRUSIVE_TREE_NODE_HPP
#define BOOST_INTRUSIVE_TREE_NODE_HPP

#ifndef BOOST_CONFIG_HPP
#  include <boost/config.hpp>
#endif

#if defined(BOOST_HAS_PRAGMA_ONCE)
#  pragma once
#endif

#include <boost/intrusive/detail/config_begin.hpp>
#include <boost/intrusive/detail/workaround.hpp>
#include <boost/intrusive/detail/mpl.hpp>
#include <boost/intrusive/pointer_rebind.hpp>

namespace boost {
namespace intrusive {

template<class VoidPointer>
struct tree_node
{
   typedef typename pointer_rebind<VoidPointer, tree_node>::type  node_ptr;

   node_ptr parent_, left_, right_;
};

template<class VoidPointer>
struct tree_node_traits
{
   typedef tree_node<VoidPointer> node;

   typedef typename node::node_ptr   node_ptr;
   typedef typename pointer_rebind<VoidPointer, const node>::type const_node_ptr;
   typedef typename detail::ptr_cparam<node_ptr>::type        node_ptr_cparam;
   typedef typename detail::ptr_cparam<const_node_ptr>::type  const_node_ptr_cparam;

   BOOST_INTRUSIVE_FORCEINLINE static node_ptr get_parent(BOOST_INTRUSIVE_CPARAM(const_node_ptr) n)
   {  return n->parent_;  }

   BOOST_INTRUSIVE_FORCEINLINE static node_ptr get_parent(BOOST_INTRUSIVE_CPARAM(node_ptr) n)
   {  return n->parent_;  }

   BOOST_INTRUSIVE_FORCEINLINE static void set_parent(BOOST_INTRUSIVE_CPARAM(node_ptr) n, node_ptr p)
   {  n->parent_ = p;  }

   BOOST_INTRUSIVE_FORCEINLINE static node_ptr get_left(BOOST_INTRUSIVE_CPARAM(const_node_ptr) n)
   {  return n->left_;  }

   BOOST_INTRUSIVE_FORCEINLINE static node_ptr get_left(BOOST_INTRUSIVE_CPARAM(node_ptr) n)
   {  return n->left_;  }

   BOOST_INTRUSIVE_FORCEINLINE static void set_left(BOOST_INTRUSIVE_CPARAM(node_ptr) n, node_ptr l)
   {  n->left_ = l;  }

   BOOST_INTRUSIVE_FORCEINLINE static node_ptr get_right(BOOST_INTRUSIVE_CPARAM(const_node_ptr) n)
   {  return n->right_;  }

   BOOST_INTRUSIVE_FORCEINLINE static node_ptr get_right(BOOST_INTRUSIVE_CPARAM(node_ptr) n)
   {  return n->right_;  }

   BOOST_INTRUSIVE_FORCEINLINE static void set_right(BOOST_INTRUSIVE_CPARAM(node_ptr) n, node_ptr r)
   {  n->right_ = r;  }
};

} //namespace intrusive
} //namespace boost

#include <boost/intrusive/detail/config_end.hpp>

#endif //BOOST_INTRUSIVE_TREE_NODE_HPP
