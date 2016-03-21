#pragma once

#include "vglobal.h"

#include "Types.h"

NV_NAMESPACE_BEGIN

//-----------------------------------------------------------------------------------
// ***** ListNode
//
// Base class for the elements of the intrusive linked list.
// To store elements in the List do:
//
// struct MyData : ListNode<MyData>
// {
//     . . .
// };
template<class T>
struct ListNode
{
    union {
        T*    pPrev;
        void* pVoidPrev;
    };
    union {
        T*    pNext;
        void* pVoidNext;
    };

    void    removeNode()
    {
        pPrev->pNext = pNext;
        pNext->pPrev = pPrev;
    }

    // Removes us from the list and inserts pnew there instead.
    void    ReplaceNodeWith(T* pnew)
    {
        pPrev->pNext = pnew;
        pNext->pPrev = pnew;
        pnew->pPrev = pPrev;
        pnew->pNext = pNext;
    }

    // Inserts the argument linked list node after us in the list.
    void    InsertNodeAfter(T* p)
    {
        p->pPrev          = pNext->pPrev; // this
        p->pNext          = pNext;
        pNext->pPrev      = p;
        pNext             = p;
    }
    // Inserts the argument linked list node before us in the list.
    void    InsertNodeBefore(T* p)
    {
        p->pNext          = pNext->pPrev; // this
        p->pPrev          = pPrev;
        pPrev->pNext      = p;
        pPrev             = p;
    }

    void    Alloc_MoveTo(ListNode<T>* pdest)
    {
        pdest->pNext = pNext;
        pdest->pPrev = pPrev;
        pPrev->pNext = (T*)pdest;
        pNext->pPrev = (T*)pdest;
    }
};


//------------------------------------------------------------------------
// ***** List
//
// Doubly linked intrusive list.
// The data type must be derived from ListNode.
//
// Adding:   PushFront(), PushBack().
// Removing: Remove() - the element must be in the list!
// Moving:   BringToFront(), SendToBack() - the element must be in the list!
//
// Iterating:
//    MyData* data = MyList.first();
//    while (!MyList.isNull(data))
//    {
//        . . .
//        data = MyList.next(data);
//    }
//
// Removing:
//    MyData* data = MyList.first();
//    while (!MyList.isNull(data))
//    {
//        MyData* next = MyList.next(data);
//        if (ToBeRemoved(data))
//             MyList.Remove(data);
//        data = next;
//    }
//

// List<> represents a doubly-linked list of T, where each T must derive
// from ListNode<B>. B specifies the base class that was directly
// derived from ListNode, and is only necessary if there is an intermediate
// inheritance chain.

template<class T, class B = T> class List
{
public:
    typedef T ValueType;

    List()
    {
        m_root.pNext = m_root.pPrev = (ValueType*)&m_root;
    }

    void clear()
    {
        m_root.pNext = m_root.pPrev = (ValueType*)&m_root;
    }

    const ValueType* first() const { return (const ValueType*)m_root.pNext; }
    const ValueType* last () const { return (const ValueType*)m_root.pPrev; }
          ValueType* first()       { return (ValueType*)m_root.pNext; }
          ValueType* last ()       { return (ValueType*)m_root.pPrev; }

    // Determine if list is empty (i.e.) points to itself.
    // Go through void* access to avoid issues with strict-aliasing optimizing out the
    // access after removeNode(), etc.
    bool isEmpty()                   const { return m_root.pVoidNext == (const T*)(const B*)&m_root; }
    bool isFirst(const ValueType* p) const { return p == m_root.pNext; }
    bool isLast (const ValueType* p) const { return p == m_root.pPrev; }
    bool isNull (const ValueType* p) const { return p == (const T*)(const B*)&m_root; }

    inline static const ValueType* getPrev(const ValueType* p) { return (const ValueType*)p->pPrev; }
    inline static const ValueType* getNext(const ValueType* p) { return (const ValueType*)p->pNext; }
    inline static       ValueType* getPrev(      ValueType* p) { return (ValueType*)p->pPrev; }
    inline static       ValueType* getNext(      ValueType* p) { return (ValueType*)p->pNext; }

    void prepend(ValueType* p)
    {
        p->pNext          =  m_root.pNext;
        p->pPrev          = (ValueType*)&m_root;
        m_root.pNext->pPrev =  p;
        m_root.pNext        =  p;
    }

    void append(ValueType* p)
    {
        p->pPrev          =  m_root.pPrev;
        p->pNext          = (ValueType*)&m_root;
        m_root.pPrev->pNext =  p;
        m_root.pPrev        =  p;
    }

    static void remove(ValueType* p)
    {
        p->pPrev->pNext = p->pNext;
        p->pNext->pPrev = p->pPrev;
    }

    void bringToFront(ValueType* p)
    {
        remove(p);
        prepend(p);
    }

    void sendToBack(ValueType* p)
    {
        remove(p);
        append(p);
    }

    // Appends the contents of the argument list to the front of this list;
    // items are removed from the argument list.
    void prepend(List<T>& src)
    {
        if (!src.isEmpty())
        {
            ValueType* pfirst = src.first();
            ValueType* plast  = src.last();
            src.clear();
            plast->pNext   = m_root.pNext;
            pfirst->pPrev  = (ValueType*)&m_root;
            m_root.pNext->pPrev = plast;
            m_root.pNext        = pfirst;
        }
    }

    void append(List<T>& src)
    {
        if (!src.isEmpty())
        {
            ValueType* pfirst = src.first();
            ValueType* plast  = src.last();
            src.clear();
            plast->pNext   = (ValueType*)&m_root;
            pfirst->pPrev  = m_root.pPrev;
            m_root.pPrev->pNext = pfirst;
            m_root.pPrev        = plast;
        }
    }

    // Removes all source list items after (and including) the 'pfirst' node from the
    // source list and adds them to out list.
    void    pushFollowingListItemsToFront(List<T>& src, ValueType *pfirst)
    {
        if (pfirst != &src.m_root)
        {
            ValueType *plast = src.m_root.pPrev;

            // Remove list remainder from source.
            pfirst->pPrev->pNext = (ValueType*)&src.m_root;
            src.m_root.pPrev      = pfirst->pPrev;
            // Add the rest of the items to list.
            plast->pNext      = m_root.pNext;
            pfirst->pPrev     = (ValueType*)&m_root;
            m_root.pNext->pPrev = plast;
            m_root.pNext        = pfirst;
        }
    }

    // Removes all source list items up to but NOT including the 'pend' node from the
    // source list and adds them to out list.
    void    pushPrecedingListItemsToFront(List<T>& src, ValueType *ptail)
    {
        if (src.first() != ptail)
        {
            ValueType *pfirst = src.m_root.pNext;
            ValueType *plast  = ptail->pPrev;

            // Remove list remainder from source.
            ptail->pPrev      = (ValueType*)&src.m_root;
            src.m_root.pNext    = ptail;

            // Add the rest of the items to list.
            plast->pNext      = m_root.pNext;
            pfirst->pPrev     = (ValueType*)&m_root;
            m_root.pNext->pPrev = plast;
            m_root.pNext        = pfirst;
        }
    }


    // Removes a range of source list items starting at 'pfirst' and up to, but not including 'pend',
    // and adds them to out list. Note that source items MUST already be in the list.
    void    pushListItemsToFront(ValueType *pfirst, ValueType *pend)
    {
        if (pfirst != pend)
        {
            ValueType *plast = pend->pPrev;

            // Remove list remainder from source.
            pfirst->pPrev->pNext = pend;
            pend->pPrev          = pfirst->pPrev;
            // Add the rest of the items to list.
            plast->pNext      = m_root.pNext;
            pfirst->pPrev     = (ValueType*)&m_root;
            m_root.pNext->pPrev = plast;
            m_root.pNext        = pfirst;
        }
    }


    void    allocMoveTo(List<T>* pdest)
    {
        if (isEmpty())
            pdest->clear();
        else
        {
            pdest->m_root.pNext = m_root.pNext;
            pdest->m_root.pPrev = m_root.pPrev;

            m_root.pNext->pPrev = (ValueType*)&pdest->m_root;
            m_root.pPrev->pNext = (ValueType*)&pdest->m_root;
        }
    }


private:
    // Copying is prohibited
    List(const List<T>&);
    const List<T>& operator = (const List<T>&);

    ListNode<B> m_root;
};


//------------------------------------------------------------------------
// ***** FreeListElements
//
// Remove all elements in the list and free them in the allocator

template<class List, class Allocator>
void FreeListElements(List& list, Allocator& allocator)
{
    typename List::ValueType* self = list.first();
    while(!list.isNull(self))
    {
        typename List::ValueType* next = list.getNext(self);
        allocator.Free(self);
        self = next;
    }
    list.clear();
}

NV_NAMESPACE_END
