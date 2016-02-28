/************************************************************************************

Filename    :   OVR_BinaryFile.cpp
Content     :   Simple helper class to read a binary file.
Created     :   Jun, 2014
Authors     :   J.M.P. van Waveren

Copyright   :   Copyright 2014 Oculus VR, LLC. All Rights reserved.


*************************************************************************************/

#include "BinaryFile.h"
#include "SysFile.h"

namespace NervGear
{

BinaryReader::~BinaryReader()
{
	if ( m_allocated )
	{
		OVR_FREE( const_cast< UByte* >( m_data ) );
	}
}

BinaryReader::BinaryReader( const char * path, const char ** perror ) :
	m_data( NULL ),
	m_size( 0 ),
	m_offset( 0 ),
	m_allocated( true )
{
	SysFile f;
	if ( !f.open( path, File::Open_Read, File::ReadOnly ) )
	{
		if ( perror != NULL )
		{
			*perror = "Failed to open file";
		}
		return;
	}

	m_size = f.length();
	m_data = (UByte*) OVR_ALLOC( m_size + 1 );
	int bytes = f.read( (UByte *)m_data, m_size );
	if ( bytes != m_size && perror != NULL )
	{
		*perror = "Failed to read file";
	}
	f.close();
}

} // namespace NervGear
