#pragma once

#include "vglobal.h"
#include "Types.h"

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

NV_NAMESPACE_BEGIN

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
    bool			openWrite( const char * path, uint size );

    void			close();

    bool			isReadOnly() const { return m_readOnly; }
    uint			length() const { return m_length; }
    bool			isValid() const { return ( m_length != 0 ); }

private:
    int				m_file;

    bool			m_readOnly;
    uint			m_length;
};


// View of a portion of the memory mapped file
class MappedView
{
public:
					MappedView();
					~MappedView();

    bool			open( MappedFile * file ); // Returns false on error
    UByte *			mapView( uint offset = 0, uint length = 0 ); // Returns 0 on error, 0 length means whole file
    void			close();

    bool			isValid() const { return ( m_data != 0 ); }
    uint			offset() const { return m_offset; }
    uint			length() const { return m_length; }
    MappedFile *	file() { return m_file; }
    UByte *			front() { return m_data; }

private:
    void *			m_map;

    MappedFile *	m_file;
    UByte *			m_data;
    uint			m_offset;
    uint			m_length;
};

NV_NAMESPACE_END


