#pragma once

#include "vglobal.h"
#include <Deque>

NV_NAMESPACE_BEGIN

//Begin Tempalate VDque
template <class E>
class VDeque : public std::deque<E>
{
    typedef std::deque<E> ParentType;
public :    
    VDeque(int capacity = 500);
    VDeque(const VDeque<E> &oneVDque);
    virtual ~VDeque(void);

    virtual void        append          (const E &element);
    virtual void        preAppend       (const E &element);
    virtual E           takeFirst       ();
    virtual E           takeLast        ();
    virtual const   E&  peekFirst       (int i = 0) const;
    virtual const   E&  peekLast        (int i = 0) const;
    virtual int         size            () const;
    virtual int         capacity        () const;
    virtual void        clear           ();
    virtual inline bool isEmpty         ()const;
    virtual inline bool isFull          ()const;


protected:
    E           *v_data;        //Pointer of Deque
    const int   v_capacity;     //The capacity of Deque
    int         v_start;        //The index of first element
    int         v_end;          //The index of last element
    int         v_count;        //Sum of the Deque's element

private:
    VDeque&      operator= (const VDeque& ins) { return *this; }

};

template <class E>
VDeque<E>::VDeque(int capacity ):std::deque<E>::deque(capacity),v_capacity(capacity)
{

}


template <class E>
VDeque<E>::VDeque(const VDeque<E> &oneVDeque):std::deque<E>::deque(oneVDeque)
{

}

template <class E>
VDeque<E>::~VDeque()
{

}

template <class E>
void VDeque<E>::append(const E &element)
{
    this->push_front(element);
    return;
}

template <class E>
void VDeque<E>::preAppend(const E &element)
{
    this->push_back(element);
    return;
}

template <class E>
E VDeque<E>::takeFirst()
{
    E tmp = this->front();
    this->pop_front();
    return tmp;
}

template <class E>
E VDeque<E>::takeLast()
{
    E tmp = this->back();
    this->pop_back();
    return tmp;
}

template <class E>
const E& VDeque<E>::peekFirst(int i) const
{
    if (i >= this->v_capacity)
    {
        i -= this->v_capacity;
    }
    return this->at(i);
}

template <class E>
const E& VDeque<E>::peekLast(int i) const
{
    int tmp = this->v_capacity-1-i;
    if (tmp < 0)
    {
        tmp += this->v_capacity;
    }
    return this->at(tmp);
}

template <class E>
int VDeque<E>::size() const
{
    return this->v_capacity;
}

template <class E>
int VDeque<E>::capacity() const
{
    return this->max_size();
}

template <class E>
void VDeque<E>::clear()
{
    ParentType::clear();
}

template <class E>
inline bool VDeque<E>::isEmpty()const
{
    return this->size()==0;
}

template <class E>
inline bool VDeque<E>::isFull() const
{
    return this->size()==this->v_capacity;
}





//End Template VDque

//Begin Template VCircularBuffer
template <class E>
class VCircularBuffer : public VDeque<E>
{
public:
    VCircularBuffer(int MaxSize = 500) : VDeque<E>::VDeque(MaxSize) { }
    virtual ~VCircularBuffer(){}

    inline virtual void preAppend  (const E &Item);    // Adds Item to the end, overwriting the oldest element at the beginning if necessary
    inline virtual void append (const E &Item);    // Adds Item to the beginning, overwriting the oldest element at the end if necessary
};

template <class E>
inline void VCircularBuffer<E>::preAppend(const E &Item)
{
    if (this->isFull())
    {
     this->takeLast();
    }
    this->push_back(Item);
}

template <class E>
inline void VCircularBuffer<E>::append(const E &Item)
{
    if (this->isFull())
    {
        this->takeFirst();
    }
    this->push_front(Item);
}
//End Template VCircularBuffer

NV_NAMESPACE_END
