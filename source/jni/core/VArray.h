
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
public:
    typedef typename VArray<E>::iterator Iterator;
    typedef typename VArray<E>::const_iterator ConstIterator;
    typedef E ValueType;

    VArray() {}

    int length() const { return (int) this->size(); }

    uint allocBack()
    {
        uint size_temp=this->size();
        this->resize(size_temp+1);
        return size_temp;
    }

    bool isEmpty() const { return this->empty(); }

    VArray &operator << (const E &e)
    {
        append(e);
        return *this;
    }

    VArray &operator << (const VArray<E> &elements)
    {
        append(elements);
        return *this;
    }

    const E &first() const { return this->front(); }

    E &first() { return this->front(); }

    const E &last() const { return this->back(); }

    E &last() { return this->back(); }

    E &operator[](int i) { return this->at(i); }

    const E &operator[](int i) const { return this->at(i); }

    void append(const E &e) { this->push_back(e); }
    void append(E &&e) { this->push_back(e); }

    void append(const VArray<E> &elements)
    {
        for(const E &e : elements)
        {
            this->push_back(e);
        }
    }

    void prepend(const E &e) { this->insert(this->begin(), e); }
    void prepend(E &&e) { this->insert(this->begin(), e); }

    void prepend(const VArray<E> &elements)
    {
        for (auto i = elements.rbegin(); i != elements.rend(); i++) {
            this->insert(this->begin(), *i);
        }
    }

    void removeFirst() { this->erase(this->begin()); }
    void removeLast() { this->pop_back(); }
    void removeAt(int i) { this->erase(this->begin() + i); }
    void removeOne(const E &e)
    {
        for (auto i = this->begin(); i != this->end(); i++) {
            if (*i == e) {
                this->erase(i);
                break;
            }
        }
    }

    void removeAll(const E &e)
    {
        for (uint i = 0, max = this->size(); i < max; i++) {
            if (this->at(i) == e) {
                this->removeAt(i);
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

