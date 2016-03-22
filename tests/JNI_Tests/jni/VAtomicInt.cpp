/*
 * VAtomicInt.cpp
 *
 *  Created on: 2016年3月18日
 *      Author: yangkai
 */
#include "VAtomicInt.h"
NV_NAMESPACE_BEGIN
inline VAtomicInt::VAtomicInt() : atomic<Type>()
{

}
inline VAtomicInt::VAtomicInt(Type value) : atomic<Type>(value)
{

}
inline VAtomicInt::Type VAtomicInt::exchangeAddSync(Type value)
{
    return fetch_add(value);
}
inline VAtomicInt::Type VAtomicInt::exchangeAddRelease(Type value)
{
    return fetch_add(value);
}
inline VAtomicInt::Type VAtomicInt::exchangeAddAcquire(Type value)
{
    return fetch_add(value);
}
inline VAtomicInt::Type VAtomicInt::exchangeAddNoSync(Type value)
{
    return fetch_add(value);
}
inline void VAtomicInt::incrementSync()
{
    (*this)++;
}

inline void VAtomicInt::incrementRelease()
{
    (*this)++;
}

inline void VAtomicInt::incrementAcquire()
{
    (*this)++;
}

inline void VAtomicInt::incrementNoSync()
{
    (*this)++;
}

VAtomicInt::Type VAtomicInt::operator *= (VAtomicInt::Type argument)
{
    VAtomicInt::Type newValue, oldValue = load();
    do
    {
        newValue = oldValue * argument;
    }while(compare_exchange_weak(oldValue, newValue));
    return newValue;
}

VAtomicInt::Type VAtomicInt::operator /= (VAtomicInt::Type argument)
{
    VAtomicInt::Type newValue, oldValue = load();
    do
    {
        newValue = oldValue / argument;
    }while(compare_exchange_weak(oldValue, newValue));
    return newValue;
}

VAtomicInt::Type VAtomicInt::operator >>= (unsigned bits)
{
    VAtomicInt::Type newValue, oldValue = load();
    do
    {
        newValue = oldValue >> bits;
    }while(compare_exchange_weak(oldValue, newValue));
    return newValue;
}

VAtomicInt::Type VAtomicInt::operator <<= (unsigned bits)
{
    VAtomicInt::Type newValue, oldValue = load();
    do
    {
        newValue = oldValue << bits;
    }while(compare_exchange_weak(oldValue, newValue));
    return newValue;
}
NV_NAMESPACE_END



