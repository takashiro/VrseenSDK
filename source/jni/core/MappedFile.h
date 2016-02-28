/************************************************************************************

Filename    :   OVR_MappedFile.h
Content     :   Cross-platform memory-mapped file wrapper.
Created     :   May 12, 2014
Authors     :   Chris Taylor

Copyright   :   Copyright 2014 Oculus VR, LLC. All Rights reserved.


*************************************************************************************/

#ifndef OVR_MappedFile_h
#define OVR_MappedFile_h

#include "Types.h"

#ifdef OVR_OS_WIN32
#include <windows.h>
#endif

/*
	Memory-mapped files are a fairly good compromise between performance and flexibility.

	Compared with asynchronous io, memory-mapped files are:
		+ Much easier to implement in a portable way
		+ Automatically paged in and out of RAM
		+ Automatically read-ahead cached

	When asynch IO is not available or blocking is acceptable then this is a
	great alternative with low overhead and similar performance.

	For random file access, use MappedView with a MappedFile that has been
	opened with random_access = true.  Random access is usually used for a
	database-like file type, which is much better implemented using asynch IO.
*/

namespace NervGear
{

// Read-only memory mapped file
class MappedFile
{
	friend class MappedView;

public:
					MappedFile();
					~MappedFile();

	// Opens the file for shared read-only access with other applications
	// Returns false on error (file not found, etc)
    bool			openRead( const char * path, bool read_ahead = false, bool no_cache = false );

	// Creates and opens the file for exclusive read/write access
    bool			openWrite( const char * path, UPInt size );

    void			close();

    bool			isReadOnly() const { return m_readOnly; }
    UPInt			length() const { return m_length; }
    bool			isValid() const { return ( m_length != 0 ); }

private:
#if defined( OVR_OS_WIN32 )
    HANDLE			m_file;
#else
    int				m_file;
#endif

    bool			m_readOnly;
    UPInt			m_length;
};


// View of a portion of the memory mapped file
class MappedView
{
public:
					MappedView();
					~MappedView();

    bool			open( MappedFile * file ); // Returns false on error
    UByte *			mapView( UPInt offset = 0, UInt32 length = 0 ); // Returns 0 on error, 0 length means whole file
    void			close();

    bool			isValid() const { return ( m_data != 0 ); }
    UPInt			offset() const { return m_offset; }
    UInt32			length() const { return m_length; }
    MappedFile *	file() { return m_file; }
    UByte *			front() { return m_data; }

private:
#if defined( OVR_OS_WIN32 )
    HANDLE			m_map;
#else
    void *			m_map;
#endif

    MappedFile *	m_file;
    UByte *			m_data;
    UPInt			m_offset;
    UInt32			m_length;
};

} // namespace NervGear

#endif // OVR_MappedFile_h
