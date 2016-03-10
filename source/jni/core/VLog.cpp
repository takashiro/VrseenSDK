#include "VLog.h"

#include <sstream>
#include <android/log.h>

NV_NAMESPACE_BEGIN

struct VLog::Private
{
    const char *file;
    uint line;
    VLog::Priority priority;
    std::stringstream buffer;
};

VLog::VLog(const char *file, uint line, VLog::Priority priority)
    : d(new Private)
{
    d->file = file;
    d->line = line;
    d->priority = priority;
}

VLog::~VLog()
{
    VString tag;
    tag.sprintf("%s: %d", d->file, d->line);
    __android_log_write(d->priority, tag.toLatin1().data(), d->buffer.str().data());
    delete d;
}

VLog &VLog::operator << (char ch)
{
    d->buffer << ch << ' ';
    return *this;
}

VLog &VLog::operator << (char16_t ch)
{
    d->buffer << ch << ' ';
    return *this;
}

VLog &VLog::operator << (char32_t ch)
{
    d->buffer << ch << ' ';
    return *this;
}

VLog &VLog::operator << (short num)
{
    d->buffer << num << ' ';
    return *this;
}

VLog &VLog::operator << (ushort num)
{
    d->buffer << num << ' ';
    return *this;
}

VLog &VLog::operator << (int num)
{
    d->buffer << num << ' ';
    return *this;
}

VLog &VLog::operator << (uint num)
{
    d->buffer << num << ' ';
    return *this;
}

VLog &VLog::operator << (float num)
{
    d->buffer << num << ' ';
    return *this;
}

VLog &VLog::operator << (double num)
{
    d->buffer << num << ' ';
    return *this;
}

VLog &VLog::operator <<(long long num)
{
    d->buffer << num << ' ';
    return *this;
}

VLog &VLog::operator <<(ulonglong num)
{
    d->buffer << num << ' ';
    return *this;
}

VLog &VLog::operator << (const char *str)
{
    d->buffer << str << ' ';
    return *this;
}

VLog &VLog::operator << (const VString &str)
{
    d->buffer << str << ' ';
    return *this;
}

VLog &VLog::operator << (const VByteArray &str)
{
    d->buffer << str << ' ';
    return *this;
}

NV_NAMESPACE_END
