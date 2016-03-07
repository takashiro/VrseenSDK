#include "VByteArray.h"

NV_NAMESPACE_BEGIN

VByteArray::VByteArray(uint length, char ch)
{
    resize(length);
    for (uint i = 0; i < length; i++) {
        at(i) = ch;
    }
}

NV_NAMESPACE_END
