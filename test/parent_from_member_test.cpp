/////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Ion Gaztanaga  2014-2014
//
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/intrusive for documentation.
//
/////////////////////////////////////////////////////////////////////////////
#include <boost/intrusive/parent_from_member.hpp>
#include <boost/core/lightweight_test.hpp>

struct POD
{
   int   int_;
   float float_;
}pod;

struct Derived
   : public POD
{
   int   derived_int_;
   float derived_float_;
}derived;

struct Abstract
{
   int   abstract_int_;
   float abstract_float_;
   virtual void virtual_func1() = 0;
   virtual void virtual_func2() = 0;
   virtual ~Abstract(){}
};

struct DerivedPoly
   : public Abstract
{
   int   derivedpoly_int_;
   float derivedpoly_float_;
   virtual void virtual_func1() BOOST_OVERRIDE {}
   virtual void virtual_func2() BOOST_OVERRIDE {}
   Abstract *abstract()             {  return this; }
   Abstract const *abstract() const {  return this; }
} derivedpoly;

struct MultiInheritance
   : public Derived, public DerivedPoly
{
   int   multiinheritance_int_;
   float multiinheritance_float_;
} multiinheritance;

struct Abstract2
{
   int   abstract2_int_;
   float abstract2_float_;
   virtual void virtual_func1() = 0;
   virtual void virtual_func2() = 0;
   virtual ~Abstract2(){}
};

struct DerivedPoly2
   : public Abstract2
{
   int   derivedpoly2_int_;
   float derivedpoly2_float_;
   virtual void virtual_func1() BOOST_OVERRIDE {}
   virtual void virtual_func2() BOOST_OVERRIDE {}
   Abstract2 *abstract2()             {  return this; }
   Abstract2 const *abstract2() const {  return this; }
   virtual ~DerivedPoly2(){}
} derivedpoly2;

struct MultiInheritance2
   : public DerivedPoly, public DerivedPoly2
{
   int   multiinheritance2_int_;
   float multiinheritance2_float_;
} multiinheritance2;

struct VirtualDerivedPoly
   : public virtual Derived
{
   int   virtualderivedpoly_int_;
   float virtualderivedpoly_float_;
   virtual void f1(){}
   virtual void f2(){}
   virtual ~VirtualDerivedPoly(){}
} virtualderivedpoly;

struct VirtualMultipleDerivedPoly
   : public virtual Derived, virtual public DerivedPoly
{
   int   virtualmultiplederivedpoly_int_;
   float virtualmultiplederivedpoly_float_;
   virtual void f1(){}
   virtual void f2(){}
   virtual ~VirtualMultipleDerivedPoly(){}
} virtualmultiplederivedpoly;

struct VirtualDerived
   : public virtual Derived
{
   int   virtualderived_int_;
   float virtualderived_float_;
   virtual void f1(){}
   virtual void f2(){}
   virtual ~VirtualDerived(){}
} virtualderived;

struct VirtualMixed
   : public Derived, public virtual DerivedPoly2
{
   int   virtualmixed_int_;
   float virtualmixed_float_;
   virtual ~VirtualMixed(){}
} virtualmixed;

#ifdef BOOST_INTRUSIVE_MSVC_ABI_PTR_TO_MEMBER
//Force the most general (12 byte) pointer to data member representation for the following classes
#pragma pointers_to_members(full_generality, virtual_inheritance)

struct FullGenerality
   : public Derived, public DerivedPoly
{
   int   fullgenerality_int_;
   float fullgenerality_float_;
} fullgenerality;

struct FullGeneralityVirtual
   : public virtual Derived, public DerivedPoly
{
   int   fullgeneralityvirtual_int_;
   float fullgeneralityvirtual_float_;
   virtual ~FullGeneralityVirtual(){}
} fullgeneralityvirtual;

#endif

using namespace boost::intrusive;

int main()
{
   //POD
   BOOST_TEST(&pod == get_parent_from_member(&pod.int_,   &POD::int_));
   BOOST_TEST(&pod == get_parent_from_member(&pod.float_, &POD::float_));

   //Derived
   BOOST_TEST(&derived == get_parent_from_member(&derived.int_,   &Derived::int_));
   BOOST_TEST(&derived == get_parent_from_member(&derived.float_, &Derived::float_));
   BOOST_TEST(&derived == get_parent_from_member(&derived.derived_int_,   &Derived::derived_int_));
   BOOST_TEST(&derived == get_parent_from_member(&derived.derived_float_, &Derived::derived_float_));

   //Abstract
   BOOST_TEST(derivedpoly.abstract() == get_parent_from_member(&derivedpoly.abstract_int_,   &Abstract::abstract_int_));
   BOOST_TEST(derivedpoly.abstract() == get_parent_from_member(&derivedpoly.abstract_float_, &Abstract::abstract_float_));

   //DerivedPoly
   BOOST_TEST(&derivedpoly == get_parent_from_member(&derivedpoly.abstract_int_,   &DerivedPoly::abstract_int_));
   BOOST_TEST(&derivedpoly == get_parent_from_member(&derivedpoly.abstract_float_, &DerivedPoly::abstract_float_));
   BOOST_TEST(&derivedpoly == get_parent_from_member(&derivedpoly.derivedpoly_int_,   &DerivedPoly::derivedpoly_int_));
   BOOST_TEST(&derivedpoly == get_parent_from_member(&derivedpoly.derivedpoly_float_, &DerivedPoly::derivedpoly_float_));

   //MultiInheritance
   BOOST_TEST(multiinheritance.abstract() == get_parent_from_member(&multiinheritance.abstract_int_,   &MultiInheritance::abstract_int_));
   BOOST_TEST(multiinheritance.abstract() == get_parent_from_member(&multiinheritance.abstract_float_, &MultiInheritance::abstract_float_));
   BOOST_TEST(&multiinheritance == get_parent_from_member(&multiinheritance.derivedpoly_int_,   &MultiInheritance::derivedpoly_int_));
   BOOST_TEST(&multiinheritance == get_parent_from_member(&multiinheritance.derivedpoly_float_, &MultiInheritance::derivedpoly_float_));
   BOOST_TEST(&multiinheritance == get_parent_from_member(&multiinheritance.int_,   &MultiInheritance::int_));
   BOOST_TEST(&multiinheritance == get_parent_from_member(&multiinheritance.float_, &MultiInheritance::float_));
   BOOST_TEST(&multiinheritance == get_parent_from_member(&multiinheritance.derived_int_,   &MultiInheritance::derived_int_));
   BOOST_TEST(&multiinheritance == get_parent_from_member(&multiinheritance.derived_float_, &MultiInheritance::derived_float_));

   BOOST_TEST(multiinheritance.abstract() == get_parent_from_member(&multiinheritance.abstract_int_,   &MultiInheritance::abstract_int_));
   BOOST_TEST(multiinheritance.abstract() == get_parent_from_member(&multiinheritance.abstract_float_, &MultiInheritance::abstract_float_));
   BOOST_TEST(&multiinheritance == get_parent_from_member(&multiinheritance.derivedpoly_int_,   &MultiInheritance::derivedpoly_int_));
   BOOST_TEST(&multiinheritance == get_parent_from_member(&multiinheritance.derivedpoly_float_, &MultiInheritance::derivedpoly_float_));
   BOOST_TEST(multiinheritance2.abstract2() == get_parent_from_member(&multiinheritance2.abstract2_int_,   &MultiInheritance2::abstract2_int_));
   BOOST_TEST(multiinheritance2.abstract2() == get_parent_from_member(&multiinheritance2.abstract2_float_, &MultiInheritance2::abstract2_float_));
   BOOST_TEST(&multiinheritance2 == get_parent_from_member(&multiinheritance2.derivedpoly2_int_,   &MultiInheritance2::derivedpoly2_int_));
   BOOST_TEST(&multiinheritance2 == get_parent_from_member(&multiinheritance2.derivedpoly2_float_, &MultiInheritance2::derivedpoly2_float_));

   //Parents with virtual bases, members not located in virtual bases
   //(MSVC ABI: 8 byte pointer to member representation)
   #ifdef BOOST_INTRUSIVE_MSVC_ABI_PTR_TO_MEMBER
   //(unless compiling with /vmg, where all pointer to members use the 12 byte representation)
   const bool vmg = sizeof(&POD::int_) != sizeof(int);
   BOOST_TEST(vmg || sizeof(&VirtualDerived::virtualderived_int_) == 2*sizeof(int));
   BOOST_TEST(vmg || sizeof(&VirtualMultipleDerivedPoly::virtualmultiplederivedpoly_int_) == 2*sizeof(int));
   BOOST_TEST(vmg || sizeof(&VirtualMixed::virtualmixed_int_) == 2*sizeof(int));
   BOOST_TEST(vmg || sizeof(static_cast<int VirtualMixed::*>(&VirtualMixed::derived_int_)) == 2*sizeof(int));
   #endif
   BOOST_TEST(&virtualderived == get_parent_from_member(&virtualderived.virtualderived_int_,   &VirtualDerived::virtualderived_int_));
   BOOST_TEST(&virtualderived == get_parent_from_member(&virtualderived.virtualderived_float_, &VirtualDerived::virtualderived_float_));

   BOOST_TEST(&virtualderivedpoly == get_parent_from_member(&virtualderivedpoly.virtualderivedpoly_int_,   &VirtualDerivedPoly::virtualderivedpoly_int_));
   BOOST_TEST(&virtualderivedpoly == get_parent_from_member(&virtualderivedpoly.virtualderivedpoly_float_, &VirtualDerivedPoly::virtualderivedpoly_float_));
   BOOST_TEST(&virtualmultiplederivedpoly == get_parent_from_member(&virtualmultiplederivedpoly.virtualmultiplederivedpoly_float_, &VirtualMultipleDerivedPoly::virtualmultiplederivedpoly_float_));
   BOOST_TEST(&virtualmultiplederivedpoly == get_parent_from_member(&virtualmultiplederivedpoly.virtualmultiplederivedpoly_int_,   &VirtualMultipleDerivedPoly::virtualmultiplederivedpoly_int_));
   BOOST_TEST(&virtualmultiplederivedpoly == get_parent_from_member(&virtualmultiplederivedpoly.derivedpoly_float_, &VirtualMultipleDerivedPoly::derivedpoly_float_));
   BOOST_TEST(&virtualmultiplederivedpoly == get_parent_from_member(&virtualmultiplederivedpoly.derivedpoly_int_,   &VirtualMultipleDerivedPoly::derivedpoly_int_));

   BOOST_TEST(&virtualmixed == get_parent_from_member(&virtualmixed.virtualmixed_int_,   &VirtualMixed::virtualmixed_int_));
   BOOST_TEST(&virtualmixed == get_parent_from_member(&virtualmixed.virtualmixed_float_, &VirtualMixed::virtualmixed_float_));
   //Members of non-virtual bases, explicitly converted to pointer to members of the class with virtual bases
   BOOST_TEST(&virtualmixed == (get_parent_from_member<VirtualMixed, int>  (&virtualmixed.derived_int_,   &VirtualMixed::derived_int_)));
   BOOST_TEST(&virtualmixed == (get_parent_from_member<VirtualMixed, float>(&virtualmixed.derived_float_, &VirtualMixed::derived_float_)));
   BOOST_TEST(&virtualmixed == (get_parent_from_member<VirtualMixed, int>  (&virtualmixed.int_,   &VirtualMixed::int_)));
   BOOST_TEST(&virtualmixed == (get_parent_from_member<VirtualMixed, float>(&virtualmixed.float_, &VirtualMixed::float_)));

   //MSVC ABI: full generality (12 byte) pointer to member representation
   #ifdef BOOST_INTRUSIVE_MSVC_ABI_PTR_TO_MEMBER
   BOOST_TEST(sizeof(&FullGenerality::fullgenerality_int_) == 3*sizeof(int));
   BOOST_TEST(sizeof(&FullGeneralityVirtual::fullgeneralityvirtual_int_) == 3*sizeof(int));
   BOOST_TEST(&fullgenerality == get_parent_from_member(&fullgenerality.fullgenerality_int_,   &FullGenerality::fullgenerality_int_));
   BOOST_TEST(&fullgenerality == get_parent_from_member(&fullgenerality.fullgenerality_float_, &FullGenerality::fullgenerality_float_));
   BOOST_TEST(&fullgenerality == get_parent_from_member(&fullgenerality.derived_int_,   &FullGenerality::derived_int_));
   BOOST_TEST(&fullgenerality == get_parent_from_member(&fullgenerality.derivedpoly_float_, &FullGenerality::derivedpoly_float_));
   BOOST_TEST(&fullgeneralityvirtual == get_parent_from_member(&fullgeneralityvirtual.fullgeneralityvirtual_int_,   &FullGeneralityVirtual::fullgeneralityvirtual_int_));
   BOOST_TEST(&fullgeneralityvirtual == get_parent_from_member(&fullgeneralityvirtual.fullgeneralityvirtual_float_, &FullGeneralityVirtual::fullgeneralityvirtual_float_));
   BOOST_TEST(&fullgeneralityvirtual == get_parent_from_member(&fullgeneralityvirtual.derivedpoly_float_, &FullGeneralityVirtual::derivedpoly_float_));
   BOOST_TEST(&fullgenerality == (get_parent_from_member<FullGenerality, int>(&fullgenerality.derivedpoly_int_, &FullGenerality::derivedpoly_int_)));
   BOOST_TEST(&fullgeneralityvirtual == (get_parent_from_member<FullGeneralityVirtual, int>(&fullgeneralityvirtual.derivedpoly_int_, &FullGeneralityVirtual::derivedpoly_int_)));
   #endif

   return boost::report_errors();
}
