#pragma once

#include <vector>
#include <map>
#include <string>
#include <istream>
#include <ostream>

#include "VSharedPointer.h"
#include "VString.h"

NV_NAMESPACE_BEGIN

class VJson;
class JsonData;
typedef std::vector<VJson> JsonArray;
typedef std::map<std::string, VJson> JsonObject;

class VJson
{
public:
	enum Type
	{
		None,
		Null,
		Boolean,
		Number,
		String,
		Array,
		Object
	};

	VJson();
    VJson(bool value);
    VJson(int value);
    VJson(double value);
	VJson(const std::string &value);
    VJson(const VString &value);
    VJson(const char *value);
    VJson(Type type);

	Type type() const;
	bool isValid() const { return type() != VJson::None; }
	bool isInvalid() const { return type() == VJson::None; }
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
    const JsonArray &toArray() const;
    const JsonObject &toObject() const;

	bool operator==(bool value) const;
	bool operator!=(bool value) const { return !(*this == value); }

	bool operator==(double value) const;
	bool operator!=(double value) const { return !(*this == value); }

	bool operator==(const std::string &value) const;
	bool operator!=(const std::string &value) const { return !(*this == value); }

	bool operator==(const VJson &value) const;
	bool operator!=(const VJson &value) const { return !(*this == value); }

	//Array Functions
	void append(const VJson &value);
	void removeAt(int index);
	void removeOne(const VJson &value);
        void removeAll(const VJson &value);
	VJson &operator[](int i);
	const VJson &operator[](int i) const;
	const VJson &at(int i) const;

	//Object Functions
	void insert(const std::string &key, const VJson &value);
    void remove(const std::string &key);
    VJson &operator[](const std::string &key);
	const VJson &operator[](const std::string &key) const;
	const VJson &value(const std::string &key) const;
	const VJson &value(const std::string &key, const VJson &defaultValue) const;
	bool contains(const std::string &key) const;

	//Array/Object functions
	size_t size() const;
	void clear();

    friend std::istream &operator>>(std::istream &in, VJson &value);
    friend std::ostream &operator<<(std::ostream &out, const VJson &value);

    static VJson Parse(const char *str);
    static VJson Load(const char *path);

protected:
    VString &string();
	std::vector<VJson> &array();
	std::map<std::string, VJson> &object();

    VSharedPointer<JsonData> p_ptr;
};

class JsonData
{
public:
    VJson::Type type;

    union
    {
        bool boolean;
        double number;
        VString *str;
        std::vector<VJson> *array;
        std::map<std::string, VJson> *object;
    };

    JsonData() = default;

    JsonData(const JsonData &source)
    {
        cloneData(source);
    }

    const JsonData &operator = (const JsonData &source)
    {
        cloneData(source);
        return *this;
    }

private:
    void cloneData(const JsonData &source);
};

NV_NAMESPACE_END
