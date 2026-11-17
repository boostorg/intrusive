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

#include <boost/intrusive/detail/workaround.hpp>
#include <boost/move/utility_core.hpp>
#include <boost/move/detail/type_traits.hpp>

namespace boost {
namespace intrusive {
namespace detail {

//Holds the (usually empty) value traits of a container as a base class
//so that the empty base optimization applies.
template<class ValueTraits>
struct value_traits_holder
   : public ValueTraits
{
   BOOST_INTRUSIVE_FORCEINLINE value_traits_holder()
      BOOST_NOEXCEPT_IF(::boost::move_detail::is_nothrow_default_constructible<ValueTraits>::value)
      :  ValueTraits()
   {}

   BOOST_INTRUSIVE_FORCEINLINE explicit value_traits_holder(const ValueTraits &val_traits)
      BOOST_NOEXCEPT_IF(::boost::move_detail::is_nothrow_copy_constructible<ValueTraits>::value)
      :  ValueTraits(val_traits)
   {}

   BOOST_INTRUSIVE_FORCEINLINE explicit value_traits_holder(BOOST_RV_REF(ValueTraits) val_traits)
      BOOST_NOEXCEPT_IF(::boost::move_detail::is_nothrow_move_constructible<ValueTraits>::value)
      :  ValueTraits(::boost::move(val_traits))
   {}
};

}  //namespace detail{
}  //namespace intrusive{
}  //namespace boost{

#endif //BOOST_INTRUSIVE_DETAIL_VALUE_TRAITS_HOLDER_HPP
