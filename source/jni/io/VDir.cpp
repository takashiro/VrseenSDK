#include "VDir.h"

#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>
#include <algorithm>

NV_NAMESPACE_BEGIN

VDir::VDir()
    : m_path(".")
{
}

VDir::VDir(const VPath &path)
    : m_path(path)
{
}

bool VDir::reach()
{
    return chdir(m_path.toUtf8().data()) == 0;
}

bool VDir::exists() const
{
    return access(m_path.toUtf8().data(), F_OK) == 0;
}

bool VDir::contains(const VString &path)
{
    VString fullPath = m_path + path;
    return access(fullPath.toUtf8().data(), F_OK) == 0;
}

void VDir::makeDir()
{
    char *path = strdup(m_path.toUtf8().data());
    int mode = S_IRUSR | S_IWUSR;

    for (char *currentChar = path + 1; *currentChar; ++currentChar) {
        if (*currentChar == '/') {
            *currentChar = 0;
            DIR *checkDir = opendir(path);
            if (checkDir == NULL) {
                mkdir(path, mode);
            } else {
                closedir(checkDir);
            }
            *currentChar = '/';
        }
    }

    free(path);
}

bool VDir::isReadable() const
{
    return access(m_path.toUtf8().data(), R_OK) == 0;
}

bool VDir::isWritable() const
{
    return access(m_path.toUtf8().data(), W_OK) == 0;
}

VArray<VString> VDir::entryList() const
{
    VArray<VString> entries;
    DIR *dir = opendir(m_path.toUtf8().data());
    if (dir != NULL) {
        struct dirent *entry;
        while ((entry = readdir(dir)) != NULL) {
            if (entry->d_name[0] == '.') {
                continue;
            }
            if (entry->d_type == DT_DIR) {
                VString subdir(entry->d_name);
                subdir += u'/';
                entries.append(std::move(subdir));
            } else if (entry->d_type == DT_REG) {
                VString file(entry->d_name);
                entries.append(std::move(file));
            }
        }
        closedir(dir);
    }
    std::sort(entries.begin(), entries.end());
    return entries;
}

VArray<VString> VDir::Search(const VArray<VString> &searchPaths, const VString &relativePath)
{
    VArray<VString> result;

    for (const VString &searchPath : searchPaths) {
        const VString fullPath = searchPath + relativePath;

        VDir dir(fullPath);
        VArray<VString> entryList = dir.entryList();
        for (const VString &entry : entryList) {
            result << fullPath + entry;
        }
    }

    return result;
}

NV_NAMESPACE_END
