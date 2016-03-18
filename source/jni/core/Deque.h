#pragma once

#include "vglobal.h"

#include "Allocator.h"

NV_NAMESPACE_BEGIN

template <class Elem>
class Deque
{
public:
    enum
    {
        DefaultCapacity = 500
    };

    Deque(int capacity = DefaultCapacity);
    Deque(const Deque<Elem> &OtherDeque);
    virtual ~Deque(void);

    virtual void        append  	(const Elem &Item);    // Adds Item to the end
    virtual void        prepend 	(const Elem &Item);    // Adds Item to the beginning
    virtual Elem        takeLast   	(void);                // Removes Item from the end
    virtual Elem        takeFirst  	(void);                // Removes Item from the beginning
    virtual const Elem& peekBack  	(int count = 0) const; // Returns count-th Item from the end
    virtual const Elem& peekFront 	(int count = 0) const; // Returns count-th Item from the beginning
    virtual inline int  size   	(void) const;          // Returns Number of Elements
	virtual inline int	capacity	(void)          const; // Returns the maximum possible number of elements
    virtual void        clear     	(void);				   // Remove all elements
    virtual inline bool isEmpty   	()     const;
    virtual inline bool isFull    	()     const;

protected:
    Elem        *m_data;          // The actual Data array
    const int   m_capacity;       // Deque capacity
    int         m_beginning;      // Index of the first element
    int         m_end;            // Index of the next after last element

    // Instead of calculating the number of elements, using this variable
    // is much more convenient.
    int         m_elemCount;

private:
    Deque&      operator= (const Deque& q) { return *this; } // forbidden
};

// Same as Deque, but allows to write more elements than maximum capacity
// Old elements are lost as they are overwritten with the new ones
template <class E>
class CircularBuffer : public Deque<E>
{
public:
    CircularBuffer(int MaxSize = Deque<E>::DefaultCapacity) : Deque<E>(MaxSize) { }
    virtual ~CircularBuffer(){}

    // The following methods are inline as a workaround for a VS bug causing erroneous C4505 warnings
    // See: http://stackoverflow.com/questions/3051992/compiler-warning-at-c-template-base-class
    inline virtual void append  (const E &Item);    // Adds Item to the end, overwriting the oldest element at the beginning if necessary
    inline virtual void prepend (const E &Item);    // Adds Item to the beginning, overwriting the oldest element at the end if necessary
};

//----------------------------------------------------------------------------------

// Deque Constructor function
template <class E>
Deque<E>::Deque(int capacity) :
m_capacity( capacity )
{
    m_data = (E*) OVR_ALLOC(m_capacity * sizeof(E));
    ConstructArray<E>(m_data, m_capacity);
    clear();
}

// Deque Copy Constructor function
template <class Elem>
Deque<Elem>::Deque(const Deque &OtherDeque) :
m_capacity( OtherDeque.m_capacity )  // Initialize the constant
{
    m_beginning = OtherDeque.m_beginning;
    m_end       = OtherDeque.m_end;
    m_elemCount = OtherDeque.m_elemCount;

    m_data      = (Elem*) OVR_ALLOC(m_capacity * sizeof(Elem));
    for (int i = 0; i < m_capacity; i++)
        m_data[i] = OtherDeque.m_data[i];
}

// Deque Destructor function
template <class Elem>
Deque<Elem>::~Deque(void)
{
    DestructArray<Elem>(m_data, m_capacity);
    OVR_FREE(m_data);
}

template <class Elem>
void Deque<Elem>::clear()
{
    m_beginning = 0;
    m_end       = 0;
    m_elemCount = 0;
}

// Push functions
template <class Elem>
void Deque<Elem>::append(const Elem &Item)
{
    // Error Check: Make sure we aren't
    // exceeding our maximum storage space
    OVR_ASSERT( m_elemCount < m_capacity );

    m_data[ m_end++ ] = Item;
    ++m_elemCount;

    // Check for wrap-around
    if (m_end >= m_capacity)
        m_end -= m_capacity;
}

template <class Elem>
void Deque<Elem>::prepend(const Elem &Item)
{
    // Error Check: Make sure we aren't
    // exceeding our maximum storage space
    OVR_ASSERT( m_elemCount < m_capacity );

    m_beginning--;
    // Check for wrap-around
    if (m_beginning < 0)
        m_beginning += m_capacity;

    m_data[ m_beginning ] = Item;
    ++m_elemCount;
}

// Pop functions
template <class Elem>
Elem Deque<Elem>::takeFirst(void)
{
    // Error Check: Make sure we aren't reading from an empty Deque
    OVR_ASSERT( m_elemCount > 0 );

    Elem ReturnValue = m_data[ m_beginning++ ];
    --m_elemCount;

    // Check for wrap-around
    if (m_beginning >= m_capacity)
        m_beginning -= m_capacity;

    return ReturnValue;
}

template <class Elem>
Elem Deque<Elem>::takeLast(void)
{
    // Error Check: Make sure we aren't reading from an empty Deque
    OVR_ASSERT( m_elemCount > 0 );

    m_end--;
    // Check for wrap-around
    if (m_end < 0)
        m_end += m_capacity;

    Elem ReturnValue = m_data[ m_end ];
    --m_elemCount;

    return ReturnValue;
}

// Peek functions
template <class Elem>
const Elem& Deque<Elem>::peekFront(int count) const
{
    // Error Check: Make sure we aren't reading from an empty Deque
    OVR_ASSERT( m_elemCount > count );

    int idx = m_beginning + count;
    if (idx >= m_capacity)
        idx -= m_capacity;
    return m_data[ idx ];
}

template <class Elem>
const Elem& Deque<Elem>::peekBack(int count) const
{
    // Error Check: Make sure we aren't reading from an empty Deque
    OVR_ASSERT( m_elemCount > count );

    int idx = m_end - count - 1;
    if (idx < 0)
        idx += m_capacity;
    return m_data[ idx ];
}

template <class Elem>
inline int Deque<Elem>::capacity(void) const
{
    return Deque<Elem>::m_capacity;
}

// ElemNum() function
template <class Elem>
inline int Deque<Elem>::size(void) const
{
    return m_elemCount;
}

template <class Elem>
inline bool Deque<Elem>::isEmpty(void) const
{
    return m_elemCount==0;
}

template <class Elem>
inline bool Deque<Elem>::isFull(void) const
{
    return m_elemCount==m_capacity;
}

// ******* CircularBuffer<Elem> *******
// Push functions
template <class Elem>
void CircularBuffer<Elem>::append(const Elem &Item)
{
    if (this->isFull())
        this->takeFirst();
    Deque<Elem>::append(Item);
}

template <class Elem>
void CircularBuffer<Elem>::prepend(const Elem &Item)
{
    if (this->isFull())
        this->takeLast();
    Deque<Elem>::prepend(Item);
}

NV_NAMESPACE_END
