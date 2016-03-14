#pragma once

#include "vglobal.h"

#include "VDelegatedFile.h"
#include "VFileState.h"
#include "VDefaultFile.h"

NV_NAMESPACE_BEGIN

class   VSysFile;

//VSysFile类实现直接访问文件系统中的文件的功能，封装了Open、close函数
//io的入口和出口
class VSysFile : public VDelegatedFile
{
protected:
    //    VSysFile(const VSysFile &source) : VDelegatedFile () {  }
public:

    VSysFile();
    VSysFile(VFile *pfile);
    VSysFile(const VString& path, int flags = Open_Read);

    bool  open(const VString& path, int flags = Open_Read);

    inline bool  Create(const VString& path)
    { return open(path, Open_ReadWrite | Open_Create); }

    static bool  GetFileStat(VFileStat* pfileStats, const VString& path);

    virtual int   errorCode() override;
    virtual bool  isOpened() override;
    virtual bool  fileClose() override;

};

NV_NAMESPACE_END
