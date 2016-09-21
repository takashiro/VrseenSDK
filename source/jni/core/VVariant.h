#pragma once

#include "VString.h"
#include "VArray.h"
#include "VMap.h"

#include <functional>

NV_NAMESPACE_BEGIN

class VVariant;
typedef VArray<VVariant> VVariantArray;
typedef VMap<VString, VVariant> VVariantMap;

class VVariant
{
public:
    enum Type
    {
        Null,

        Boolean,
        Int,
        UInt,
        LongLong,
        ULongLong,
        Float,
        Double,
        Pointer,

        String,
        Array,
        Map,

        Closure,

        TypeCount
    };

    typedef std::function<void()> Function;

    VVariant();
    VVariant(bool value);
    VVariant(int value);
    VVariant(uint value);
    VVariant(long long value);
    VVariant(ulonglong value);
    VVariant(float value);
    VVariant(double value);
    VVariant(void *pointer);
    VVariant(const void *pointer);

    VVariant(const char *str);
    VVariant(const VString &str);
    VVariant(VString &&str);

    VVariant(const VVariantArray &array);
    VVariant(VVariantArray &&array);

    VVariant(const VVariantMap &map);
    VVariant(VVariantMap &&map);

    VVariant(const Function &function);
    VVariant(Function &&function);

    VVariant(const VVariant &var);
    VVariant(VVariant &&var);

    ~VVariant();

    Type type() const { return m_type; }

    bool isNull() const { return m_type == Null; }
    bool isBool() const { return m_type == Boolean; }
    bool isInt() const { return m_type == Int; }
    bool isUInt() const { return m_type == UInt; }
    bool isLongLong() const { return m_type == LongLong; }
    bool isULongLong() const { return m_type == ULongLong; }
    bool isFloat() const { return m_type == Float; }
    bool isDouble() const { return m_type == Double; }
    bool isPointer() const { return m_type == Pointer; }
    bool isString() const { return m_type == String; }
    bool isArray() const { return m_type == Array; }
    bool isMap() const { return m_type == Map; }
    bool isClosure() const { return m_type == Closure; }

    bool toBool() const;
    int toInt() const;
    uint toUInt() const;
    float toFloat() const;
    double toDouble() const;
    void *toPointer() const;

    const VString &toString() const;

    const VVariantArray &toArray() const;
    const VVariantMap &toMap() const;
    const Function& toFunction() const;

    VVariant &operator[](uint index) { return (*(m_value.array))[index]; }

    const VVariant &at(uint index) const;
    const VVariant &operator[](uint index) const { return at(index); }

    VVariant &operator[](const VString &key) { return (*(m_value.map))[key]; }

    const VVariant &value(const VString &key) const;
    const VVariant &operator[](const VString &key) const { return value(key); }

    int length() const;
    uint size() const;

    void execute() const;
    void operator()() const { execute(); }

    VVariant &operator=(const VVariant &source);
    VVariant &operator=(VVariant &&source);

private:
    void release();

    VVariant::Type m_type;

    union Value
    {
        bool boolean;
        int decimal;
        uint udecimal;
        long long bdecimal;
        ulonglong ubdecimal;
        float real;
        double dreal;
        void *pointer;
        VString *str;
        VVariantArray *array;
        VVariantMap *map;
        Function *function;
    };
    Value m_value;
};

NV_NAMESPACE_END
