#include "VFile.h"

NV_NAMESPACE_BEGIN

VFile::VFile()
{
}

vint64 VFile::readData(char *data, vint64 maxSize)
{
    NV_UNUSED(data, maxSize);
    return 0;
}

vint64 VFile::writeData(const char *data, vint64 maxSize)
{
    NV_UNUSED(data, maxSize);
    return 0;
}

NV_NAMESPACE_END
