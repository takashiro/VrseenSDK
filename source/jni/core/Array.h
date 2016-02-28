/************************************************************************************

PublicHeader:   OVR.h
Filename    :   OVR_Array.h
Content     :   Template implementation for Array
Created     :   September 19, 2012
Notes       :

Copyright   :   Copyright 2014 Oculus VR, LLC. All Rights reserved.

************************************************************************************/

#ifndef OVR_Array_h
#define OVR_Array_h

#include "ContainerAllocator.h"

namespace NervGear {

//-----------------------------------------------------------------------------------
// ***** ArrayDefaultPolicy
//
// Default resize behavior. No minimal capacity, Granularity=4,
// Shrinking as needed. ArrayConstPolicy actually is the same as
// ArrayDefaultPolicy, but parametrized with constants.
// This struct is used only in order to reduce the template "matroska".
struct ArrayDefaultPolicy
{
    ArrayDefaultPolicy() : m_capacity(0) {}
    ArrayDefaultPolicy(const ArrayDefaultPolicy&) : m_capacity(0) {}

    UPInt minCapacity() const { return 0; }
    UPInt granularity() const { return 4; }
    bool  neverShrinking() const { return 0; }

    UPInt capacity()    const      { return m_capacity; }
    void  setCapacity(UPInt capacity) { m_capacity = capacity; }
private:
    UPInt m_capacity;
};


//-----------------------------------------------------------------------------------
// ***** ArrayConstPolicy
//
// Statically parametrized resizing behavior:
// MinCapacity, Granularity, and Shrinking flag.
template<int MinCapacity=0, int Granularity=4, bool NeverShrink=false>
struct ArrayConstPolicy
{
    typedef ArrayConstPolicy<MinCapacity, Granularity, NeverShrink> SelfType;

    ArrayConstPolicy() : m_capacity(0) {}
    ArrayConstPolicy(const SelfType&) : m_capacity(0) {}

    UPInt minCapacity() const { return MinCapacity; }
    UPInt granularity() const { return Granularity; }
    bool  neverShrinking() const { return NeverShrink; }

    UPInt capacity()    const      { return m_capacity; }
    void  setCapacity(UPInt capacity) { m_capacity = capacity; }
private:
    UPInt m_capacity;
};

//-----------------------------------------------------------------------------------
// ***** ArrayDataBase
//
// Basic operations with array data: Reserve, Resize, Free, ArrayPolicy.
// For internal use only: ArrayData,ArrayDataCC and others.
template<class T, class Allocator, class SizePolicy>
struct ArrayDataBase
{
    typedef T                                           ValueType;
    typedef Allocator                                   AllocatorType;
    typedef SizePolicy                                  SizePolicyType;
    typedef ArrayDataBase<T, Allocator, SizePolicy>     SelfType;

    ArrayDataBase()
        : data(0), size(0), policy() {}

    ArrayDataBase(const SizePolicy& p)
        : data(0), size(0), policy(p) {}

    ~ArrayDataBase()
    {
        if (data)
        {
            Allocator::DestructArray(data, size);
            Allocator::Free(data);
        }
    }

    UPInt capacity() const
    {
        return policy.capacity();
    }

    void clearAndRelease()
    {
        if (data)
        {
            Allocator::DestructArray(data, size);
            Allocator::Free(data);
            data = 0;
        }
        size = 0;
        policy.setCapacity(0);
    }

    void reserve(UPInt newCapacity)
    {
        if (policy.neverShrinking() && newCapacity < capacity())
            return;

        if (newCapacity < policy.minCapacity())
            newCapacity = policy.minCapacity();

        // Resize the buffer.
        if (newCapacity == 0)
        {
            if (data)
            {
                Allocator::Free(data);
                data = 0;
            }
            policy.setCapacity(0);
        }
        else
        {
            UPInt gran = policy.granularity();
            newCapacity = (newCapacity + gran - 1) / gran * gran;
            if (data)
            {
                if (Allocator::IsMovable())
                {
                    data = (T*)Allocator::Realloc(data, sizeof(T) * newCapacity);
                }
                else
                {
                    T* newData = (T*)Allocator::Alloc(sizeof(T) * newCapacity);
                    UPInt i, s;
                    s = (size < newCapacity) ? size : newCapacity;
                    for (i = 0; i < s; ++i)
                    {
                        Allocator::Construct(&newData[i], data[i]);
                        Allocator::Destruct(&data[i]);
                    }
                    for (i = s; i < size; ++i)
                    {
                        Allocator::Destruct(&data[i]);
                    }
                    Allocator::Free(data);
                    data = newData;
                }
            }
            else
            {
                data = (T*)Allocator::Alloc(sizeof(T) * newCapacity);
                //memset(Buffer, 0, (sizeof(ValueType) * newSize)); // Do we need this?
            }
            policy.setCapacity(newCapacity);
            // OVR_ASSERT(Data); // need to throw (or something) on alloc failure!
        }
    }

    // This version of Resize DOES NOT construct the elements.
    // It's done to optimize PushBack, which uses a copy constructor
    // instead of the default constructor and assignment
    void ResizeNoConstruct(UPInt newSize)
    {
        UPInt oldSize = size;

        if (newSize < oldSize)
        {
            Allocator::DestructArray(data + newSize, oldSize - newSize);
            if (newSize < (policy.capacity() >> 1))
            {
                reserve(newSize);
            }
        }
        else if(newSize >= policy.capacity())
        {
            reserve(newSize + (newSize >> 2));
        }
        //! IMPORTANT to modify Size only after Reserve completes, because garbage collectable
        // array may use this array and may traverse it during Reserve (in the case, if
        // collection occurs because of heap limit exceeded).
        size = newSize;
    }

    ValueType*  data;
    UPInt       size;
    SizePolicy  policy;
};



//-----------------------------------------------------------------------------------
// ***** ArrayData
//
// General purpose array data.
// For internal use only in Array, ArrayLH, ArrayPOD and so on.
template<class T, class Allocator, class SizePolicy>
struct ArrayData : ArrayDataBase<T, Allocator, SizePolicy>
{
    typedef T ValueType;
    typedef Allocator                                   AllocatorType;
    typedef SizePolicy                                  SizePolicyType;
    typedef ArrayDataBase<T, Allocator, SizePolicy>     BaseType;
    typedef ArrayData    <T, Allocator, SizePolicy>     SelfType;

    ArrayData()
        : BaseType() { }

    ArrayData(int size)
        : BaseType() { resize(size); }

    ArrayData(const SelfType& a)
        : BaseType(a.policy) { append(a.data, a.size); }


    void resize(UPInt newSize)
    {
        UPInt oldSize = this->size;
        BaseType::ResizeNoConstruct(newSize);
        if(newSize > oldSize)
            Allocator::ConstructArray(this->data + oldSize, newSize - oldSize);
    }

    void append(const ValueType& val)
    {
        BaseType::ResizeNoConstruct(this->size + 1);
        Allocator::Construct(this->data + this->size - 1, val);
    }

    template<class S>
    void appendAlt(const S& val)
    {
        BaseType::ResizeNoConstruct(this->size + 1);
        Allocator::ConstructAlt(this->data + this->size - 1, val);
    }

    // Append the given data to the array.
    void append(const ValueType other[], UPInt count)
    {
        if (count)
        {
            UPInt oldSize = this->size;
            BaseType::ResizeNoConstruct(this->size + count);
            Allocator::ConstructArray(this->data + oldSize, count, other);
        }
    }
};



//-----------------------------------------------------------------------------------
// ***** ArrayDataCC
//
// A modification of ArrayData that always copy-constructs new elements
// using a specified DefaultValue. For internal use only in ArrayCC.
template<class T, class Allocator, class SizePolicy>
struct ArrayDataCC : ArrayDataBase<T, Allocator, SizePolicy>
{
    typedef T                                           ValueType;
    typedef Allocator                                   AllocatorType;
    typedef SizePolicy                                  SizePolicyType;
    typedef ArrayDataBase<T, Allocator, SizePolicy>     BaseType;
    typedef ArrayDataCC  <T, Allocator, SizePolicy>     SelfType;

    ArrayDataCC(const ValueType& defval)
        : BaseType(), defaultValue(defval) { }

    ArrayDataCC(const ValueType& defval, int size)
        : BaseType(), defaultValue(defval) { Resize(size); }

    ArrayDataCC(const SelfType& a)
        : BaseType(a.policy), defaultValue(a.defaultValue) { append(a.data, a.size); }


    void Resize(UPInt newSize)
    {
        UPInt oldSize = this->size;
        BaseType::ResizeNoConstruct(newSize);
        if(newSize > oldSize)
            Allocator::ConstructArray(this->data + oldSize, newSize - oldSize, defaultValue);
    }

    void append(const ValueType& val)
    {
        BaseType::ResizeNoConstruct(this->size + 1);
        Allocator::Construct(this->data + this->size - 1, val);
    }

    template<class S>
    void appendAlt(const S& val)
    {
        BaseType::ResizeNoConstruct(this->size + 1);
        Allocator::ConstructAlt(this->data + this->size - 1, val);
    }

    // Append the given data to the array.
    void append(const ValueType other[], UPInt count)
    {
        if (count)
        {
            UPInt oldSize = this->size;
            BaseType::ResizeNoConstruct(this->size + count);
            Allocator::ConstructArray(this->data + oldSize, count, other);
        }
    }

    ValueType   defaultValue;
};





//-----------------------------------------------------------------------------------
// ***** ArrayBase
//
// Resizable array. The behavior can be POD (suffix _POD) and
// Movable (no suffix) depending on the allocator policy.
// In case of _POD the constructors and destructors are not called.
//
// Arrays can't handle non-movable objects! Don't put anything in here
// that can't be moved around by bitwise copy.
//
// The addresses of elements are not persistent! Don't keep the address
// of an element; the array contents will move around as it gets resized.
template<class ArrayData>
class ArrayBase
{
public:
    typedef typename ArrayData::ValueType       ValueType;
    typedef typename ArrayData::AllocatorType   AllocatorType;
    typedef typename ArrayData::SizePolicyType  SizePolicyType;
    typedef ArrayBase<ArrayData>                SelfType;


#undef new
    OVR_MEMORY_REDEFINE_NEW(ArrayBase)
// Redefine operator 'new' if necessary.
#if defined(OVR_DEFINE_NEW)
#define new OVR_DEFINE_NEW
#endif


    ArrayBase()
        : m_data() {}
    ArrayBase(int size)
        : m_data(size) {}
    ArrayBase(const SelfType& a)
        : m_data(a.m_data) {}

    ArrayBase(const ValueType& defval)
        : m_data(defval) {}
    ArrayBase(const ValueType& defval, int size)
        : m_data(defval, size) {}

    SizePolicyType* sizePolicy() const                  { return m_data.policy; }
    void            setSizePolicy(const SizePolicyType& p) { m_data.policy = p; }

    bool    neverShrinking()const       { return m_data.policy.neverShrinking(); }
    UPInt   size()       const       { return m_data.size;  }
    // For those that prefer to avoid the hazards of working with unsigned values.
    // Note that on most platforms this will limit the capacity to 2 gig elements.
    int  	sizeInt()      const       { return (int)m_data.size;  }
    bool    isEmpty()       const       { return m_data.size == 0; }
    UPInt   capacity()   const       { return m_data.capacity(); }
    int		capacityInt()	const		{ return (int)m_data.capacity(); }
    UPInt   numBytes()   const       { return m_data.capacity() * sizeof(ValueType); }

    void    clearAndRelease()           { m_data.clearAndRelease(); }
    void    clear()                     { m_data.resize(0); }
    void    resize(UPInt newSize)       { m_data.resize(newSize); }

    // Reserve can only increase the capacity
    void    reserve(UPInt newCapacity)
    {
        if (newCapacity > m_data.capacity())
            m_data.reserve(newCapacity);
    }

    // Basic access.
    ValueType& at(UPInt index)
    {
        OVR_ASSERT(index < m_data.size);
        return m_data.data[index];
    }
    const ValueType& at(UPInt index) const
    {
        OVR_ASSERT(index < m_data.size);
        return m_data.data[index];
    }

    ValueType value(UPInt index) const
    {
        OVR_ASSERT(index < m_data.size);
        return m_data.data[index];
    }

    // Basic access.
    ValueType& operator [] (UPInt index)
    {
        OVR_ASSERT(index < m_data.size);
        return m_data.data[index];
    }
    const ValueType& operator [] (UPInt index) const
    {
        OVR_ASSERT(index < m_data.size);
        return m_data.data[index];
    }

    // Raw pointer to the data. Use with caution!
    const ValueType* dataPtr() const { return m_data.data; }
          ValueType* dataPtr()       { return m_data.data; }

    // Insert the given element at the end of the array.
    void    pushBack(const ValueType& val)
    {
        // DO NOT pass elements of your own vector into
        // push_back()!  Since we're using references,
        // resize() may munge the element storage!
        // OVR_ASSERT(&val < &Buffer[0] || &val > &Buffer[BufferSize]);
        m_data.append(val);
    }

    template<class S>
    void	pushBackAlt(const S& val)
    {
        m_data.appendAlt(val);
    }

	// This is rather dangerous because a second call to PushDefault() may
	// invalidate the reference returned by a previous call to PushDefault().
	// Consider the construction of a tree with the nodes stored in an array,
	// in which case it is tempting to call PushDefault() multiple times for
	// the children of a node.
	// Instead, use AllocBack() which default initializes a new element at
	// the end of the array and returns the index to the element.
    ValueType& pushDefault()
    {
        m_data.append(ValueType());
        return back();
    }

	// Default initializes a new element at the end of the array and returns
	// the index to the element.
    UPInt	allocBack()
	{
        UPInt size = m_data.size;
        m_data.resize(size + 1);
		return size;
	}

    // Remove the last element.
    void    popBack(UPInt count = 1)
    {
        OVR_ASSERT(m_data.size >= count);
        m_data.resize(m_data.size - count);
    }

	// Remove and return the last element.
    ValueType pop()
    {
        ValueType t = back();
        popBack();
        return t;
    }


    // Access the first element.
    ValueType&          front()         { return at(0); }
    const ValueType&    front() const   { return at(0); }

    // Access the last element.
    ValueType&          back()          { return at(m_data.size - 1); }
    const ValueType&    back() const    { return at(m_data.size - 1); }

    // Array copy.  Copies the contents of a into this array.
    const SelfType& operator = (const SelfType& a)
    {
        resize(a.size());
        for (UPInt i = 0; i < m_data.size; i++) {
            *(m_data.data + i) = a[i];
        }
        return *this;
    }

    // Removing multiple elements from the array.
    void    remove(UPInt index, UPInt num)
    {
        OVR_ASSERT(index + num <= m_data.size);
        if (m_data.size == num)
        {
            clear();
        }
        else
        {
            AllocatorType::DestructArray(m_data.data + index, num);
            AllocatorType::CopyArrayForward(
                m_data.data + index,
                m_data.data + index + num,
                m_data.size - num - index);
            m_data.size -= num;
        }
    }

    // Removing an element from the array is an expensive operation!
    // It compacts only after removing the last element.
    // If order of elements in the array is not important then use
    // RemoveAtUnordered, that could be much faster than the regular
    // RemoveAt.
    void    removeAt(UPInt index)
    {
        OVR_ASSERT(index < m_data.size);
        if (m_data.size == 1)
        {
            clear();
        }
        else
        {
            AllocatorType::Destruct(m_data.data + index);
            AllocatorType::CopyArrayForward(
                m_data.data + index,
                m_data.data + index + 1,
                m_data.size - 1 - index);
            --m_data.size;
        }
    }

    // Removes an element from the array without respecting of original order of
    // elements for better performance. Do not use on array where order of elements
    // is important, otherwise use it instead of regular RemoveAt().
    void    removeAtUnordered(UPInt index)
    {
        OVR_ASSERT(index < m_data.size);
        if (m_data.size == 1)
        {
            clear();
        }
        else
        {
            // copy the last element into the 'index' position
            // and decrement the size (instead of moving all elements
            // in [index + 1 .. size - 1] range).
            const UPInt lastElemIndex = m_data.size - 1;
            if (index < lastElemIndex)
            {
                AllocatorType::Destruct(m_data.data + index);
                AllocatorType::Construct(m_data.data + index, m_data.data[lastElemIndex]);
            }
            AllocatorType::Destruct(m_data.data + lastElemIndex);
            --m_data.size;
        }
    }

    // Insert the given object at the given index shifting all the elements up.
    void    insert(UPInt index, const ValueType& val = ValueType())
    {
        OVR_ASSERT(index <= m_data.size);

        m_data.resize(m_data.size + 1);
        if (index < m_data.size - 1)
        {
            AllocatorType::CopyArrayBackward(
                m_data.data + index + 1,
                m_data.data + index,
                m_data.size - 1 - index);
        }
        AllocatorType::Construct(m_data.data + index, val);
    }

    // Insert the given object at the given index shifting all the elements up.
    void    insertMultipleAt(UPInt index, UPInt num, const ValueType& val = ValueType())
    {
        OVR_ASSERT(index <= m_data.size);

        m_data.resize(m_data.size + num);
        if (index < m_data.size - num)
        {
            AllocatorType::CopyArrayBackward(
                m_data.data + index + num,
                m_data.data + index,
                m_data.size - num - index);
        }
        for (UPInt i = 0; i < num; ++i)
            AllocatorType::Construct(m_data.data + index + i, val);
    }

    // Append the given data to the array.
    void    append(const SelfType& other)
    {
        append(other.m_data.data, other.size());
    }

    // Append the given data to the array.
    void    append(const ValueType other[], UPInt count)
    {
        m_data.append(other, count);
    }

    class Iterator
    {
        SelfType*       m_array;
        SPInt           m_curIndex;

    public:
        Iterator() : m_array(0), m_curIndex(-1) {}
        Iterator(SelfType* parr, SPInt idx = 0) : m_array(parr), m_curIndex(idx) {}

        bool operator==(const Iterator& it) const { return m_array == it.m_array && m_curIndex == it.m_curIndex; }
        bool operator!=(const Iterator& it) const { return m_array != it.m_array || m_curIndex != it.m_curIndex; }

        Iterator& operator++()
        {
            if (m_array)
            {
                if (m_curIndex < (SPInt)m_array->size())
                    ++m_curIndex;
            }
            return *this;
        }
        Iterator operator++(int)
        {
            Iterator it(*this);
            operator++();
            return it;
        }
        Iterator& operator--()
        {
            if (m_array)
            {
                if (m_curIndex >= 0)
                    --m_curIndex;
            }
            return *this;
        }
        Iterator operator--(int)
        {
            Iterator it(*this);
            operator--();
            return it;
        }
        Iterator operator+(int delta) const
        {
            return Iterator(m_array, m_curIndex + delta);
        }
        Iterator operator-(int delta) const
        {
            return Iterator(m_array, m_curIndex - delta);
        }
        SPInt operator-(const Iterator& right) const
        {
            OVR_ASSERT(m_array == right.m_array);
            return m_curIndex - right.m_curIndex;
        }
        ValueType& operator*() const    { OVR_ASSERT(m_array); return  (*m_array)[m_curIndex]; }
        ValueType* operator->() const   { OVR_ASSERT(m_array); return &(*m_array)[m_curIndex]; }
        ValueType* GetPtr() const       { OVR_ASSERT(m_array); return &(*m_array)[m_curIndex]; }

        bool isFinished() const { return !m_array || m_curIndex < 0 || m_curIndex >= (int)m_array->size(); }

        void remove()
        {
            if (!isFinished())
                m_array->removeAt(m_curIndex);
        }

        SPInt index() const { return m_curIndex; }
    };

    Iterator begin() { return Iterator(this); }
    Iterator end()   { return Iterator(this, (SPInt)size()); }
    Iterator last()  { return Iterator(this, (SPInt)size() - 1); }

    class ConstIterator
    {
        const SelfType* m_array;
        SPInt           m_curIndex;

    public:
        ConstIterator() : m_array(0), m_curIndex(-1) {}
        ConstIterator(const SelfType* parr, SPInt idx = 0) : m_array(parr), m_curIndex(idx) {}

        bool operator==(const ConstIterator& it) const { return m_array == it.m_array && m_curIndex == it.m_curIndex; }
        bool operator!=(const ConstIterator& it) const { return m_array != it.m_array || m_curIndex != it.m_curIndex; }

        ConstIterator& operator++()
        {
            if (m_array)
            {
                if (m_curIndex < (int)m_array->size())
                    ++m_curIndex;
            }
            return *this;
        }
        ConstIterator operator++(int)
        {
            ConstIterator it(*this);
            operator++();
            return it;
        }
        ConstIterator& operator--()
        {
            if (m_array)
            {
                if (m_curIndex >= 0)
                    --m_curIndex;
            }
            return *this;
        }
        ConstIterator operator--(int)
        {
            ConstIterator it(*this);
            operator--();
            return it;
        }
        ConstIterator operator+(int delta) const
        {
            return ConstIterator(m_array, m_curIndex + delta);
        }
        ConstIterator operator-(int delta) const
        {
            return ConstIterator(m_array, m_curIndex - delta);
        }
        SPInt operator-(const ConstIterator& right) const
        {
            OVR_ASSERT(m_array == right.m_array);
            return m_curIndex - right.m_curIndex;
        }
        const ValueType& operator*() const  { OVR_ASSERT(m_array); return  (*m_array)[m_curIndex]; }
        const ValueType* operator->() const { OVR_ASSERT(m_array); return &(*m_array)[m_curIndex]; }
        const ValueType* GetPtr() const     { OVR_ASSERT(m_array); return &(*m_array)[m_curIndex]; }

        bool isFinished() const { return !m_array || m_curIndex < 0 || m_curIndex >= (int)m_array->size(); }

        SPInt index()  const { return m_curIndex; }
    };
    ConstIterator begin() const { return ConstIterator(this); }
    ConstIterator end() const   { return ConstIterator(this, (SPInt)size()); }
    ConstIterator last() const  { return ConstIterator(this, (SPInt)size() - 1); }

protected:
    ArrayData   m_data;
};



//-----------------------------------------------------------------------------------
// ***** Array
//
// General purpose array for movable objects that require explicit
// construction/destruction.
template<class T, class SizePolicy=ArrayDefaultPolicy>
class Array : public ArrayBase<ArrayData<T, ContainerAllocator<T>, SizePolicy> >
{
public:
    typedef T                                                           ValueType;
    typedef ContainerAllocator<T>                                       AllocatorType;
    typedef SizePolicy                                                  SizePolicyType;
    typedef Array<T, SizePolicy>                                        SelfType;
    typedef ArrayBase<ArrayData<T, ContainerAllocator<T>, SizePolicy> > BaseType;

    Array() : BaseType() {}
    Array(int size) : BaseType(size) {}
    Array(const SizePolicyType& p) : BaseType() { setSizePolicy(p); }
    Array(const SelfType& a) : BaseType(a) {}
    const SelfType& operator=(const SelfType& a) { BaseType::operator=(a); return *this; }
};

// ***** ArrayPOD
//
// General purpose array for movable objects that DOES NOT require
// construction/destruction. Constructors and destructors are not called!
// Global heap is in use.
template<class T, class SizePolicy=ArrayDefaultPolicy>
class ArrayPOD : public ArrayBase<ArrayData<T, ContainerAllocator_POD<T>, SizePolicy> >
{
public:
    typedef T                                                               ValueType;
    typedef ContainerAllocator_POD<T>                                       AllocatorType;
    typedef SizePolicy                                                      SizePolicyType;
    typedef ArrayPOD<T, SizePolicy>                                         SelfType;
    typedef ArrayBase<ArrayData<T, ContainerAllocator_POD<T>, SizePolicy> > BaseType;

    ArrayPOD() : BaseType() {}
    ArrayPOD(int size) : BaseType(size) {}
    ArrayPOD(const SizePolicyType& p) : BaseType() { setSizePolicy(p); }
    ArrayPOD(const SelfType& a) : BaseType(a) {}
    const SelfType& operator=(const SelfType& a) { BaseType::operator=(a); return *this; }
};


// ***** ArrayCPP
//
// General purpose, fully C++ compliant array. Can be used with non-movable data.
// Global heap is in use.
template<class T, class SizePolicy=ArrayDefaultPolicy>
class ArrayCPP : public ArrayBase<ArrayData<T, ContainerAllocator_CPP<T>, SizePolicy> >
{
public:
    typedef T                                                               ValueType;
    typedef ContainerAllocator_CPP<T>                                       AllocatorType;
    typedef SizePolicy                                                      SizePolicyType;
    typedef ArrayCPP<T, SizePolicy>                                         SelfType;
    typedef ArrayBase<ArrayData<T, ContainerAllocator_CPP<T>, SizePolicy> > BaseType;

    ArrayCPP() : BaseType() {}
    ArrayCPP(int size) : BaseType(size) {}
    ArrayCPP(const SizePolicyType& p) : BaseType() { setSizePolicy(p); }
    ArrayCPP(const SelfType& a) : BaseType(a) {}
    const SelfType& operator=(const SelfType& a) { BaseType::operator=(a); return *this; }
};


// ***** ArrayCC
//
// A modification of the array that uses the given default value to
// construct the elements. The constructors and destructors are
// properly called, the objects must be movable.

template<class T, class SizePolicy=ArrayDefaultPolicy>
class ArrayCC : public ArrayBase<ArrayDataCC<T, ContainerAllocator<T>, SizePolicy> >
{
public:
    typedef T                                                               ValueType;
    typedef ContainerAllocator<T>                                           AllocatorType;
    typedef SizePolicy                                                      SizePolicyType;
    typedef ArrayCC<T, SizePolicy>                                          SelfType;
    typedef ArrayBase<ArrayDataCC<T, ContainerAllocator<T>, SizePolicy> >   BaseType;

    ArrayCC(const ValueType& defval) : BaseType(defval) {}
    ArrayCC(const ValueType& defval, int size) : BaseType(defval, size) {}
    ArrayCC(const ValueType& defval, const SizePolicyType& p) : BaseType(defval) { setSizePolicy(p); }
    ArrayCC(const SelfType& a) : BaseType(a) {}
    const SelfType& operator=(const SelfType& a) { BaseType::operator=(a); return *this; }
};

} // OVR

#endif
