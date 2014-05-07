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

    bool is_linked() const { return _previous or _next; }

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

    static node_ptr get_previous(const_node_ptr p) { return p->_previous; }
    static void set_previous(node_ptr p, node_ptr prev) { p->_previous = prev; }
    static node_ptr get_next(const_node_ptr p) { return p->_next; }
    static void set_next(node_ptr p, node_ptr next) { p->_next = next; }
};

struct List_BPtr_Value_Traits
{
    typedef List_BPtr_Node_Traits                 node_traits;
    typedef node_traits::val_t                    value_type;
    typedef node_traits::node_ptr                 node_ptr;
    typedef node_traits::const_node_ptr           const_node_ptr;
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


#endif
