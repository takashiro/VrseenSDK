#pragma once

#include "vglobal.h"

NV_NAMESPACE_BEGIN

//-----------------------------------------------------------------------------------
// *** File Statistics

// This class contents are similar to _stat, providing
// creation, modify and other information about the file.
class VFileStat
{
public:
    // No change or create time because they are not available on most systems
//    SInt64  modifyTime;
//    SInt64  accessTime;
//    SInt64  fileSize;
    ulong  modifyTime;
    ulong  accessTime;
    ulong  fileSize;

    bool operator== (const VFileStat& stat) const
    {
        return ( (modifyTime == stat.modifyTime) &&
                 (accessTime == stat.accessTime) &&
                 (fileSize == stat.fileSize) );
    }
};

NV_NAMESPACE_END
