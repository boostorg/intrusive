//////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Ion Gaztanaga 2014-2014. Distributed under the Boost
// Software License, Version 1.0. (See accompanying file
// LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/intrusive for documentation.
//
//////////////////////////////////////////////////////////////////////////////

#ifndef BOOST_INTRUSIVE_DETAIL_POINTER_ELEMENT_HPP
#define BOOST_INTRUSIVE_DETAIL_POINTER_ELEMENT_HPP

#ifndef BOOST_CONFIG_HPP
#  include <boost/config.hpp>
#endif

#if defined(BOOST_HAS_PRAGMA_ONCE)
#  pragma once
#endif

#ifndef BOOST_INTRUSIVE_DETAIL_WORKAROUND_HPP
#include <boost/intrusive/detail/workaround.hpp>
#endif   //BOOST_INTRUSIVE_DETAIL_WORKAROUND_HPP

#include <boost/move/detail/pointer_element.hpp>

namespace boost {
namespace intrusive {

using ::boost::movelib::pointer_element;

namespace detail {

using ::boost::movelib::detail::first_param;

}

}  //namespace intrusive {
}  //namespace boost {

#endif // defined(BOOST_INTRUSIVE_DETAIL_POINTER_ELEMENT_HPP)
