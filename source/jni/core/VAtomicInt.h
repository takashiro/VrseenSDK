/*
 * VAtomicInt.h
 *
 *  Created on: 2016年3月17日
 *      Author: yangkai
 */
#pragma once
#include <atomic>
#include "vglobal.h"
NV_NAMESPACE_BEGIN
class VAtomicInt : public atomic<int>
{
    typedef int Type;
public:
    VAtomicInt();

    explicit VAtomicInt(Type value);

    Type exchangeAddSync(Type value);

    Type exchangeAddRelease(Type value);

    Type exchangeAddAcquire(Type value);

    Type exchangeAddNoSync(Type value);

    void incrementSync();

    void incrementRelease();

    void incrementAcquire();

    void incrementNoSync();

    Type operator *= (Type argument);

    Type operator /= (Type argument);

    Type operator >>= (unsigned bits);

    Type operator <<= (unsigned bits);
};
NV_NAMESPACE_END
