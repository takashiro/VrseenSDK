#pragma once

#include "vglobal.h"

#include <deque>

NV_NAMESPACE_BEGIN

template <class E>
class VDeque : public std::deque<E>
{
    typedef std::deque<E> ParentType;
public:

    void append(const E &element) { ParentType::push_back(element); }
    void append(E &&element) { ParentType::push_back(std::move(element)); }

    void prepend(const E &element) { ParentType::push_front(element); }
    void prepend(E &&element) { ParentType::push_front(std::move(element)); }

    void removeFirst() { ParentType::pop_front(); }

    E &&takeFirst()
    {
        E element = std::move(ParentType::front());
        ParentType::pop_front();
        return std::move(element);
    }

    void removeLast() { ParentType::pop_back(); }

    E &&takeLast()
    {
        E element = std::move(ParentType::back());
        ParentType::pop_back();
        return std::move(element);
    }

    const E &peekFirst(uint i = 0) const { return ParentType::operator [](i); }
    const E &peekLast(uint i = 0) const { return ParentType::operator [](size() - i); }

    uint size() const { return ParentType::size(); }
    int length() const { return (int) ParentType::size(); }

    bool isEmpty() const { return ParentType::empty(); }
};

NV_NAMESPACE_END
