/*
 * VDir.h
 *
 *  Created on: 2016年3月18日
 *      Author: gaojialing
 */

#pragma once

#include "VPath.h"
#include "VArray.h"
#include "VString.h"

NV_NAMESPACE_BEGIN

class VDir
{
public:
    VDir(const VPath &path);
    VDir();

    bool exists(const VString &filename) ;
    bool contains( VPath path, mode_t mode );
    void makePath( const VPath &path, mode_t mode );
    VArray<VString> entryList() const;

    static VArray<VString> Search(const VArray<VString> &searchPaths, const VString &relativePath);
    const VPath &getPath() const;

private:
    VPath m_path;
};

NV_NAMESPACE_END
