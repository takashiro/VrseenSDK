#pragma once

#include "vglobal.h"

NV_NAMESPACE_BEGIN

template< typename T, typename UniqueType, UniqueType InitialValue >
class VNumber
{
public:
    // constructors
                        VNumber();
    explicit            VNumber( T const value );


// NOTE: the assignmnet of a Type value is implemented here and PURPOSEFULLY
// COMMENTED OUT to demonstrate that the whole point of this template is to
// enforce type safety. Assignment without explicit conversion to the templated
// type would break type safety.
/*
    Type &                operator=( T const value );
*/

    VNumber &    operator=( VNumber const & rhs );

    // comparison operators
    bool                operator==( VNumber const & rhs ) const;
    bool                operator!=( VNumber const & rhs ) const;

    bool                operator< ( VNumber const & rhs) const;
    bool                operator> ( VNumber const & rhs) const;
    bool                operator<=( VNumber const & rhs) const;
    bool                operator>=( VNumber const & rhs) const;

    // unary operators
    VNumber &  operator++();       // prefix
    VNumber    operator++( int );  // postfix

    VNumber &  operator--();       // prefix
    VNumber    operator--( int );  // postfix

    // compound assignment operators
    VNumber &  operator+=( VNumber const & rhs );
    VNumber &  operator-=( VNumber const & rhs );
    VNumber &  operator*=( VNumber const & rhs );
    VNumber &  operator/=( VNumber const & rhs );
    VNumber &  operator%=( VNumber const & rhs );

    // binary arithmetic operators
    VNumber    operator+( VNumber const & rhs ) const;
    VNumber    operator-( VNumber const & rhs ) const;
    VNumber    operator*( VNumber const & rhs ) const;
    VNumber    operator/( VNumber const & rhs ) const;
    VNumber    operator%( VNumber const & rhs ) const;

    T                   Get() const;
    void                Set( T const value );

	// for using as a handle
	void				Release() { Value = InitialValue; }

	bool				IsValid() const { return Value != InitialValue; }

private:
    T                   Value;    // the value itself
};

template< typename T, typename UniqueType, UniqueType InitialValue >
inline
VNumber< T, UniqueType, InitialValue >::VNumber() :
    Value( static_cast< T >( InitialValue ) )
{
}

template< typename T, typename UniqueType, UniqueType InitialValue >
inline
VNumber< T, UniqueType, InitialValue >::VNumber( T const value ) :
    Value( value )
{
}

template< typename T, typename UniqueType, UniqueType InitialValue >
inline
VNumber< T, UniqueType, InitialValue > & VNumber< T, UniqueType, InitialValue >::operator=( VNumber const & rhs )
{
    if ( &rhs != this )
    {
        this->Value = rhs.Value;
    }
    return *this;
}

template< typename T, typename UniqueType, UniqueType InitialValue >
inline
bool VNumber< T, UniqueType, InitialValue >::operator==( VNumber const & rhs ) const
{
    return this->Value == rhs.Value;
}

template< typename T, typename UniqueType, UniqueType InitialValue >
inline
bool VNumber< T, UniqueType, InitialValue >::operator!=( VNumber const & rhs ) const
{
    return !operator==( rhs );
}

template< typename T, typename UniqueType, UniqueType InitialValue >
inline
bool VNumber< T, UniqueType, InitialValue >::operator< ( VNumber const & rhs) const
{
    return this->Value < rhs.Value;
}

template< typename T, typename UniqueType, UniqueType InitialValue >
inline
bool VNumber< T, UniqueType, InitialValue >::operator> ( VNumber const & rhs) const
{
    return this->Value > rhs.Value;
}

template< typename T, typename UniqueType, UniqueType InitialValue >
inline
bool VNumber< T, UniqueType, InitialValue >::operator<=( VNumber const & rhs) const
{
    return this->Value <= rhs.Value;
}

template< typename T, typename UniqueType, UniqueType InitialValue >
inline
bool VNumber< T, UniqueType, InitialValue >::operator>=( VNumber const & rhs) const
{
    return this->Value >= rhs.Value;
}

template< typename T, typename UniqueType, UniqueType InitialValue >
inline
VNumber< T, UniqueType, InitialValue > &  VNumber< T, UniqueType, InitialValue >::operator++()
{
    this->Value++;
    return *this;
}

template< typename T, typename UniqueType, UniqueType InitialValue >
inline
VNumber< T, UniqueType, InitialValue > VNumber< T, UniqueType, InitialValue >::operator++( int )
{
    // postfix
    VNumber temp( *this );
    operator++();
    return temp;
}

template< typename T, typename UniqueType, UniqueType InitialValue >
inline
VNumber< T, UniqueType, InitialValue > & VNumber< T, UniqueType, InitialValue >::operator--()
{
    this->Value--;
    return *this;
}

template< typename T, typename UniqueType, UniqueType InitialValue >
inline
VNumber< T, UniqueType, InitialValue > VNumber< T, UniqueType, InitialValue >::operator--( int )
{
    // postfix
    VNumber temp( *this );
    operator--();
    return temp;
}

template< typename T, typename UniqueType, UniqueType InitialValue >
inline
VNumber< T, UniqueType, InitialValue > & VNumber< T, UniqueType, InitialValue >::operator+=( VNumber const & rhs )
{
    this->Value = this->Value + rhs.Value;
    return *this;
}

template< typename T, typename UniqueType, UniqueType InitialValue >
inline
VNumber< T, UniqueType, InitialValue > & VNumber< T, UniqueType, InitialValue >::operator-=( VNumber const & rhs )
{
    this->Value = this->Value - rhs.Value;
    return *this;
}

template< typename T, typename UniqueType, UniqueType InitialValue >
inline
VNumber< T, UniqueType, InitialValue > &    VNumber< T, UniqueType, InitialValue >::operator*=( VNumber const & rhs )
{
    this->Value = this->Value * rhs.Value;
    return *this;
}

template< typename T, typename UniqueType, UniqueType InitialValue >
inline
VNumber< T, UniqueType, InitialValue > &    VNumber< T, UniqueType, InitialValue >::operator/=( VNumber const & rhs )
{
    this->Value = this->Value / rhs.Value;
    return *this;
}

template< typename T, typename UniqueType, UniqueType InitialValue >
inline
VNumber< T, UniqueType, InitialValue > &    VNumber< T, UniqueType, InitialValue >::operator%=( VNumber const & rhs )
{
    this->Value = this->Value % rhs.Value;
    return *this;
}

template< typename T, typename UniqueType, UniqueType InitialValue >
inline
VNumber< T, UniqueType, InitialValue > VNumber< T, UniqueType, InitialValue >::operator+( VNumber const & rhs ) const
{
    return VNumber( this->Value + rhs.Value );
}

template< typename T, typename UniqueType, UniqueType InitialValue >
inline
VNumber< T, UniqueType, InitialValue > VNumber< T, UniqueType, InitialValue >::operator-( VNumber const & rhs ) const
{
    return VNumber( this->Value - rhs.Value );
}

template< typename T, typename UniqueType, UniqueType InitialValue >
inline
VNumber< T, UniqueType, InitialValue > VNumber< T, UniqueType, InitialValue >::operator*( VNumber const & rhs ) const
{
    return VNumber( this->Value * rhs.Value );
}

template< typename T, typename UniqueType, UniqueType InitialValue >
inline
VNumber< T, UniqueType, InitialValue > VNumber< T, UniqueType, InitialValue >::operator/( VNumber const & rhs ) const
{
    return VNumber( this->Value / rhs.Value );
}

template< typename T, typename UniqueType, UniqueType InitialValue >
inline
VNumber< T, UniqueType, InitialValue > VNumber< T, UniqueType, InitialValue >::operator%( VNumber const & rhs ) const
{
    return VNumber( this->Value % rhs.Value );
}

template< typename T, typename UniqueType, UniqueType InitialValue >
inline
T VNumber< T, UniqueType, InitialValue >::Get() const
{
    return this->Value;
}

template< typename T, typename UniqueType, UniqueType InitialValue >
inline
void VNumber< T, UniqueType, InitialValue >::Set( T const value )
{
    this->Value = value;
}

NV_NAMESPACE_END

