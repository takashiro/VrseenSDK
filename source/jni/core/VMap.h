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
    typedef std::map<Key, Value> ParentType;

    typedef typename ParentType::iterator Iterator;
    typedef typename ParentType::const_iterator ConstIterator;

    bool isEmpty() const { return ParentType::empty(); }

    bool contains(const Key &key) const { return ParentType::find(key) != ParentType::end(); }

    const Value &value(const Key &key) const
    {
        return contains(key) ? ParentType::at(key) : defaultValue;
    }

    Value &operator[](const Key &key) { return ParentType::operator[](key); }
    const Value &operator[](const Key &key) const { return value(key); }

    void insert(const Key &key, const Value &value) { ParentType::insert(std::pair<Key, Value>(key, value)); }
    void insert(const Key &key, Value &&value) { ParentType::insert(std::pair<Key, Value>(key, std::move(value))); }

    void remove(const Key &key) { ParentType::erase(key); }

private:
    Value defaultValue;
};

NV_NAMESPACE_END
