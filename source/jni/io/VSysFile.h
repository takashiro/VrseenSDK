#pragma once

#include "vglobal.h"

#include "VDelegatedFile.h"
#include "VString.h"

NV_NAMESPACE_BEGIN


struct VFileStat
{
    // No change or create time because they are not available on most systems
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

//VSysFile类实现直接访问文件系统中的文件的功能，封装了Open、close函数
//io的入口和出口
class VSysFile : public VDelegatedFile
{
public:

    VSysFile();
    VSysFile(VFile *pfile);
    VSysFile(const VString& path, int flags = Open_Read);

    bool  open(const VString& path, int flags = Open_Read);

    inline bool  Create(const VString& path)
    { return open(path, Open_ReadWrite | Open_Create); }

    static bool  GetFileStat(VFileStat* pfileStats, const VString& path);

    int   errorCode() override;
    bool  isOpened() override;
    bool  close() override;

};

NV_NAMESPACE_END
