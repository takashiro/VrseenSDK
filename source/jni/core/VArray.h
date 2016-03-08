#pragma once

#include "vglobal.h"

#include <vector>

NV_NAMESPACE_BEGIN

template<class E>
class VArray : public std::vector<E>
{
public:
    typedef typename std::vector<E>::iterator Iterator;
    typedef typename std::vector<E>::const_iterator ConstIterator;

    int length() const { return (int) this->size(); }
    bool isEmpty() const { return this->empty(); }

    const E &first() const { return this->front(); }
    E &first() { return this->front(); }

    const E &last() const { return this->back(); }
    E &last() { return this->last(); }

    void append(const E &e) { this->push_back(e); }
    VArray &operator << (const E &e) { this->append(e); return *this; }

    void append(const VArray<E> &elements)
    {
        for (const E &e : elements) {
            append(e);
        }
    }
    VArray &operator << (const VArray<E> &elements) { append(elements); return *this; }

    void prepend(const E &e) { insert(0, e); }

    void prepend(const VArray<E> &elements)
    {
        for (uint i = 0, max = elements.size(); i < max; i++) {
            insert(i, elements.at(i));
        }
    }

    void insert(uint i, const E &e) { std::vector<E>::insert(this->cbegin() + i, e); }

    void removeAt(uint i) { this->erase(this->begin() + i); }

    void removeOne(const E &e)
    {
        for (uint i = 0, max = this->size(); i < max; i++) {
            if (this->at(i) == e) {
                removeAt(i);
                break;
            }
        }
    }

    void removeAll(const E &e)
    {
        for (uint i = 0, max = this->size(); i < max; i++) {
            if (this->at(i) == e) {
                removeAt(i);
                i--;
                max--;
            }
        }
    }

    bool contains(const E &e)
    {
        for (uint i = 0, max = this->size(); i < max; i++) {
            if (this->at(i) == e) {
                return true;
            }
        }
        return false;
    }
};

NV_NAMESPACE_END
