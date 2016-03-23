#pragma once

#include "vglobal.h"
#include "VString.h"
#include "VMap.h"

NV_NAMESPACE_BEGIN

template<class T>
class VStringHash : public VMap<VString, T>
{
public:
    typedef T ValueType;
    typedef std::map<VString, T> SelfType;
    typedef typename std::map<VString, T>::iterator Iterator;
    typedef typename std::map<VString, T>::const_iterator ConstIterator;
};

NV_NAMESPACE_END
