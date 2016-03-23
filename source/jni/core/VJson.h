#pragma once

#include <string>
#include <istream>
#include <ostream>

#include "VString.h"
#include "VArray.h"
#include "VMap.h"

NV_NAMESPACE_BEGIN

class VJson;
typedef VArray<VJson> VJsonArray;
typedef VMap<VString, VJson> VJsonObject;

class VJson
{
public:
	enum Type
	{
        Null,
		Boolean,
		Number,
		String,
		Array,
		Object
	};

    //Null
	VJson();

    //Boolean
    VJson(bool value);

    //Number
    VJson(int value);
    VJson(double value);

    //String
    VJson(const char *value);
	VJson(const std::string &value);
    VJson(const VString &value);
    VJson(VString &&value);

    //Array
    VJson(const VJsonArray &array);
    VJson(VJsonArray &&array);

    //Object
    VJson(const VJsonObject &object);
    VJson(VJsonObject &&object);

    VJson(const VJson &source);
    VJson(VJson &&source);
    ~VJson();

    Type type() const { return m_type; }
	bool isNull() const { return type() == VJson::Null; }
	bool isNumber() const { return type() == VJson::Number; }
	bool isString() const { return type() == VJson::String; }
	bool isArray() const { return type() == VJson::Array; }
	bool isObject() const { return type() == VJson::Object; }

	bool toBool() const;

    double toDouble() const;
	int toInt() const;

    VString toString() const;
    std::string toStdString() const;

    VJsonArray &array();
    const VJsonArray &toArray() const;

    VJsonObject &object();
    const VJsonObject &toObject() const;

    //Assignment functions
    VJson &operator =(const VJson &source);
    VJson &operator =(VJson &&source);

	//Array Functions
    VJson &at(uint i) { return m_value.array->at(i); }
    VJson &operator[](uint i) { return m_value.array->at(i); }
    const VJson &operator[](uint i) const { return m_value.array->at(i); }
    const VJson &at(uint i) const { return m_value.array->at(i); }

	//Object Functions
    VJson &operator[](const VString &key) { return (*(m_value.object))[key]; }
    const VJson &operator[](const VString &key) const { return m_value.object->value(key); }
    const VJson &value(const VString &key) const { return m_value.object->value(key); }
    const VJson &value(const VString &key, const VJson &defaultValue) const;
    bool contains(const VString &key) const { return m_value.object->contains(key); }

	//Array/Object functions
    uint size() const;
	void clear();

    friend std::istream &operator>>(std::istream &in, VJson &value);
    friend std::ostream &operator<<(std::ostream &out, const VJson &value);

    static VJson Parse(const VByteArray &str);
    static VJson Load(const VString &path);

private:
    void copy(const VJson &source);
    void release();

    VJson::Type m_type;

    union Value
    {
        bool boolean;
        double number;
        VString *str;
        VJsonArray *array;
        VJsonObject *object;
    };
    Value m_value;
};

NV_NAMESPACE_END
