#pragma once

#include "vglobal.h"
#include <Deque>

NV_NAMESPACE_BEGIN

//Begin Tempalate VDque
template <class E>
class VDeque : public std::deque<E>
{
public :
    VDeque(int capacity = 500);
    VDeque(const VDeque<E> &oneVDque);
    virtual ~VDeque(void);

    virtual void        VDpush_front      (const E &element);
    virtual void        VDpush_back       (const E &element);
    virtual E           VDpop_front       ();
    virtual E           VDpop_back        ();
    virtual const   E&  VDpeek_front       (int i = 0) const;
    virtual const   E&  VDpeek_back        (int i = 0) const;
    virtual int         VDsize            () const;
    virtual int         VDcapacity        () const;
    virtual void        VDclear           ();
    virtual inline bool VDisEmpty         ()const;
    virtual inline bool VDisFull          ()const;


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
void VDeque<E>::VDpush_front(const E &element)
{
    this->push_front(element);
    return;
}

template <class E>
void VDeque<E>::VDpush_back(const E &element)
{
    this->push_back(element);
    return;
}

template <class E>
E VDeque<E>::VDpop_front()
{
    E tmp = this->front();
    this->pop_front();
    return tmp;
}

template <class E>
E VDeque<E>::VDpop_back()
{
    E tmp = this->back();
    this->pop_back();
    return tmp;
}

template <class E>
const E& VDeque<E>::VDpeek_front(int i) const
{
    if (i >= this->v_capacity)
    {
        i -= this->v_capacity;
    }
    return this->at(i);
}

template <class E>
const E& VDeque<E>::VDpeek_back(int i) const
{
    int tmp = this->v_capacity-1-i;
    if (tmp < 0)
    {
        tmp += this->v_capacity;
    }
    return this->at(tmp);
}

template <class E>
int VDeque<E>::VDsize() const
{
    return this->v_capacity;
}

template <class E>
int VDeque<E>::VDcapacity() const
{
    return this->max_size();
}

template <class E>
void VDeque<E>::VDclear()
{
    this->clear();
}

template <class E>
inline bool VDeque<E>::VDisEmpty()const
{
    return this->size()==0;
}

template <class E>
inline bool VDeque<E>::VDisFull() const
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

    inline virtual void VCpush_back  (const E &Item);    // Adds Item to the end, overwriting the oldest element at the beginning if necessary
    inline virtual void VCpush_front (const E &Item);    // Adds Item to the beginning, overwriting the oldest element at the end if necessary
};

template <class E>
inline void VCircularBuffer<E>::VCpush_back(const E &Item)
{
    if (this->VDisFull())
    {
     this->VDpop_back();
    }
    this->push_back(Item);
}

template <class E>
inline void VCircularBuffer<E>::VCpush_front(const E &Item)
{
    if (this->VDisFull())
    {
        this->VDpop_front();
    }
    this->push_front(Item);
}
//End Template VCircularBuffer

NV_NAMESPACE_END
