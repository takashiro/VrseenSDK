#pragma once

#include "vglobal.h"

#include "VString.h"
#include "Hash.h"

NV_NAMESPACE_BEGIN

//-----------------------------------------------------------------------------------
// *** StringHash

// This is a custom string hash table that supports case-insensitive
// searches through special functions such as GetCaseInsensitive, etc.
// This class is used for Flash labels, exports and other case-insensitive tables.

template<class U, class Allocator = ContainerAllocator<U> >
class StringHash : public Hash<VString, U, VString::NoCaseHashFunctor, Allocator>
{
public:
    typedef U                                                        ValueType;
    typedef StringHash<U, Allocator>                                 SelfType;
    typedef Hash<VString, U, VString::NoCaseHashFunctor, Allocator>    BaseType;

public:

    void    operator = (const SelfType& src) { BaseType::operator = (src); }

    bool    GetCaseInsensitive(const VString& key, U* pvalue) const
    {
        VString::NoCaseKey ikey(key);
        return BaseType::GetAlt(ikey, pvalue);
    }
    // Pointer-returning get variety.
    const U* GetCaseInsensitive(const VString& key) const
    {
        VString::NoCaseKey ikey(key);
        return BaseType::GetAlt(ikey);
    }
    U*  GetCaseInsensitive(const VString& key)
    {
        VString::NoCaseKey ikey(key);
        return BaseType::GetAlt(ikey);
    }


    typedef typename BaseType::Iterator base_iterator;

    base_iterator    FindCaseInsensitive(const VString& key)
    {
        VString::NoCaseKey ikey(key);
        return BaseType::FindAlt(ikey);
    }

    // Set just uses a find and assigns value if found. The key is not modified;
    // this behavior is identical to Flash string variable assignment.
    void    SetCaseInsensitive(const VString& key, const U& value)
    {
        base_iterator it = FindCaseInsensitive(key);
        if (it != BaseType::End())
        {
            it->Second = value;
        }
        else
        {
            BaseType::Add(key, value);
        }
    }
};

NV_NAMESPACE_END
