#include "VLog.h"

#include <sstream>
#include <android/log.h>

NV_NAMESPACE_BEGIN

struct VLog::Private
{
    VLog::Priority priority;
    std::stringstream buffer;
};

VLog::VLog(VLog::Priority priority)
    : d(new Private)
{
    d->priority = priority;
}

VLog::~VLog()
{
    __android_log_write(d->priority, "VLog", d->buffer.str().data());
    delete d;
}

VLog &VLog::operator << (short num)
{
    d->buffer << num << " ";
    return *this;
}

VLog &VLog::operator << (ushort num)
{
    d->buffer << num << " ";
    return *this;
}

VLog &VLog::operator << (int num)
{
    d->buffer << num << " ";
    return *this;
}

VLog &VLog::operator << (uint num)
{
    d->buffer << num << " ";
    return *this;
}

VLog &VLog::operator << (float num)
{
    d->buffer << num << " ";
    return *this;
}

VLog &VLog::operator << (double num)
{
    d->buffer << num << " ";
    return *this;
}

VLog &VLog::operator << (const char *str)
{
    d->buffer << str << " ";
    return *this;
}

VLog &VLog::operator << (const VString &str)
{
    d->buffer << str << " ";
    return *this;
}

VLog &VLog::operator << (const VByteArray &str)
{
    d->buffer << str << " ";
    return *this;
}

NV_NAMESPACE_END
