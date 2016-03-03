#pragma once

#include <vector>
#include <map>
#include <string>
#include <istream>
#include <ostream>

#include "VSharedPointer.h"

NV_NAMESPACE_BEGIN

class Json;
class JsonData;
typedef std::vector<Json> JsonArray;
typedef std::map<std::string, Json> JsonObject;

class Json
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

	Json();
    Json(bool value);
    Json(int value);
    Json(double value);
	Json(const std::string &value);
    Json(const char *value);
    Json(Type type);

	Type type() const;
	bool isValid() const { return type() != Json::None; }
	bool isInvalid() const { return type() == Json::None; }
	bool isNull() const { return type() == Json::Null; }
	bool isNumber() const { return type() == Json::Number; }
	bool isString() const { return type() == Json::String; }
	bool isArray() const { return type() == Json::Array; }
	bool isObject() const { return type() == Json::Object; }

	bool toBool() const;
	double toDouble() const;
	int toInt() const;
	std::string toString() const;
        const JsonArray &toArray() const;
        const JsonObject &toObject() const;

	bool operator==(bool value) const;
	bool operator!=(bool value) const { return !(*this == value); }

	bool operator==(double value) const;
	bool operator!=(double value) const { return !(*this == value); }

	bool operator==(const std::string &value) const;
	bool operator!=(const std::string &value) const { return !(*this == value); }

	bool operator==(const Json &value) const;
	bool operator!=(const Json &value) const { return !(*this == value); }

	//Array Functions
	void append(const Json &value);
	void removeAt(int index);
	void removeOne(const Json &value);
        void removeAll(const Json &value);
	Json &operator[](int i);
	const Json &operator[](int i) const;
	const Json &at(int i) const;

	//Object Functions
	void insert(const std::string &key, const Json &value);
        void remove(const std::string &key);
	Json &operator[](const std::string &key);
	const Json &operator[](const std::string &key) const;
	const Json &value(const std::string &key) const;
	const Json &value(const std::string &key, const Json &defaultValue) const;
	bool contains(const std::string &key) const;

	//Array/Object functions
	size_t size() const;
	void clear();

    friend std::istream &operator>>(std::istream &in, Json &value);
    friend std::ostream &operator<<(std::ostream &out, const Json &value);

    static Json Parse(const char *str);
    static Json Load(const char *path);

protected:
	std::string &string();
	std::vector<Json> &array();
	std::map<std::string, Json> &object();

	VSharedPointer<JsonData> p_ptr;
};

class JsonData
{
public:
    Json::Type type;

    union
    {
        bool boolean;
        double number;
        std::string *str;
        std::vector<Json> *array;
        std::map<std::string, Json> *object;
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
