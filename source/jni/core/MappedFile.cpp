/************************************************************************************

Filename    :   OVR_MappedFile.cpp
Content     :   Cross-platform memory-mapped file wrapper.
Created     :   May 12, 2014
Authors     :   Chris Taylor

Copyright   :   Copyright 2014 Oculus VR, LLC. All Rights reserved.


*************************************************************************************/

#include "MappedFile.h"

#if defined( OVR_OS_LINUX ) || defined( OVR_OS_MAC ) || defined( OVR_OS_ANDROID ) || defined( OVR_OS_IPHONE )
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#endif

#if defined( OVR_OS_WIN32)
// Already included windows.h in header
#elif defined( OVR_OS_LINUX )
#include <unistd.h>
#elif defined( OVR_OS_MAC ) || defined( OVR_OS_IPHONE )
#include <sys/sysctl.h>
#include <unistd.h>
#endif

using namespace NervGear;

static UInt32 DEFAULT_ALLOCATION_GRANULARITY = 65536;

static UInt32 GetAllocationGranularity()
{
	UInt32 alloc_gran = 0;

#if defined( OVR_OS_WIN32 )

	SYSTEM_INFO sys_info;
	GetSystemInfo( &sys_info );
	alloc_gran = sys_info.dwAllocationGranularity;

#elif defined( OVR_OS_MAC ) || defined( OVR_OS_IPHONE )

	alloc_gran = (UInt32)getpagesize();

#elif defined( _SC_PAGE_SIZE )

	alloc_gran = (UInt32)sysconf( _SC_PAGE_SIZE );

#endif

	return ( alloc_gran > 0 ) ? alloc_gran : DEFAULT_ALLOCATION_GRANULARITY;
}

/*
	MappedFile
*/

MappedFile::MappedFile()
{
	m_length = 0;

#if defined( OVR_OS_WIN32 )

	File = INVALID_HANDLE_VALUE;

#else

	m_file = -1;

#endif
}

MappedFile::~MappedFile()
{
	close();
}

bool MappedFile::openRead( const char * path, bool read_ahead, bool no_cache )
{
	close();

	m_readOnly = true;

#if defined( OVR_OS_WIN32 )

	UNREFERENCED_PARAMETER( no_cache );

	UInt32 access_pattern = !read_ahead ? FILE_FLAG_RANDOM_ACCESS : FILE_FLAG_SEQUENTIAL_SCAN;

	File = CreateFileA( path, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, access_pattern, 0 );
	if ( File == INVALID_HANDLE_VALUE )
	{
		return false;
	}

	LARGE_INTEGER qlen;
	if ( !GetFileSizeEx( File, &qlen ) )
	{
		return false;
	}

	Length = (UPInt)qlen.QuadPart;

#else

	// Don't allow private files to be read by other applications.
	m_file = open( path, O_RDONLY, (mode_t)0440 );

	if ( m_file == -1 )
	{
		return false;
	}
	else
	{
		m_length = lseek( m_file, 0, SEEK_END );

		if ( m_length <= 0 )
		{
			return false;
		}
		else
		{
#if defined( F_RDAHEAD )
			if ( read_ahead )
			{
				fcntl( File, F_RDAHEAD, 1 );
			}
#endif

#if defined( F_NOCACHE )
			if ( no_cache )
			{
				fcntl( File, F_NOCACHE, 1 );
			}
#endif
		}
	}

#endif

	return true;
}

bool MappedFile::openWrite( const char * path, uint size )
{
	close();

	m_readOnly = false;
	m_length = size;

#if defined( OVR_OS_WIN32 )

	const UInt32 access_pattern = FILE_FLAG_SEQUENTIAL_SCAN;

	File = CreateFileA( path, GENERIC_WRITE|GENERIC_READ, FILE_SHARE_WRITE, 0, CREATE_ALWAYS, access_pattern, 0 );
	if ( File == INVALID_HANDLE_VALUE )
	{
		return false;
	}

	// Set file size
	LARGE_INTEGER qlen;
	qlen.QuadPart = size;
	if ( !SetFilePointerEx( File, qlen, 0, FILE_BEGIN ) )
	{
		return false;
	}
	if ( !SetEndOfFile( File ) )
	{
		return false;
	}

	// Write a zero to the last byte to flush the file with zeros now
	qlen.QuadPart = size - 1;
	if ( !SetFilePointerEx( File, qlen, NULL, FILE_BEGIN ) )
	{
		return false;
	}
	DWORD written = 1;
	if ( !WriteFile( File, "", 1, &written, NULL ) )
	{
		return false;
	}

#else

	// Don't allow private files to be read or written by
	// other applications.
	m_file = open( path, O_RDWR|O_CREAT|O_TRUNC, (mode_t)0660 );

	if ( m_file == -1 )
	{
		return false;
	}
	else
	{
		if ( -1 == lseek( m_file, size - 1, SEEK_SET ) )
		{
			return false;
		}
		else
		{
			if ( 1 != write( m_file, "", 1 ) )
			{
				return false;
			}
		}
	}

#endif

	return true;
}

void MappedFile::close()
{
#if defined( OVR_OS_WIN32 )

	if ( File != INVALID_HANDLE_VALUE )
	{
		CloseHandle( File );
		File = INVALID_HANDLE_VALUE;
	}

#else

	if ( m_file != -1 )
	{
        ::close( m_file );
		m_file = -1;
	}

#endif

	m_length = 0;
}

/*
	MappedView
*/

MappedView::MappedView()
{
	m_data = 0;
	m_length = 0;
	m_offset = 0;

#if defined( OVR_OS_WIN32 )

	Map = 0;

#else

	m_map = MAP_FAILED;

#endif
}

MappedView::~MappedView()
{
	close();
}

bool MappedView::open( MappedFile * file )
{
	close();

	if ( !file || !file->isValid() )
	{
		return false;
	}

	m_file = file;

#if defined( OVR_OS_WIN32 )

	const UInt32 flags = file->IsReadOnly() ? PAGE_READONLY : PAGE_READWRITE;
	Map = CreateFileMapping( file->File, 0, flags, 0, 0, 0 );
	if ( !Map )
	{
		return false;
	}

#endif

	return true;
}

UByte * MappedView::mapView( uint offset, UInt32 length )
{
	if ( length == 0)
	{
		length = static_cast<UInt32>( m_file->length() );
	}

	if ( offset )
	{
		UInt32 granularity = GetAllocationGranularity();

		// Bring offset back to the previous allocation granularity
		UInt32 mask = granularity - 1;
		UInt32 masked = (UInt32)offset & mask;
		if ( masked )
		{
			offset -= masked;
			length += masked;
		}
	}

#if defined( OVR_OS_WIN32 )

	UInt32 flags = FILE_MAP_READ;
	if ( !File->IsReadOnly() )
	{
		flags |= FILE_MAP_WRITE;
	}

	Data = (UByte*)MapViewOfFile( Map, flags,
#if defined( OVR_64BIT_POINTERS )
					(UInt32)( offset >> 32 ),
#else
					0,
#endif
					(UInt32)offset, length );
	if ( !Data )
	{
		return 0;
	}

#else

	int prot = PROT_READ;
	if ( !m_file->m_readOnly )
	{
		prot |= PROT_WRITE;
	}

	// Use MAP_PRIVATE so that memory is not exposed to other processes.
	m_map = mmap( 0, length, prot, MAP_PRIVATE, m_file->m_file, offset );

	if ( m_map == MAP_FAILED )
	{
		return 0;
	}

	m_data = reinterpret_cast<UByte*>( m_map );

#endif

	m_offset = offset;
	m_length = length;

	return m_data;
}

void MappedView::close()
{
#if defined( OVR_OS_WIN32 )

	if ( Data )
	{
		UnmapViewOfFile( Data );
		Data = 0;
	}
	if ( Map )
	{
		CloseHandle( Map );
		Map = 0;
	}

#else

	if ( m_map != MAP_FAILED )
	{
		munmap( m_map, m_length );
		m_map = MAP_FAILED;
	}
	m_data = 0;

#endif

	m_length = 0;
	m_offset = 0;
}
