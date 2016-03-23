
/*
 * VArray.h
 *
 *  Created on: 2016/3/6
 *      Author: gaojialing
 */

#pragma once

#include "vglobal.h"

#include <vector>
#include <utility>

NV_NAMESPACE_BEGIN

template <class E>
class VArray : public std::vector<E>
{
    typedef std::vector<E> ParentType;

public:
    typedef typename ParentType::iterator Iterator;
    typedef typename ParentType::const_iterator ConstIterator;
    typedef E ValueType;

    VArray() {}

    int length() const { return (int) ParentType::size(); }
    uint size() const { return ParentType::size(); }

    uint allocBack()
    {
        uint size_temp=this->size();
        this->resize(size_temp+1);
        return size_temp;
    }

    bool isEmpty() const { return ParentType::empty(); }

    VArray &operator << (const E &e)
    {
        append(e);
        return *this;
    }

    VArray &operator << (E &&e)
    {
        append(e);
        return *this;
    }

    VArray &operator << (const VArray<E> &elements)
    {
        append(elements);
        return *this;
    }

    const E &first() const { return ParentType::front(); }
    E &first() { return ParentType::front(); }

    const E &last() const { return ParentType::back(); }
    E &last() { return ParentType::back(); }

    E &operator[](int i) { return ParentType::at(i); }

    const E &at(uint i) const { return ParentType::at(i); }
    const E &operator[](int i) const { return ParentType::at(i); }

    void append(const E &e) { ParentType::push_back(e); }
    void append(E &&e) { ParentType::push_back(e); }

    void append(const VArray<E> &elements)
    {
        for(const E &e : elements) {
            append(e);
        }
    }

    void prepend(const E &e) { ParentType::insert(ParentType::begin(), e); }
    void prepend(E &&e) { ParentType::insert(ParentType::begin(), e); }

    void prepend(const VArray<E> &elements)
    {
        for (auto i = elements.rbegin(); i != elements.rend(); i++) {
            ParentType::insert(ParentType::begin(), *i);
        }
    }

    void removeFirst() { ParentType::erase(ParentType::begin()); }
    void removeLast() { ParentType::pop_back(); }
    void removeAt(int i) { ParentType::erase(ParentType::begin() + i); }

    void removeOne(const E &e)
    {
        for (auto i = ParentType::begin(); i != ParentType::end(); i++) {
            if (*i == e) {
                this->erase(i);
                break;
            }
        }
    }

    void removeAll(const E &e)
    {
        for (uint i = 0, max = size(); i < max; i++) {
            if (at(i) == e) {
                removeAt(i);
                i--;
                max--;
            }
        }
    }

    bool contains(const E &e) const
    {
        for(const E &i : *this) {
            if (i == e) {
                return true;
            }
        }
        return false;
    }

    void removeAtUnordered(uint index)
    {
        if (index >= this->size()) {
            return;
        }

        if (this->size() == 1) {
            this->clear();
            return;
        }
        std::swap((*this)[index], this->back());
        this->pop_back();
    }
};

NV_NAMESPACE_END

