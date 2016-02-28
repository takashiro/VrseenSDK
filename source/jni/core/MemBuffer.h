/************************************************************************************

Filename    :   MemBuffer.h
Content     :	Memory buffer.
Created     :	May 13, 2014
Authors     :   John Carmack

Copyright   :   Copyright 2014 Oculus VR, LLC. All Rights reserved.


*************************************************************************************/
#ifndef MEMBUFFER_H
#define MEMBUFFER_H

namespace NervGear {


// This does NOT free the memory on delete, it is just a wrapper around
// memory, and can be copied freely for passing / returning as a value.
// This does NOT have a virtual destructor, so if a copy is made of
// a derived version, it won't cause it to be freed.
class MemBuffer
{
public:
    MemBuffer() : buffer( 0 ), length( 0 ) {}
    MemBuffer( const void * buffer, int length ) : buffer( buffer), length( length ) {}
	explicit MemBuffer( int length );
	~MemBuffer();

	// Calls Free() on Buffer and sets it to NULL and lengrh to 0
    void freeData();

    bool writeToFile( const char * filename );

    const void *	buffer;
    int		length;
};

// This DOES free the memory on delete.
class MemBufferFile : public MemBuffer
{
public:
	explicit MemBufferFile( const char * filename );
    explicit MemBufferFile();	// use this constructor as MemBufferFile( NoInit ) -- this takes a parameters so you still can't defalt-construct by accident
	virtual ~MemBufferFile();

    bool loadFile( const char * filename );

	// Moves the data to a new MemBuffer that won't
	// be deleted on destruction, removing it from the
	// MemBufferFile.
    MemBuffer toMemBuffer();
};

//==============================================================
// MemBufferT
//
// This allocates memory on construction and frees the memory on delete.
// On copy assignment or copy construct any existing pointer in the destination
// is freed, the pointer from the source is assigned to the destination and
// the pointer in the source is cleared.
template< class C >
class MemBufferT
{
public:
	// allocates a buffer of the specified size.
	explicit MemBufferT( unsigned long long const size )
        : m_buffer( 0 )
        , m_size( size )
	{
        m_buffer = new C[size];
	}

	// explicit copy construtor. This is explicit so these objects don't accidentally
	// get passed by value.
	explicit MemBufferT( MemBufferT & other )
        : m_buffer( other.m_buffer )
        , m_size( other.m_size )
	{
        other.m_buffer = 0;
        other.m_size = 0;
	}

	// frees the buffer on deconstruction
	~MemBufferT()
	{
        free();
	}

	// returns a const pointer to the buffer
    operator C const * () const { return m_buffer; }
	
	// returns a non-const pointer to the buffer
    operator C * () { return m_buffer; }

    unsigned long long	size() const { return m_size; }

	// assignment operator
	MemBufferT & operator = ( MemBufferT & other )
	{
		if ( &other == this )
		{
			return *this;
		}

        free();	// free existing data before copying

        m_buffer = other.m_buffer;
        m_size = other.m_size;
        other.m_buffer = 0;
        other.m_size = 0;
	}

	// frees any existing buffer and allocates a new buffer of the specified size
    void realloc( unsigned long long size )
	{
        free();

        m_buffer = new C[size];
        m_size = size;
	}

private:
    C *					m_buffer;
    unsigned long long	m_size;

private:
    MemBufferT() : m_buffer( 0 ), m_size( 0 ) { }

	// frees the existing buffer
    void free()
	{
        delete m_buffer;
        m_buffer = 0;
        m_size = 0;
	}
};

}	// namespace NervGear

#endif // MEMBUFFER_H
