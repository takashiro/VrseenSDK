#include "VLog.h"

#include <sstream>

#ifdef ANDROID
#include <android/log.h>
#else
#include <iostream>
#endif

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
#ifdef ANDROID
    __android_log_print(d->priority, d->file, "[Line %u] %s", d->line, d->buffer.str().data());
#else
    std::cout << d->file << " [Line " << d->line << "] " << d->buffer.str() << std::endl;
#endif
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

VLog &VLog::operator <<(const std::string &str)
{
    d->buffer << str << ' ';
    return *this;
}

NV_NAMESPACE_END
