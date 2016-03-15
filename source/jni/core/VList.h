/*
 * VList.h
 *
 *  Created on: 2016年3月2日
 *      Author: yangkai
 */

#pragma once
#include <list>
#include "vglobal.h"

using namespace std;

NV_NAMESPACE_BEGIN

//使用VList的类型应该继承该模板；比如 class X:public NodeOfVList<VList<X>>
template <typename Container> class NodeOfVList
{
public:
    Container *pointToVList;
};

template <class E> class VList : public list<E>
{
public:
    bool isEmpty() const
    {
        return this->empty();
    }

    const E &first() const
    {
        return this->front();
    }

    const E &last() const
    {
        return this->back();
    }

    E &first()
    {
        return this->front();
    }

    E &last()
    {
        return this->back();
    }

    void append(const E &element)
    {
        this->push_back(element);
    }

    void append(const VList<E> &elements)
    {
        for (E e : elements) {
            this->push_back(e);
        }
    }

    void prepend(const E &element)
    {
        this->push_front(element);
    }

    void prepend(const VList<E> &elements)
    {
        for (auto reverseIterator = elements.rbegin();
                reverseIterator != elements.rend();
                reverseIterator++) {
            this->push_front(*reverseIterator);
        }
    }

    void bringToFront(typename VList<E>::iterator thisIterator)
    {
        E temp = *thisIterator;
        this->erase(thisIterator);
        this->push_front(temp);
    }

    void sendToBack(typename VList<E>::iterator thisIterator)
    {
        E temp = *thisIterator;
        this->erase(thisIterator);
        this->push_back(temp);
    }

    void removeAll(const E &element)
    {
        this->remove(element);
    }

    bool contains(const E &element) const
    {
        for (E i:*this) {
            if (i == element) {
                return true;
            }
        }
        return false;
    }

    const E getNextByContent(E& content) const
    {
        bool flag = false;
        for(E i:*this) {
            if (flag) {
                return i;
            }
            if (i == content) {
                flag = true;
            }
        }
        return nullptr;
    }
};
NV_NAMESPACE_END
