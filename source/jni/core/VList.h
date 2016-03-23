/*
 * VList.h
 *
 *  Created on: 2016年3月2日
 *      Author: yangkai
 */

#pragma once

#include <list>

#include "vglobal.h"

NV_NAMESPACE_BEGIN

//When uses VList, the type should inherit NodeOfList. For example,class X:public NodeOfVList<VList<X>>
template <typename Container> class NodeOfVList
{
public:
    Container *pointToVList;
};

template <class E>
class VList : public std::list<E>
{
    typedef std::list<E> ParentType;

public:
    bool isEmpty() const
    {
        return ParentType::empty();
    }

    ~VList<E>()
    {
        for (auto e:(*this)) {
            delete e;
        }
    }
    const E &first() const
    {
        return ParentType::front();
    }

    const E &last() const
    {
        return ParentType::back();
    }

    E &first()
    {
        return ParentType::front();
    }

    E &last()
    {
        return ParentType::back();
    }

    void append(E &&element) { ParentType::push_back(std::move(element)); }

    void append(const E &element) { ParentType::push_back(element); }

    void append(const VList<E> &elements)
    {
        for (const E &e : elements) {
            append(e);
        }
    }

    void prepend(E &&element) { ParentType::push_front(std::move(element)); }

    void prepend(const E &element) { ParentType::push_front(element); }

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
        for (const E &i : *this) {
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
