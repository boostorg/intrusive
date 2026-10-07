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

#ifndef BOOST_INTRUSIVE_DETAIL_VALUE_TRAITS_HOLDER_HPP
#define BOOST_INTRUSIVE_DETAIL_VALUE_TRAITS_HOLDER_HPP

#ifndef BOOST_CONFIG_HPP
#  include <boost/config.hpp>
#endif

#if defined(BOOST_HAS_PRAGMA_ONCE)
#  pragma once
#endif

namespace boost {
namespace intrusive {
namespace detail {

//Holds the (usually empty) value traits of a container as a base class
//so that the empty base optimization applies.
template<class ValueTraits>
struct value_traits_holder
   : public ValueTraits
{
   inline explicit value_traits_holder(const ValueTraits &val_traits)
      :  ValueTraits(val_traits)
   {}
};

}  //namespace detail{
}  //namespace intrusive{
}  //namespace boost{

#endif //BOOST_INTRUSIVE_DETAIL_VALUE_TRAITS_HOLDER_HPP
