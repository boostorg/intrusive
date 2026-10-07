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

//Same checks as container_size_test.cpp, but disabling __declspec(empty_bases)
//so that MSVC uses the layout of compilers older than Visual Studio 2015 Update 2.
//Container sizes must not grow on those compilers.
#define BOOST_INTRUSIVE_DISABLE_EMPTY_BASES
#include "container_size_test.cpp"
