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
#ifndef BOOST_INTRUSIVE_DETAIL_PARENT_FROM_MEMBER_HPP
#define BOOST_INTRUSIVE_DETAIL_PARENT_FROM_MEMBER_HPP

#ifndef BOOST_CONFIG_HPP
#  include <boost/config.hpp>
#endif

#if defined(BOOST_HAS_PRAGMA_ONCE)
#  pragma once
#endif

#include <boost/intrusive/detail/config_begin.hpp>
#include <boost/intrusive/detail/workaround.hpp>
#include <boost/intrusive/detail/assert.hpp>
#include <cstddef>

#if defined(_MSC_VER)
#  define BOOST_INTRUSIVE_MSVC_ABI_PTR_TO_MEMBER
#endif

namespace boost {
namespace intrusive {
namespace detail {

template<class Parent, class Member>
BOOST_INTRUSIVE_FORCEINLINE std::ptrdiff_t offset_from_pointer_to_member(const Member Parent::* ptr_to_member)
{
   //The implementation of a pointer to member is compiler dependent.
   #if defined(BOOST_INTRUSIVE_MSVC_ABI_PTR_TO_MEMBER)

   //MSVC ABI represents a pointer to data member as 1, 2 or 3 32-bit ints
   //(even in 64 bit mode) depending on the inheritance model of Parent:
   //
   // - single/multiple inheritance:  4 bytes: { field_offset }
   // - virtual inheritance:          8 bytes: { field_offset, vbtable_offset }
   // - unspecified (incomplete Parent, /vmg): 12 bytes: { field_offset, vbptr_offset, vbtable_offset }
   //
   //and the compiler obtains the member address from the parent as:
   //
   //   vbase  = vbtable_offset == 0 ? 0 : vbptr_offset + *(int*)(*(char**)(parent + vbptr_offset) + vbtable_offset)
   //   member = parent + vbase + field_offset
   //
   //field_offset is always the first int and vbtable_offset, when present, is always the last int.
   //vbtable_offset is zero unless the member is located in a virtual base of Parent. In that case
   //the distance from Parent to the member is not a constant, as it depends on the most derived type
   //(the vbtable is stored in the object, so it can't be read without knowing the parent address),
   //and the parent address can't be recovered. Note that ISO C++ ([conv.mem]) does not allow
   //converting a pointer to member of a virtual base to a pointer to member of the derived class,
   //MSVC accepts it as an extension, so this function asserts to detect that non-standard case.
   typedef const Member Parent::* ptr_to_member_t;
   union caster_union
   {
      ptr_to_member_t ptr_to_member;
      int offsets[sizeof(ptr_to_member_t)/sizeof(int)];
   } caster;
   //Note: sizeof(caster) can be bigger than sizeof(ptr_to_member_t) due to alignment (x86 12-byte pointers to member have 8 byte alignment)
   BOOST_INTRUSIVE_STATIC_ASSERT( sizeof(caster.offsets) == sizeof(ptr_to_member_t) && sizeof(caster.offsets) <= 3u*sizeof(int) );

   caster.ptr_to_member = ptr_to_member;
   //Members located in virtual bases are not supported (vbtable_offset != 0)
   BOOST_INTRUSIVE_INVARIANT_ASSERT( sizeof(caster.offsets) == sizeof(int) || caster.offsets[sizeof(caster.offsets)/sizeof(int) - 1u] == 0 );
   return std::ptrdiff_t(caster.offsets[0]);

   //This works with gcc, msvc, ac++, ibmcpp
   #elif defined(__GNUC__)   || defined(__HP_aCC) || defined(BOOST_INTEL) || \
         defined(__IBMCPP__) || defined(__DECCXX)
   const Parent * const parent = 0;
   const char *const member = static_cast<const char*>(static_cast<const void*>(&(parent->*ptr_to_member)));
   return std::ptrdiff_t(member - static_cast<const char*>(static_cast<const void*>(parent)));
   #else
   //This is the traditional C-front approach: __MWERKS__, __DMC__, __SUNPRO_CC
   union caster_union
   {
      const Member Parent::* ptr_to_member;
      std::ptrdiff_t offset;
   } caster;
   caster.ptr_to_member = ptr_to_member;
   return caster.offset - 1;
   #endif
}

template<class Parent, class Member>
BOOST_INTRUSIVE_FORCEINLINE Parent *parent_from_member(Member *member, const Member Parent::* ptr_to_member)
{
   return static_cast<Parent*>(static_cast<void*>
      (reinterpret_cast<char*>(member) - offset_from_pointer_to_member(ptr_to_member)));
}

template<class Parent, class Member>
BOOST_INTRUSIVE_FORCEINLINE const Parent *parent_from_member(const Member *member, const Member Parent::* ptr_to_member)
{
   return static_cast<const Parent*>(static_cast<const void*>
      (reinterpret_cast<const char*>(member) - offset_from_pointer_to_member(ptr_to_member)));
}

}  //namespace detail {
}  //namespace intrusive {
}  //namespace boost {

#include <boost/intrusive/detail/config_end.hpp>

#endif   //#ifndef BOOST_INTRUSIVE_DETAIL_PARENT_FROM_MEMBER_HPP
