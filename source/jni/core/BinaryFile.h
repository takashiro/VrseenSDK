#pragma once

#include "vglobal.h"

#include "Types.h"
#include "Array.h"

/*
	This is a simple helper class to read binary data next to a JSON file.
*/

NV_NAMESPACE_BEGIN

class BinaryReader
{
public:
	BinaryReader( const UByte * binData, const int binSize ) :
        m_data( binData ),
        m_size( binSize ),
        m_offset( 0 ),
        m_allocated( false ) {}
	~BinaryReader();

	BinaryReader( const char * path, const char ** perror );

    UInt32 readUInt32() const
	{
		const int bytes = sizeof( unsigned int );
        if ( m_data == NULL || bytes > m_size - m_offset )
		{
			return 0;
		}
        m_offset += bytes;
        return *(UInt32 *)( m_data + m_offset - bytes );
	}

	template< typename _type_ >
    bool readArray( Array< _type_ > & out, const int numElements ) const
	{
		const int bytes = numElements * sizeof( out[0] );
        if ( m_data == NULL || bytes > m_size - m_offset )
		{
			out.resize( 0 );
			return false;
		}
		out.resize( numElements );
        memcpy( &out[0], &m_data[m_offset], bytes );
        m_offset += bytes;
		return true;
	}

    bool atEnd() const
	{
        return ( m_offset == m_size );
	}

private:
    const UByte *	m_data;
    SInt32			m_size;
    mutable SInt32	m_offset;
    bool			m_allocated;
};

NV_NAMESPACE_END


