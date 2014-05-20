/////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Matei David 2014
//
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/intrusive for documentation.
//
/////////////////////////////////////////////////////////////////////////////

#ifndef BOOST_INTRUSIVE_BPTR_VALUE_HPP
#define BOOST_INTRUSIVE_BPTR_VALUE_HPP

#include <cassert>
#include <boost/intrusive/list.hpp>
#include "bounded_pointer.hpp"
#include "common_functors.hpp"


namespace boost
{
namespace intrusive
{

struct BPtr_Value
{
    static const bool constant_time_size = true;

    BPtr_Value(int value = 42) : value_(value) {}
    //BPtr_Value(const BPtr_Value& rhs) = delete;
    BPtr_Value(const BPtr_Value& rhs) : value_(rhs.value_) {}
    //BPtr_Value(BPtr_Value&&) = delete;
    ~BPtr_Value()
    {
        if (is_linked())
        {
            std::cerr << "BPtr_Value dtor: destructing linked value: &=" << (void*)this << "\n";
            assert(false);
        }
    }

    // testvalue is used in std::vector and thus prev and next
    // have to be handled appropriately when copied:
    BPtr_Value& operator = (const BPtr_Value& src)
    {
        if (is_linked())
        {
            std::cerr << "BPtr_Value asop: assigning to linked value: &=" << (void*)this << ", src=" << (void*)&src << "\n";
            assert(false);
        }
        value_ = src.value_;
        //_previous = src._previous;
        //_next = src._next;
        return *this;
    }

    // value
    int value_;

    // list node hooks
    Bounded_Pointer< BPtr_Value > _previous;
    Bounded_Pointer< BPtr_Value > _next;
    // tree node hooks
    Bounded_Pointer< BPtr_Value > _parent;
    Bounded_Pointer< BPtr_Value > _l_child;
    Bounded_Pointer< BPtr_Value > _r_child;
    char _extra;

    bool is_linked() const { return _previous or _next or _parent or _l_child or _r_child; }

    friend bool operator< (const BPtr_Value &other1, const BPtr_Value &other2)
    {  return other1.value_ < other2.value_;  }

    friend bool operator< (int other1, const BPtr_Value &other2)
    {  return other1 < other2.value_;  }

    friend bool operator< (const BPtr_Value &other1, int other2)
    {  return other1.value_ < other2;  }

    friend bool operator== (const BPtr_Value &other1, const BPtr_Value &other2)
    {  return other1.value_ == other2.value_;  }

    friend bool operator== (int other1, const BPtr_Value &other2)
    {  return other1 == other2.value_;  }

    friend bool operator== (const BPtr_Value &other1, int other2)
    {  return other1.value_ == other2;  }

    friend bool operator!= (const BPtr_Value &other1, const BPtr_Value &other2)
    {  return !(other1 == other2);  }

    friend bool operator!= (int other1, const BPtr_Value &other2)
    {  return !(other1 == other2.value_);  }

    friend bool operator!= (const BPtr_Value &other1, int other2)
    {  return !(other1.value_ == other2);  }

    friend std::ostream& operator << (std::ostream& os, const BPtr_Value& v)
    {
        os << v.value_;
        return os;
    }
}; // class BPtr_Value

template < typename Node_Algorithms >
void swap_nodes(Bounded_Reference< BPtr_Value > lhs,
                Bounded_Reference< BPtr_Value > rhs)
{
    Node_Algorithms::swap_nodes(
        boost::intrusive::pointer_traits< Bounded_Pointer< BPtr_Value > >::pointer_to(lhs),
        boost::intrusive::pointer_traits< Bounded_Pointer< BPtr_Value > >::pointer_to(rhs));
}

struct List_BPtr_Node_Traits
{
    typedef BPtr_Value                     val_t;
    typedef val_t                          node;
    typedef Bounded_Pointer< val_t >       node_ptr;
    typedef Bounded_Pointer< const val_t > const_node_ptr;

    static node_ptr get_previous(const_node_ptr p)      { return p->_previous; }
    static void set_previous(node_ptr p, node_ptr prev) { p->_previous = prev; }
    static node_ptr get_next(const_node_ptr p)          { return p->_next; }
    static void set_next(node_ptr p, node_ptr next)     { p->_next = next; }
};

struct RBTree_BPtr_Node_Traits
{
    typedef BPtr_Value                     val_t;
    typedef val_t                          node;
    typedef Bounded_Pointer< val_t >       node_ptr;
    typedef Bounded_Pointer< const val_t > const_node_ptr;
    typedef char                           color;

    static node_ptr get_parent(const_node_ptr p)        { return p->_parent; }
    static void set_parent(node_ptr p, node_ptr parent) { p->_parent = parent; }
    static node_ptr get_left(const_node_ptr p)          { return p->_l_child; }
    static void set_left(node_ptr p, node_ptr l_child)  { p->_l_child = l_child; }
    static node_ptr get_right(const_node_ptr p)         { return p->_r_child; }
    static void set_right(node_ptr p, node_ptr r_child) { p->_r_child = r_child; }
    static color get_color(const_node_ptr p)            { return p->_extra; }
    static void set_color(node_ptr p, color c)          { p->_extra = c; }
    static color black()                                { return 0; }
    static color red()                                  { return 1; }
};

struct AVLTree_BPtr_Node_Traits
{
    typedef BPtr_Value                     val_t;
    typedef val_t                          node;
    typedef Bounded_Pointer< val_t >       node_ptr;
    typedef Bounded_Pointer< const val_t > const_node_ptr;
    typedef char                           balance;

    static node_ptr get_parent(const_node_ptr p)        { return p->_parent; }
    static void set_parent(node_ptr p, node_ptr parent) { p->_parent = parent; }
    static node_ptr get_left(const_node_ptr p)          { return p->_l_child; }
    static void set_left(node_ptr p, node_ptr l_child)  { p->_l_child = l_child; }
    static node_ptr get_right(const_node_ptr p)         { return p->_r_child; }
    static void set_right(node_ptr p, node_ptr r_child) { p->_r_child = r_child; }
    static balance get_balance(const_node_ptr p)        { return p->_extra; }
    static void set_balance(node_ptr p, balance b)      { p->_extra = b; }
    static balance negative()                           { return -1; }
    static balance zero()                               { return 0; }
    static balance positive()                           { return 1; }
};

struct Tree_BPtr_Node_Traits
{
    typedef BPtr_Value                     val_t;
    typedef val_t                          node;
    typedef Bounded_Pointer< val_t >       node_ptr;
    typedef Bounded_Pointer< const val_t > const_node_ptr;

    static node_ptr get_parent(const_node_ptr p)        { return p->_parent; }
    static void set_parent(node_ptr p, node_ptr parent) { p->_parent = parent; }
    static node_ptr get_left(const_node_ptr p)          { return p->_l_child; }
    static void set_left(node_ptr p, node_ptr l_child)  { p->_l_child = l_child; }
    static node_ptr get_right(const_node_ptr p)         { return p->_r_child; }
    static void set_right(node_ptr p, node_ptr r_child) { p->_r_child = r_child; }
};

template < typename Node_Traits >
struct BPtr_Value_Traits
{
    typedef Node_Traits                           node_traits;
    typedef typename node_traits::val_t           value_type;
    typedef typename node_traits::node_ptr        node_ptr;
    typedef typename node_traits::const_node_ptr  const_node_ptr;
    typedef node_ptr                              pointer;
    typedef const_node_ptr                        const_pointer;
    typedef Bounded_Reference< value_type >       reference;
    typedef Bounded_Reference< const value_type > const_reference;

    static const boost::intrusive::link_mode_type link_mode = boost::intrusive::safe_link;

    static const_node_ptr to_node_ptr(const_reference v) { return &v; }
    static node_ptr to_node_ptr(reference v)             { return &v; }
    static const_pointer to_value_ptr(const_node_ptr p)  { return p; }
    static pointer to_value_ptr(node_ptr p)              { return p; }
};

template < typename >
struct Value_Container;

template <>
struct Value_Container< BPtr_Value >
{
    typedef Bounded_Reference_Cont< BPtr_Value > type;
};

namespace test
{

template <>
class new_cloner< BPtr_Value >
{
public:
    typedef BPtr_Value value_type;
    typedef Bounded_Pointer< value_type > pointer;
    typedef Bounded_Reference< const value_type > const_reference;
    typedef Bounded_Allocator< value_type > allocator_type;

    pointer operator () (const_reference r)
    {
        pointer p = allocator_type().allocate(1);
        new (p.raw()) value_type(r);
        return p;
    }
};

template <>
class delete_disposer< BPtr_Value >
{
public:
    typedef BPtr_Value value_type;
    typedef Bounded_Pointer< value_type > pointer;
    typedef Bounded_Allocator< value_type > allocator_type;

    void operator () (pointer p)
    {
        p->~value_type();
        allocator_type().deallocate(p, 1);
    }
};

} // namespace test

} // namespace intrusive
} // namespace boost


#endif
