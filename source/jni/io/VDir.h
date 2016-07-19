#pragma once

#include "VPath.h"
#include "VArray.h"
#include "VString.h"

NV_NAMESPACE_BEGIN

class VDir
{
public:
    VDir();
    VDir(const VPath &path);

    const VPath &path() const { return m_path; }

    bool reach();

    bool exists() const;
    void makeDir();

    bool isReadable() const;
    bool isWritable() const;

    bool contains(const VString &relativePath);
    VArray<VString> entryList() const;

    static VArray<VString> Search(const VArray<VString> &searchPaths, const VString &relativePath);

private:
    VPath m_path;
};

NV_NAMESPACE_END
