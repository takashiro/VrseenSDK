/*
 * VAtomicInt.h
 *
 *  Created on: 2016骞�鏈�7鏃�
 *      Author: yangkai
 */
#pragma once
#include <atomic>
#include "vglobal.h"
using namespace std;
NV_NAMESPACE_BEGIN
class VAtomicInt : public atomic<int>
{
public:
    typedef int Type;
    inline VAtomicInt() : atomic<Type>()
    {

    }
    explicit inline VAtomicInt(Type value) : atomic<Type>(value)
    {

    }
    inline VAtomicInt(const VAtomicInt &src) : atomic<Type>()
    {
        store(src.load());
    }
    inline Type exchangeAddSync(Type value)
    {
        return fetch_add(value);
    }
    inline Type exchangeAddRelease(Type value)
    {
        return fetch_add(value);
    }
    inline Type exchangeAddAcquire(Type value)
    {
        return fetch_add(value);
    }
    inline Type exchangeAddNoSync(Type value)
    {
        return fetch_add(value);
    }
    inline void incrementSync()
    {
        (*this)++;
    }

    inline void incrementRelease()
    {
        (*this)++;
    }

    inline void incrementAcquire()
    {
        (*this)++;
    }

    inline void incrementNoSync()
    {
        (*this)++;
    }

    Type operator *= (Type argument);

    Type operator /= (Type argument);

    Type operator >>= (unsigned bits);

    Type operator <<= (unsigned bits);

    VAtomicInt operator = (Type argument);

    VAtomicInt operator & (Type argument);
};
NV_NAMESPACE_END
