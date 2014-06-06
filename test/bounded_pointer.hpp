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

#ifndef BOUNDED_POINTER_HPP
#define BOUNDED_POINTER_HPP

#include <iostream>
#include <cstdlib>
#include <cassert>
#include <vector>
#include <boost/utility/enable_if.hpp>
#include <boost/mpl/if.hpp>
#include <boost/mpl/and.hpp>
#include <boost/intrusive/pointer_traits.hpp>
#include <boost/type_traits.hpp>

#define CONST_CONVERSIONS(_type, _t) \
    operator const _type< typename boost::add_const< _t >::type >& () const \
    { return *reinterpret_cast< const _type< typename boost::add_const< _t >::type >* >(this); } \
    operator _type< typename boost::add_const< _t >::type >& () \
    { return *reinterpret_cast< _type< typename boost::add_const< _t >::type >* >(this); } \
    \
    const _type< typename boost::remove_const< _t >::type >& unconst() const \
    { return *reinterpret_cast< const _type< typename boost::remove_const< _t >::type >* >(this); } \
    _type< typename boost::remove_const< _t >::type >& unconst() \
    { return *reinterpret_cast< _type< typename boost::remove_const< _t >::type >* >(this); }


template < typename T >
class Bounded_Pointer;
template < typename T >
class Bounded_Reference;
template < typename T >
class Bounded_Allocator;


template < typename T >
class Bounded_Pointer
{
public:
    typedef typename boost::remove_const< T >::type mut_val_t;
    typedef const mut_val_t const_val_t;

    template <class U>
    struct rebind
    {
        typedef typename boost::mpl::if_<
            boost::is_same<
                typename boost::remove_const< U >::type,
                typename boost::remove_const< T >::type >,
            Bounded_Pointer< U >,
            U*
        >::type type;
    };

    Bounded_Pointer() : _offset(255) {}
    Bounded_Pointer(const Bounded_Pointer& other) : _offset(other._offset) {}
    Bounded_Pointer& operator = (const Bounded_Pointer& other) { _offset = other._offset; return *this; }
    CONST_CONVERSIONS(Bounded_Pointer, T)

    static mut_val_t* base()
    {
        assert(Bounded_Allocator< mut_val_t >::inited());
        return &Bounded_Allocator< mut_val_t >::_base[0];
    }

    operator bool() const { return _offset != 255; }

    T* raw() const { return base() + _offset; }
    Bounded_Reference< T > operator * () const { return Bounded_Reference< T >(*this); }
    T* operator -> () const { return raw(); }
    Bounded_Pointer& operator ++ () { ++_offset; return *this; }
    Bounded_Pointer operator ++ (int) { Bounded_Pointer res(*this); ++(*this); return res; }

    template < typename U >
    //           typename boost::enable_if< boost::is_same< typename boost::remove_const< U >::type,
    //                                                      typename boost::remove_const< T >::type >, int >::type = 42 >
    bool operator == (const Bounded_Pointer< U >& rhs) const
    {
        return _offset == rhs._offset;
    }
    template < typename U >
    //           typename boost::enable_if< boost::is_same< typename boost::remove_const< U >::type,
    //                                                      typename boost::remove_const< T >::type >, int >::type = 42 >
    bool operator < (const Bounded_Pointer< U >& rhs) const
    {
        return _offset < rhs._offset;
    }

    friend std::ostream& operator << (std::ostream& os, const Bounded_Pointer< T >& ptr)
    {
        os << static_cast< int >(ptr._offset);
        return os;
    }
private:
    template <typename> friend class Bounded_Pointer;
    friend class Bounded_Reference< T >;
    friend class Bounded_Allocator< T >;

    uint8_t _offset;
}; // class Bounded_Pointer

template < typename T >
class Bounded_Reference
{
public:
    typedef typename boost::remove_const< T >::type mut_val_t;
    typedef const mut_val_t const_val_t;

    Bounded_Reference() : _offset(255) {}
    Bounded_Reference(const Bounded_Reference& other) : _offset(other._offset) {}
    CONST_CONVERSIONS(Bounded_Reference, T)

    T& raw() const { assert(_offset != 255); return *(Bounded_Pointer< T >::base() + _offset); }
    operator T& () const { assert(_offset != 255); return raw(); }
    Bounded_Pointer< T > operator & () const { assert(_offset != 255); Bounded_Pointer< T > res; res._offset = _offset; return res; }

    Bounded_Reference& operator = (const T& rhs) { assert(_offset != 255); raw() = rhs; return *this; }
    Bounded_Reference& operator = (const Bounded_Reference& rhs) { assert(_offset != 255); raw() = rhs.raw(); return *this; }

    friend std::ostream& operator << (std::ostream& os, const Bounded_Reference< T >& ref)
    {
        os << "[bptr=" << static_cast< int >(ref._offset) << ",deref=" << ref.raw() << "]";
        return os;
    }

    // the copy asop is shallow; we need swap overload to shuffle a vector of references
    friend void swap(Bounded_Reference& lhs, Bounded_Reference& rhs)
    {
        std::swap(lhs._offset, rhs._offset);
    }

private:
    friend class Bounded_Pointer< T >;
    Bounded_Reference(Bounded_Pointer< T > bptr) : _offset(bptr._offset) { assert(_offset != 255); }

    uint8_t _offset;
}; // class Bounded_Reference

template < typename T >
class Bounded_Allocator
{
public:
    typedef T value_type;
    typedef Bounded_Pointer< T > pointer;

    pointer allocate(size_t n)
    {
        assert(inited());
        assert(n == 1);
        pointer p;
        uint8_t i;
        for (i = 0; i < 255 and _in_use[i]; ++i);
        assert(i < 255);
        p._offset = i;
        _in_use[p._offset] = true;
        //std::clog << "allocating node " << static_cast< int >(p._offset) << "\n";
        return p;
    }
    void deallocate(pointer p, size_t n)
    {
        assert(inited());
        assert(n == 1);
        assert(_in_use[p._offset]);
        //std::clog << "deallocating node " << static_cast< int >(p._offset) << "\n";
        _in_use[p._offset] = false;
    }

    // static methods
    static void init()
    {
        assert(_in_use.empty());
        _in_use = std::vector< bool >(255, false);
        // allocate non-constructed storage
        _base = static_cast< T* >(::operator new [] (255 * sizeof(T)));
    }
    static bool inited()
    {
        return _in_use.size() == 255;
    }
    static bool is_clear()
    {
        assert(inited());
        for (uint8_t i = 0; i < 255; ++i)
        {
            if (_in_use[i])
            {
                return false;
            }
        }
        return true;
    }
    static void destroy()
    {
        // deallocate storage without destructors
        ::operator delete [] (_base);
        _in_use.clear();
    }

private:
    friend class Bounded_Pointer< T >;
    friend class Bounded_Pointer< const T >;
    static T* _base;
    static std::vector< bool > _in_use;
}; // class Bounded_Allocator

template < typename T >
T* Bounded_Allocator< T >::_base = NULL;

template < typename T >
std::vector< bool > Bounded_Allocator< T >::_in_use;


template < typename T >
class Bounded_Reference_Cont
    : private std::vector< Bounded_Reference< T > >
{
private:
    typedef T val_type;
    typedef std::vector< Bounded_Reference< T > > Base;
    typedef Bounded_Allocator< T > allocator_type;
    typedef Bounded_Pointer< T > pointer;

public:
    typedef typename Base::value_type value_type;
    typedef typename Base::iterator iterator;
    typedef typename Base::const_iterator const_iterator;
    typedef typename Base::reference reference;
    typedef typename Base::const_reference const_reference;
    typedef typename Base::reverse_iterator reverse_iterator;
    typedef typename Base::const_reverse_iterator const_reverse_iterator;

    iterator begin() { return Base::begin(); }
    iterator end() { return Base::end(); }
    const_iterator begin() const { return Base::begin(); }
    const_iterator end() const { return Base::end(); }
    reverse_iterator rbegin() { return Base::rbegin(); }
    reverse_iterator rend() { return Base::rend(); }
    const_reverse_iterator rbegin() const { return Base::rbegin(); }
    const_reverse_iterator rend() const { return Base::rend(); }
    reference front() { return Base::front(); }
    const_reference front() const { return Base::front(); }
    reference back() { return Base::back(); }
    const_reference back() const { return Base::back(); }
    size_t size() const { return Base::size(); }
    reference operator [] (size_t i) { return Base::operator [] (i); }
    const_reference operator [] (size_t i) const { return Base::operator [] (i); }
    void push_back(const value_type& v) { Base::push_back(v); }

    Bounded_Reference_Cont(size_t n = 0) : Base(), _allocator()
    {
        for (size_t i = 0; i < n; ++i)
        {
            pointer p = _allocator.allocate(1);
            new (p.raw()) val_type();
            Base::push_back(*p);
        }
    }
    Bounded_Reference_Cont(const Bounded_Reference_Cont& other) : Base(), _allocator(other._allocator)
    {
        //std::clog << "copying values container\n";
        *this = other;
    }
    template < typename InputIterator >
    Bounded_Reference_Cont(InputIterator it_start, InputIterator it_end) : Base(), _allocator()
    {
        for (InputIterator it = it_start; it != it_end; ++it)
        {
            pointer p = _allocator.allocate(1);
            new (p.raw()) val_type(*it);
            Base::push_back(*p);
        }
    }
    ~Bounded_Reference_Cont()
    {
        clear();
    }
    void clear()
    {
        while (not Base::empty())
        {
            pointer p = &Base::back();
            p->~val_type();
            _allocator.deallocate(p, 1);
            Base::pop_back();
        }
    }
    Bounded_Reference_Cont& operator = (const Bounded_Reference_Cont& other)
    {
        if (&other != this)
        {
            clear();
            for (typename Base::const_iterator it = other.begin(); it != other.end(); ++it)
            {
                pointer p = _allocator.allocate(1);
                new (p.raw()) val_type(*it);
                Base::push_back(*p);
            }
        }
        return *this;
    }

private:
    allocator_type _allocator;
}; // class Bounded_Reference_Cont

template < typename T >
class Bounded_Pointer_Holder
{
public:
    typedef T value_type;
    typedef Bounded_Pointer< value_type > pointer;
    typedef Bounded_Pointer< const value_type > const_pointer;
    typedef Bounded_Allocator< value_type > allocator_type;

    Bounded_Pointer_Holder() : _ptr(allocator_type().allocate(1))
    {
        new (_ptr.raw()) value_type();
    }
    ~Bounded_Pointer_Holder()
    {
        _ptr->~value_type();
        allocator_type().deallocate(_ptr, 1);
    }

    const_pointer get_node () const { return _ptr; }
    pointer get_node () { return _ptr; }

private:
    const pointer _ptr;
}; // class Bounded_Pointer_Holder


namespace boost
{
namespace intrusive
{

template < typename T >
struct pointer_traits< Bounded_Pointer< T > >
{
    typedef T element_type;
    typedef Bounded_Pointer< T > pointer;
    typedef Bounded_Pointer< const T > const_pointer;
    typedef ptrdiff_t difference_type;
    typedef Bounded_Reference< T > reference;

    template <class U>
    struct rebind_pointer
    {
        typedef typename Bounded_Pointer< T >::template rebind< U >::type type;
    };

    static pointer pointer_to(reference r) { return &r; }
    static pointer const_cast_from(const_pointer cptr) { return cptr.unconst(); }
};

} // namespace intrusive
} // namespace boost

template < typename T, typename U >
//           typename boost::enable_if< boost::is_same< typename boost::remove_const< T >::type,
//                                                      typename boost::remove_const< U >::type >, int >::type = 42 >
bool operator != (const Bounded_Pointer< T >& lhs, const Bounded_Pointer< U >& rhs)
{
    return !(lhs == rhs);
}
template < typename T >
bool operator == (const Bounded_Pointer< T >& lhs, const void* p)
{
    assert(!p);
    return lhs == Bounded_Pointer< T >();
}
template < typename T >
bool operator == (const void* p, const Bounded_Pointer< T >& rhs)
{
    assert(!p);
    return Bounded_Pointer< T >() == rhs;
}
template < typename T >
bool operator != (const Bounded_Pointer< T >& lhs, const void* p)
{
    assert(!p);
    return lhs != Bounded_Pointer< T >();
}
template < typename T >
bool operator != (const void* p, const Bounded_Pointer< T >& rhs)
{
    assert(!p);
    return Bounded_Pointer< T >() != rhs;
}


#endif
