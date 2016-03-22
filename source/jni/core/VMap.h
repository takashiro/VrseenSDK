#pragma once

#include "vglobal.h"

#include <map>

NV_NAMESPACE_BEGIN

template<typename Key, typename Value>
class VMap : public std::map<Key, Value>
{
public:
    typedef Key KeyType;
    typedef Value ValueType;

    typedef typename std::map<Key, Value>::iterator Iterator;
    typedef typename std::map<Key, Value>::const_iterator ConstIterator;

    bool isEmpty() const { return std::map<Key, Value>::empty(); }

    bool contains(const Key &key) const { return std::map<Key, Value>::find(key) != std::map<Key, Value>::end(); }

    Value &value(const Key &key) { return std::map<Key, Value>::at(key); }
    Value &operator[](const Key &key) { return std::map<Key, Value>::at(key); }
    const Value &value(const Key &key) const { return std::map<Key, Value>::at(key); }
    const Value &operator[](const Key &key) const { return std::map<Key, Value>::at(key); }

    void insert(const Key &key, const Value &value) { (*this)[key] = value; }
    void insert(const Key &key, Value &&value) { std::map<Key, Value>::insert(std::pair<Key, Value>(key, value)); }

    void remove(const Key &key) { std::map<Key, Value>::erase(key); }
};

NV_NAMESPACE_END
