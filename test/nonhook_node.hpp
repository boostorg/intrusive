#ifndef BOOST_INTRUSIVE_DETAIL_NONHOOK_NODE_HPP
#define BOOST_INTRUSIVE_DETAIL_NONHOOK_NODE_HPP

#include <boost/intrusive/detail/config_begin.hpp>
#include <boost/intrusive/list.hpp>
#include <boost/intrusive/pointer_traits.hpp>
#include <boost/intrusive/detail/utilities.hpp>


namespace boost{
namespace intrusive{


template < typename Node_Traits, template <typename> class Node_Algorithms >
struct nonhook_node_member : public Node_Traits::node
{
    typedef Node_Traits                                               node_traits;
    typedef typename node_traits::node                                node;
    typedef typename node_traits::node_ptr                            node_ptr;
    typedef typename node_traits::const_node_ptr                      const_node_ptr;
    typedef Node_Algorithms< node_traits >                            node_algorithms;

    nonhook_node_member()
    {
        node_algorithms::init(pointer_traits<node_ptr>::pointer_to(static_cast< node& >(*this)));
    }
    nonhook_node_member(const nonhook_node_member& rhs)
    {
        if (rhs.is_linked())
        {
            std::cerr << "nonhook_node_member copy ctor from linked: &=" << (void*)this << ", rhs=" << (void*)&rhs << std::endl;
        }
        *this = rhs;
    }
    nonhook_node_member& operator = (const nonhook_node_member& rhs)
    {
        if (is_linked() or rhs.is_linked())
        {
            std::cerr << "nonhook_node_member copy asop to/from linked: &=" << (void*)this << ", rhs=" << (void*)&rhs << std::endl;
        }
        static_cast< node& >(*this) = rhs;
        return *this;
    }

    void swap_nodes(nonhook_node_member& other)
    {
        node_algorithms::swap_nodes(pointer_traits<node_ptr>::pointer_to(static_cast< node& >(*this)),
                                    pointer_traits<node_ptr>::pointer_to(static_cast< node& >(other)));
    }
    bool is_linked() const
    {
        return !node_algorithms::unique(pointer_traits<const_node_ptr>::pointer_to(static_cast< const node& >(*this)));
    }
};

template < typename T, typename NonHook_Member, NonHook_Member T::* P, link_mode_type Link_Mode >
struct nonhook_node_member_value_traits
{
    typedef T                                                         value_type;
    typedef typename NonHook_Member::node_traits                      node_traits;
    typedef typename node_traits::node                                node;
    typedef typename node_traits::node_ptr                            node_ptr;
    typedef typename node_traits::const_node_ptr                      const_node_ptr;
    typedef typename pointer_traits<node_ptr>::
        template rebind_pointer<T>::type                              pointer;
    typedef typename pointer_traits<node_ptr>::
        template rebind_pointer<const T>::type                        const_pointer;
    typedef T &                                                       reference;
    typedef const T &                                                 const_reference;

    static const link_mode_type link_mode = Link_Mode;

    static node_ptr to_node_ptr(reference value)
    {
        return pointer_traits<node_ptr>::pointer_to(static_cast<node&>(value.*P));
    }

    static const_node_ptr to_node_ptr(const_reference value)
    {
        return pointer_traits<const_node_ptr>::pointer_to(static_cast<const node&>(value.*P));
    }
    
    static pointer to_value_ptr(node_ptr n)
    {
        return pointer_traits<pointer>::pointer_to
        (*detail::parent_from_member<T, NonHook_Member>
        (static_cast<NonHook_Member*>(boost::intrusive::detail::to_raw_pointer(n)), P));
    }

    static const_pointer to_value_ptr(const_node_ptr n)
    {
        return pointer_traits<const_pointer>::pointer_to
        (*detail::parent_from_member<T, NonHook_Member>
        (static_cast<const NonHook_Member*>(boost::intrusive::detail::to_raw_pointer(n)), P));
    }
};

}
}

#endif
