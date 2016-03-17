#pragma once

#include "vglobal.h"

#include "VMath.h"
#include "VString.h"
#include "VArray.h"

NV_NAMESPACE_BEGIN

namespace StringUtils
{

	// Convert a string to a common type.
	//

	template< typename _type_ > inline size_t StringTo( _type_ & value, const char * string ) { return 0; }

	template< typename _type_ > inline size_t StringTo( _type_ * valueArray, const int count, const char * string )
	{
		size_t length = 0;
		length += strspn( string + length, "{ \t\n\r" );
		for ( int i = 0; i < count; i++ )
		{
			length += StringTo< _type_ >( valueArray[i], string + length );
		}
		length += strspn( string + length, "} \t\n\r" );
		return length;
	}

	template< typename _type_ > inline size_t StringTo( VArray< _type_ > & valueArray, const char * string )
	{
		size_t length = 0;
		length += strspn( string + length, "{ \t\n\r" );
		for ( ; ; )
		{
			_type_ value;
			size_t s = StringTo< _type_ >( value, string + length );
			if ( s == 0 ) break;
			valueArray.append( value );
			length += s;
		}
		length += strspn( string + length, "} \t\n\r" );
		return length;
	}

	// specializations

	template<> inline size_t StringTo( short &          value, const char * str ) { char * endptr; value = (short) strtol( str, &endptr, 10 ); return endptr - str; }
	template<> inline size_t StringTo( unsigned short & value, const char * str ) { char * endptr; value = (unsigned short) strtoul( str, &endptr, 10 ); return endptr - str; }
	template<> inline size_t StringTo( int &            value, const char * str ) { char * endptr; value = strtol( str, &endptr, 10 ); return endptr - str; }
	template<> inline size_t StringTo( unsigned int &   value, const char * str ) { char * endptr; value = strtoul( str, &endptr, 10 ); return endptr - str; }
	template<> inline size_t StringTo( float &          value, const char * str ) { char * endptr; value = strtof( str, &endptr ); return endptr - str; }
	template<> inline size_t StringTo( double &         value, const char * str ) { char * endptr; value = strtod( str, &endptr ); return endptr - str; }

	template<> inline size_t StringTo( Vector2f & value, const char * string ) { return StringTo( &value.x, 2, string ); }
	template<> inline size_t StringTo( Vector2d & value, const char * string ) { return StringTo( &value.x, 2, string ); }
	template<> inline size_t StringTo( Vector2i & value, const char * string ) { return StringTo( &value.x, 2, string ); }

	template<> inline size_t StringTo( Vector3f & value, const char * string ) { return StringTo( &value.x, 3, string ); }
	template<> inline size_t StringTo( Vector3d & value, const char * string ) { return StringTo( &value.x, 3, string ); }
	template<> inline size_t StringTo( Vector3i & value, const char * string ) { return StringTo( &value.x, 3, string ); }

	template<> inline size_t StringTo( Vector4f & value, const char * string ) { return StringTo( &value.x, 4, string ); }
	template<> inline size_t StringTo( Vector4d & value, const char * string ) { return StringTo( &value.x, 4, string ); }
	template<> inline size_t StringTo( Vector4i & value, const char * string ) { return StringTo( &value.x, 4, string ); }

	template<> inline size_t StringTo( Matrix4f & value, const char * string ) { return StringTo( &value.M[0][0], 16, string ); }
	template<> inline size_t StringTo( Matrix4d & value, const char * string ) { return StringTo( &value.M[0][0], 16, string ); }

	template<> inline size_t StringTo( Quatf &    value, const char * string ) { return StringTo( &value.x, 4, string ); }
	template<> inline size_t StringTo( Quatd &    value, const char * string ) { return StringTo( &value.x, 4, string ); }

	template<> inline size_t StringTo( Planef &   value, const char * string ) { return StringTo( &value.N.x, 4, string ); }
	template<> inline size_t StringTo( Planed &   value, const char * string ) { return StringTo( &value.N.x, 4, string ); }

	template<> inline size_t StringTo( Bounds3f & value, const char * string ) { return StringTo( value.b, 2, string ); }
	template<> inline size_t StringTo( Bounds3d & value, const char * string ) { return StringTo( value.b, 2, string ); }

} // StringUtils

NV_NAMESPACE_END
