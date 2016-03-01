#pragma once

#include "vglobal.h"
#include "VString.h"

#include <map>

NV_NAMESPACE_BEGIN

template<class T>
class StringHash : public std::map<VString, T>
{
public:
    typedef T ValueType;
    typedef std::map<VString, T> SelfType;
    typedef typename std::map<VString, T>::iterator Iterator;
    typedef typename std::map<VString, T>::const_iterator ConstIterator;

    bool isEmpty() const { return this->empty(); }

    void insert(const VString &key, const T &value) { (*this)[key] = value; }

    void remove(const VString &key) { this->erase(key); }
};

NV_NAMESPACE_END
