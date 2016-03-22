#include "VVariant.h"

#include "VLog.h"

#include <sstream>

NV_NAMESPACE_BEGIN

VVariant::VVariant()
    : m_type(Null)
{
}

VVariant::VVariant(bool value)
    : m_type(Boolean)
{
    m_value.boolean = value;
}

VVariant::VVariant(int value)
    : m_type(Int)
{
    m_value.decimal = value;
}

VVariant::VVariant(uint value)
    : m_type(UInt)
{
    m_value.udecimal = value;
}

VVariant::VVariant(long long value)
    : m_type(LongLong)
{
    m_value.bdecimal = value;
}

VVariant::VVariant(ulonglong value)
    : m_type(ULongLong)
{
    m_value.ubdecimal = value;
}

VVariant::VVariant(float value)
    : m_type(Float)
{
    m_value.real = value;
}

VVariant::VVariant(double value)
    : m_type(Double)
{
    m_value.dreal = value;
}

VVariant::VVariant(void *pointer)
    : m_type(Pointer)
{
    m_value.pointer = pointer;
}

VVariant::VVariant(const VString &value)
    : m_type(String)
{
    m_value.str = new VString(value);
}

VVariant::VVariant(VString &&value)
    : m_type(String)
{
    m_value.str = new VString(value);
}

VVariant::VVariant(const VVariantArray &array)
    : m_type(Array)
{
    m_value.array = new VVariantArray(array);
}

VVariant::VVariant(VVariantArray &&array)
    : m_type(Array)
{
    m_value.array = new VVariantArray(array);
}

VVariant::VVariant(const VVariantMap &map)
    : m_type(Map)
{
    m_value.map = new VVariantMap(map);
}

VVariant::VVariant(VVariantMap &&map)
    : m_type(Map)
{
    m_value.map = new VVariantMap(map);
}

VVariant::VVariant(const VVariant &var)
    : m_type(var.m_type)
{
    switch (m_type) {
    case Null:
    case Boolean:
    case Int:
    case UInt:
    case LongLong:
    case ULongLong:
    case Float:
    case Double:
    case Pointer:
        m_value = var.m_value;
        break;
    case String:
        m_value.str = new VString(*(var.m_value.str));
        break;
    case Array:
        m_value.array = new VVariantArray(*(var.m_value.array));
        break;
    case Map:
        m_value.map = new VVariantMap(*(var.m_value.map));
        break;
    default:
        vAssert("VVariant Does not support such a type.");
    }
}

VVariant::VVariant(VVariant &&var)
    : m_type(var.m_type)
    , m_value(var.m_value)
{
    var.m_type = Null;
}

VVariant::~VVariant()
{
    release();
}

bool VVariant::toBool() const
{
    switch (m_type) {
    case Boolean:
        return m_value.boolean;
    case Null:
        return false;
    case Int:
        return m_value.decimal != 0;
    case UInt:
        return m_value.udecimal != 0;
    case LongLong:
        return m_value.bdecimal != 0;
    case ULongLong:
        return m_value.ubdecimal != 0;
    case Float:
        return m_value.real != 0;
    case Double:
        return m_value.dreal != 0;
    case Pointer:
        return m_value.pointer;
    case String:
        return !m_value.str->isEmpty();
    case Array:
        return !m_value.array->isEmpty();
    case Map:
        return !m_value.map->isEmpty();
    default:
        return false;
    }
}

int VVariant::toInt() const
{
    switch (m_type) {
    case Int:
        return m_value.decimal;
    case Null:
        return 0;
    case Boolean:
        return m_value.boolean ? 1 : 0;
    case UInt:
        return static_cast<int>(m_value.udecimal);
    case LongLong:
        return static_cast<int>(m_value.bdecimal);
    case ULongLong:
        return static_cast<int>(m_value.ubdecimal);
    case Float:
        return static_cast<int>(m_value.real);
    case Double:
        return static_cast<int>(m_value.dreal);
    case String:
        return m_value.str->toInt();
    case Array:
        return m_value.array->length();
    case Map:
        return static_cast<int>(m_value.map->size());
    default:
        return 0;
    }
}

uint VVariant::toUInt() const
{
    switch (m_type) {
    case UInt:
        return m_value.udecimal;
    case Null:
        return 0;
    case Boolean:
        return m_value.boolean ? 1 : 0;
    case Int:
        return m_value.decimal;
    case LongLong:
        return static_cast<uint>(m_value.bdecimal);
    case ULongLong:
        return static_cast<uint>(m_value.ubdecimal);
    case Float:
        return static_cast<uint>(m_value.real);
    case Double:
        return static_cast<uint>(m_value.dreal);
    case String:
        return m_value.str->toInt();
    case Array:
        return m_value.array->size();
    case Map:
        return m_value.map->size();
    default:
        return 0;
    }
}

float VVariant::toFloat() const
{
    switch (m_type) {
    case Float:
        return m_value.real;
    case Null:
        return 0;
    case Boolean:
        return m_value.boolean ? 1 : 0;
    case Int:
        return m_value.decimal;
    case UInt:
        return m_value.udecimal;
    case LongLong:
        return m_value.bdecimal;
    case ULongLong:
        return m_value.ubdecimal;
    case Double:
        return m_value.dreal;
    default:
        return 0;
    }
}

double VVariant::toDouble() const
{
    switch (m_type) {
    case Double:
        return m_value.dreal;
    case Null:
        return 0;
    case Boolean:
        return m_value.boolean ? 1 : 0;
    case Int:
        return m_value.decimal;
    case UInt:
        return m_value.udecimal;
    case LongLong:
        return m_value.bdecimal;
    case ULongLong:
        return m_value.ubdecimal;
    case Float:
        return m_value.real;
    default:
        return 0;
    }
}

void *VVariant::toPointer() const
{
    return m_type == Pointer ? m_value.pointer : nullptr;
}

const VString &VVariant::toString() const
{
    static VString EmptyString;
    return m_type == String ? *(m_value.str) : EmptyString;
}

const VVariantArray &VVariant::toArray() const
{
    static VVariantArray EmptyArray;
    return m_type == Array ? *(m_value.array) : EmptyArray;
}

const VVariantMap &VVariant::toMap() const
{
    static VVariantMap EmptyMap;
    return m_type == Map ? *(m_value.map) : EmptyMap;
}

VVariant &VVariant::at(uint index)
{
    vAssert(m_type == Array);
    return m_value.array->at(index);
}

const VVariant &VVariant::at(uint index) const
{
    vAssert(m_type == Array);
    return m_value.array->at(index);
}

VVariant &VVariant::value(const VString &key)
{
    vAssert(m_type == Map);
    return m_value.map->value(key);
}

const VVariant &VVariant::value(const VString &key) const
{
    vAssert(m_type == Map);
    return m_value.map->value(key);
}

int VVariant::length() const
{
    if (m_type == Array) {
        return m_value.array->length();
    } else if (m_type == Map) {
        return (int) m_value.map->size();
    } else if (m_type == String) {
        return m_value.str->length();
    }
    vAssert(false)
    return 0;
}

uint VVariant::size() const
{
    if (m_type == Array) {
        return m_value.array->size();
    } else if (m_type == Map) {
        return m_value.map->size();
    } else if (m_type == String) {
        return m_value.str->size();
    }
    vAssert(false)
            return 0;
}

void VVariant::release()
{
    switch (m_type) {
    case String:
        delete m_value.str;
        break;
    case Array:
        delete m_value.array;
        break;
    case Map:
        delete m_value.map;
        break;
    default:;
    }
}

VVariant &VVariant::operator=(const VVariant &var)
{
    release();
    m_type = var.m_type;
    switch (m_type) {
    case Null:
    case Boolean:
    case Int:
    case UInt:
    case LongLong:
    case ULongLong:
    case Float:
    case Double:
    case Pointer:
        m_value = var.m_value;
        break;
    case String:
        m_value.str = new VString(*(var.m_value.str));
        break;
    case Array:
        m_value.array = new VVariantArray(*(var.m_value.array));
        break;
    case Map:
        m_value.map = new VVariantMap(*(var.m_value.map));
        break;
    default:
        vAssert("VVariant Does not support such a type.");
    }
    return *this;
}

VVariant &VVariant::operator=(VVariant &&source)
{
    release();
    m_type = source.m_type;
    m_value = source.m_value;
    source.m_type = Null;
    return *this;
}

NV_NAMESPACE_END
