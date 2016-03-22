/*
 * VDir.cpp
 *
 *  Created on: 2016年3月18日
 *      Author: gaojialing
 */
#include "VDir.h"

#include "VPath.h"
#include "VString.h"

#include "VStringHash.h"

#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>
#include <algorithm>

NV_NAMESPACE_BEGIN

VDir::VDir(const VPath &path)
{
    m_path = path;
}

VDir::VDir()
{
}

const VPath &VDir::getPath() const
{
    return m_path;
}

bool VDir::exists(const VString &filename)
{
    struct stat st;
    int result = stat(filename.toCString(), &st);
    return result == 0;
}

bool VDir::contains(VPath path, mode_t mode)
{
    int len = path.size();
    if (path.at(len - 1) != '/') { // directory ends in a slash
        int end = len - 1;
        for (; end > 0 && path.at(end) != '/'; end--)
            ;
        path = VPath(path.data(), end);
    }
    return access(path.toCString(), mode) == 0;
}

void VDir::makePath(const VPath &path, mode_t mode)
{
    char *vpath = strdup(path.toUtf8().data());
    char *currentChar = nullptr;

    for (currentChar = vpath + 1; *currentChar; ++currentChar) {
        if (*currentChar == '/') {
            *currentChar = 0;
            DIR * checkDir = opendir(vpath);
            if (checkDir == NULL) {
                mkdir(vpath, mode);
            } else {
                closedir(checkDir);
            }
            *currentChar = '/';
        }
    }

    free(vpath);
}

VArray<VString> VDir::entryList() const
{
    VArray<VString> strings;
    if (0 == getPath().size()) {
        return strings;
    }
    const char* DirPath = getPath().toCString();
    DIR * dir = opendir(DirPath);
    if (dir != NULL) {
        struct dirent * entry;
        while ((entry = readdir(dir)) != NULL) {
            if (entry->d_name[0] == '.') {
                continue;
            }
            if (entry->d_type == DT_DIR) {
                VString s(DirPath);
                s += entry->d_name;
                s += "/";
                strings.append(s);
            } else if (entry->d_type == DT_REG) {
                VString s(DirPath);
                s += entry->d_name;
                strings.append(s);
            }
        }
        closedir(dir);
    }
    std::sort(strings.begin(), strings.end());
    return strings;
}

VArray<VString> VDir::Search(const VArray<VString> &searchPaths, const VString &relativePath)
{
    VArray<VString> uniqueStrings;

    const int numSearchPaths = searchPaths.length();
    for (int index = 0; index < numSearchPaths; ++index) {
        const VString fullPath = searchPaths[index] + VString(relativePath);

        DIR * dir = opendir(fullPath.toCString());
        if (dir != NULL) {
            struct dirent * entry;
            while ((entry = readdir(dir)) != NULL) {
                if (entry->d_name[0] == '.') {
                    continue;
                }
                if (entry->d_type == DT_DIR) {
                    VString s(relativePath);
                    s += entry->d_name;
                    s += "/";
                    uniqueStrings.push_back(s);
                } else if (entry->d_type == DT_REG) {
                    VString s(relativePath);
                    s += entry->d_name;
                    uniqueStrings.push_back(s);
                }
            }
            closedir(dir);
        }
    }

    return uniqueStrings;
}

NV_NAMESPACE_END
