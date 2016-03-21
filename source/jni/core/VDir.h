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
#include "StringHash.h"

NV_NAMESPACE_BEGIN
class VDir{
private:
    VPath m_path;
public:
    VDir(const VPath &path);
    VDir();
    bool exists(const VString &filename) ;
    bool contains( VPath path, mode_t mode );
    void makePath( const VPath &path, mode_t mode );
    VArray<VString> entryList() const;
//
//    //在searchPaths中寻找relativePath，返回所有relativePath中的所有文件
    VArray< VString > Search(const VArray<VString> &searchPaths, const VString &relativePath);
    VPath getPath() const;
};
NV_NAMESPACE_END




