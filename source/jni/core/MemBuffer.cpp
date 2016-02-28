/************************************************************************************

Filename    :   MemBuffer.cpp
Content     :	Memory buffer.
Created     :	May 13, 2014
Authors     :   John Carmack

Copyright   :   Copyright 2014 Oculus VR, LLC. All Rights reserved.

*************************************************************************************/

#include "MemBuffer.h"

#include <stdio.h>
#include <stdlib.h>

#include "Log.h"

namespace NervGear
{

bool MemBuffer::writeToFile( const char * filename )
{
	LogText( "Writing %i bytes to %s", length, filename );
	FILE * f = fopen( filename, "wb" );
	if ( f != NULL )
	{
		fwrite( buffer, length, 1, f );
		fclose( f );
		return true;
	}
	else
	{
		LogText( "MemBuffer::WriteToFile failed to write to %s", filename );
	}
	return false;
}

MemBuffer::MemBuffer( int length ) : buffer( malloc( length ) ), length( length )
{
}

MemBuffer::~MemBuffer()
{
}

void MemBuffer::freeData()
{
	if ( buffer != NULL )
	{
		free( (void *)buffer );
		buffer = NULL;
		length = 0;
	}
}

MemBufferFile::MemBufferFile( const char * filename )
{
	loadFile( filename );
}

bool MemBufferFile::loadFile( const char * filename )
{
	freeData();

	FILE * f = fopen( filename, "rb" );
	if ( !f )
	{
		LogText( "Couldn't open %s", filename );
		buffer = NULL;
		length = 0;
		return false;
	}
	fseek( f, 0, SEEK_END );
	length = ftell( f );
	fseek( f, 0, SEEK_SET );
	buffer = malloc( length );
	const int readRet = fread( (unsigned char *)buffer, 1, length, f );
	fclose( f );
	if ( readRet != length )
	{
		LogText( "Only read %i of %i bytes in %s", readRet, length, filename );
		buffer = NULL;
		length = 0;
		return false;
	}
	return true;
}

MemBufferFile::MemBufferFile()
{
}

MemBuffer MemBufferFile::toMemBuffer()
{
	MemBuffer	mb( buffer, length );
	buffer = NULL;
	length = 0;
	return mb;
}

MemBufferFile::~MemBufferFile()
{
	freeData();
}

}	// namespace NervGear
