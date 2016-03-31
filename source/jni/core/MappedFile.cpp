#include "MappedFile.h"

#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>

#include <unistd.h>

NV_NAMESPACE_BEGIN

static uint DEFAULT_ALLOCATION_GRANULARITY = 65536;

static uint GetAllocationGranularity()
{
    uint alloc_gran = 0;
#if defined( _SC_PAGE_SIZE )
    alloc_gran = (uint)sysconf( _SC_PAGE_SIZE );
#endif
	return ( alloc_gran > 0 ) ? alloc_gran : DEFAULT_ALLOCATION_GRANULARITY;
}

MappedFile::MappedFile()
{
	m_length = 0;
	m_file = -1;
}

MappedFile::~MappedFile()
{
	close();
}

bool MappedFile::openRead( const char * path, bool read_ahead, bool no_cache )
{
	close();

	m_readOnly = true;

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
    }

	return true;
}

bool MappedFile::openWrite( const char * path, uint size )
{
	close();

	m_readOnly = false;
    m_length = size;

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

	return true;
}

void MappedFile::close()
{
	if ( m_file != -1 )
	{
        ::close( m_file );
		m_file = -1;
	}

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
    m_map = MAP_FAILED;
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

	return true;
}

uchar *MappedView::mapView(uint offset, uint length )
{
	if ( length == 0)
	{
        length = static_cast<uint>( m_file->length() );
	}

	if ( offset )
	{
        uint granularity = GetAllocationGranularity();

		// Bring offset back to the previous allocation granularity
        uint mask = granularity - 1;
        uint masked = (uint)offset & mask;
		if ( masked )
		{
			offset -= masked;
			length += masked;
		}
	}
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

    m_data = reinterpret_cast<uchar *>( m_map );

	m_offset = offset;
	m_length = length;

	return m_data;
}

void MappedView::close()
{
	if ( m_map != MAP_FAILED )
	{
		munmap( m_map, m_length );
		m_map = MAP_FAILED;
	}
	m_data = 0;

	m_length = 0;
	m_offset = 0;
}

NV_NAMESPACE_END
