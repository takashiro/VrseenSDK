#include "VJson.h"

#include <iostream>
#include <sstream>
#include <fstream>

NV_NAMESPACE_BEGIN

Json::Json()
{
    p_ptr->type = None;
}

Json::Json(bool value)
{
    p_ptr->type = Boolean;
    p_ptr->boolean = value;
}

Json::Json(int value)
{
    p_ptr->type = Number;
    p_ptr->number = value;
}

Json::Json(double value)
{
    p_ptr->type = Number;
    p_ptr->number = value;
}

Json::Json(const std::string &value)
{
    p_ptr->type = String;
    p_ptr->str = new std::string(value);
}

Json::Json(const char *value)
{
    p_ptr->type = String;
    p_ptr->str = new std::string(value);
}

Json::Json(Type type)
{
    p_ptr->type = type;
    switch(type)
    {
    case Boolean:
        p_ptr->boolean = false;
        break;
    case Number:
        p_ptr->number = 0.0;
        break;
    case String:
        p_ptr->str = new std::string();
        break;
    case Array:
        p_ptr->array = new std::vector<Json>();
        break;
    case Object:
        p_ptr->object = new JsonObject();
        break;
    default:
        p_ptr->type = Null;
        break;
    }
}

Json::Type Json::type() const
{
	return p_ptr->type;
}

bool Json::toBool() const
{
	switch (p_ptr->type) {
	case Boolean:
		return p_ptr->boolean;
	case Number:
		return (int) p_ptr->number != 0;
	case String:
		return !p_ptr->str->empty();
	case Array:
		return p_ptr->array->size() > 0;
	case Object:
		return p_ptr->object->size() > 0;
	default:
		return false;
	}
}

double Json::toDouble() const
{
	if (p_ptr->type == Number)
		return p_ptr->number;

	if (p_ptr->type == String) {
		std::stringstream s;
		s << *(p_ptr->str);
		double number;
		s >> number;
		return number;
	}

	return 0.0;
}

int Json::toInt() const
{
	if (p_ptr->type == Number)
		return static_cast<int>(p_ptr->number);

	if (p_ptr->type == String) {
		std::stringstream s;
		s << *(p_ptr->str);
		int number;
		s >> number;
		return number;
	}

	return 0;
}

std::string Json::toString() const
{
	if (p_ptr->type == String)
		return *(p_ptr->str);

	if (p_ptr->type == Number) {
		std::stringstream s;
		s << p_ptr->number;
		std::string str;
		s >> str;
		return str;
	}

	return std::string();
}

const JsonArray &Json::toArray() const
{
    return *(p_ptr->array);
}

const JsonObject &Json::toObject() const
{
    return *(p_ptr->object);
}

bool Json::operator==(bool value) const
{
	return p_ptr->type == Boolean && p_ptr->boolean == value;
}

bool Json::operator==(double value) const
{
	return p_ptr->type == Number && p_ptr->number == value;
}

bool Json::operator==(const std::string &value) const
{
	return p_ptr->type == String && *(p_ptr->str) == value;
}

bool Json::operator==(const Json &value) const
{
	if (type() != value.type())
		return false;

	switch (p_ptr->type) {
	case Null:
		return true;
	case Boolean:
		return p_ptr->boolean == value.p_ptr->boolean;
	case Number:
		return p_ptr->number == value.p_ptr->number;
	case String:
		return p_ptr->str == value.p_ptr->str;
	case Array:{
		if (p_ptr->array->size() != value.p_ptr->array->size())
			return false;

		int size = p_ptr->array->size();
		for (int i = 0; i < size; i++) {
			if (p_ptr->array->at(i) != value.p_ptr->array->at(i))
				return false;
		}
		break;
	}
	case Object:{
		if (p_ptr->object->size() != value.p_ptr->object->size())
			return false;

		for (auto iter = p_ptr->object->begin(); iter != p_ptr->object->end(); iter++) {
			if (iter->second != value[iter->first])
				return false;
		}
		break;
	}
	default:
		return true;
	}
	return true;
}

void Json::append(const Json &child)
{
    p_ptr.detach();
	p_ptr->array->push_back(child);
}

void Json::removeAt(int index)
{
    p_ptr.detach();
	p_ptr->array->erase(p_ptr->array->begin() + index);
}

Json &Json::operator[](int index)
{
    p_ptr.detach();
	return p_ptr->array->operator[](index);
}

const Json &Json::operator[](int index) const
{
	return p_ptr->array->operator[](index);
}

const Json &Json::at(int index) const
{
	return p_ptr->array->operator[](index);
}

void Json::removeOne(const Json &value)
{
    p_ptr.detach();
	for (auto i = p_ptr->array->begin(); i != p_ptr->array->end(); i++) {
		if (*i == value) {
			p_ptr->array->erase(i);
			break;
		}
	}
}

void Json::removeAll(const Json &value)
{
    p_ptr.detach();
	for (auto i = p_ptr->array->begin(); i != p_ptr->array->end(); i++) {
		if (*i == value) {
			i = p_ptr->array->erase(i);
		}
    }
}

void Json::insert(const std::string &key, const Json &value)
{
    p_ptr.detach();
	p_ptr->object->operator[](key) = value;
}

void Json::remove(const std::string &key)
{
    p_ptr.detach();
    p_ptr->object->erase(key);
}

Json &Json::operator[](const std::string &key)
{
    p_ptr.detach();
	return p_ptr->object->operator[](key);
}

const Json &Json::operator[](const std::string &key) const
{
	return p_ptr->object->operator[](key);
}

bool Json::contains(const std::string &key) const
{
	return p_ptr->object->find(key) != p_ptr->object->end();
}

const Json &Json::value(const std::string &key) const
{
	return p_ptr->object->operator[](key);
}

const Json &Json::value(const std::string &key, const Json &defaultValue) const
{
	if (contains(key))
		return value(key);
	return defaultValue;
}

size_t Json::size() const
{
	if (isArray())
		return p_ptr->array->size();
	else if (isObject())
		return p_ptr->object->size();
	return 0;
}

void Json::clear()
{
    if (p_ptr->type == Array) {
        p_ptr.reset();
        p_ptr->type = Array;
        p_ptr->array = new std::vector<Json>();
    } else if (p_ptr->type == Object) {
        p_ptr.reset();
        p_ptr->type = Object;
        p_ptr->object = new JsonObject();
    }
}

std::string &Json::string()
{
	return *(p_ptr->str);
}

std::vector<Json> &Json::array()
{
	return *(p_ptr->array);
}

JsonObject &Json::object()
{
	return *(p_ptr->object);
}

namespace
{

bool json_try_read(std::istream &in, char ch)
{
    char first;
    do {
        in.get(first);
    } while (isspace(first));

    if (first == ch)
        return true;

    in.unget();
    return false;
}

bool json_try_read(std::istream &in, const char *str)
{
    char first;
    do {
        in.get(first);
    } while (isspace(first));

    const char *cur = str;
    if (*cur != first) {
        in.unget();
        return false;
    }

    cur++;
    while (*cur) {
        if (json_try_read(in, *cur)) {
            cur++;
        } else {
            while (cur != str) {
                cur--;
                in.putback(*cur);
            }
            return false;
        }
    }
    return true;
}

}

std::istream &operator>>(std::istream &in, Json &value)
{
    if (value.type() != Json::None)
        return in;

    if (json_try_read(in, "null")) {
        value.p_ptr->type = Json::Null;
    } else if (json_try_read(in, "false")) {
        value.p_ptr->type = Json::Boolean;
        value.p_ptr->boolean = false;
    } else if (json_try_read(in, "true")) {
        value.p_ptr->type = Json::Boolean;
        value.p_ptr->boolean = true;
    } else if (json_try_read(in, '"')) {
        value.p_ptr->type = Json::String;
        value.p_ptr->str = new std::string();
        std::string &str = (*value.p_ptr->str);
        char ch;
        while (!in.eof()) {
            in.get(ch);
            if (ch == '\\') {
                in.get(ch);
                if (ch == '"') {
                    str += ch;
                    continue;
                }
            } else if (ch == '"') {
                break;
            }
            str += ch;
        }
    } else {
        char ch;
        in.get(ch);
        if (ch == '-' || (ch >= '0' && ch <= '9') || ch == '.') {
            in.unget();
            value.p_ptr->type = Json::Number;
            in >> value.p_ptr->number;
        } else if (ch == '[') {
            value.p_ptr->type = Json::Array;
            value.p_ptr->array = new std::vector<Json>();
            std::vector<Json> &array = (*value.p_ptr->array);
            while (true) {
                Json element;
                in >> element;
                if (element.type() == Json::None) {
                    std::cerr << "Invalid element of array" << std::endl;
                    break;
                }
                array.push_back(element);

                if (!json_try_read(in, ',')) {
                    if (!(json_try_read(in, ']')))
                        std::cerr << "Expect ]" << std::endl;
                    else
                        break;
                }
            }
        } else if (ch == '{') {
            value.p_ptr->type = Json::Object;
            value.p_ptr->object = new JsonObject();

            JsonObject &object = *(value.p_ptr->object);
            while (true) {
                Json key;
                in >> key;
                if (key.type() != Json::String) {
                    std::cerr << "Expect JSON key string." << std::endl;
                    break;
                }
                if (!json_try_read(in, ':')) {
                    std::cerr << "Expect :" << std::endl;
                    break;
                }

                Json value;
                in >> value;
                if (value.type() == Json::None) {
                    std::cerr << "Invalid value of JSON object" << std::endl;
                    break;
                }

                object[key.string()] = value;

                if (!json_try_read(in, ',')) {
                    if (!(json_try_read(in, '}')))
                        std::cerr << "Expect }" << std::endl;
                    else
                        break;
                }
            }
        } else {
            std::cerr << "Unexpected character" << std::endl;
        }
    }
    return in;
}

std::ostream &operator<<(std::ostream &out, const Json &value)
{
    switch (value.type()) {
    case Json::Null:
        out << "null";
        break;
    case Json::Boolean:
        if (value.toBool())
            out << "true";
        else
            out << "false";
        break;
    case Json::Number:
        out << value.toDouble();
        break;
    case Json::String:
        out << '"' << value.toString() << '"';
        break;
    case Json::Array:{
        out << '[';
        const JsonArray &elements = value.toArray();
        int size = elements.size();
        if (size > 0) {
            out << elements.at(0);
            for(int i = 1; i < size; i++)
                out << ", " << elements.at(i);
        }
        out << ']';
        break;
    }
    case Json::Object:{
        out << '{';
        const JsonObject &jsonObject = value.toObject();
        if (!jsonObject.empty()) {
            JsonObject::const_iterator iter = jsonObject.begin();
            out << '"' << iter->first << "\" : " << iter->second;
            for (iter++; iter != jsonObject.end(); iter++) {
                out << ", \"" << iter->first << "\" : " << iter->second;
            }
        }
        out << '}';
        break;
    }
    default:
        std::cerr << "invalid JSON value can't be printed" << std::endl;
        break;
    }
    return out;
}

Json Json::Parse(const char *str)
{
	std::stringstream s;
	s << str;
	Json json;
	s >> json;
	return json;
}

Json Json::Load(const char *path)
{
	std::ifstream s(path, std::ios::binary);
	Json json;
	s >> json;
	return json;
}

void JsonData::cloneData(const JsonData &source)
{
    type = source.type;
    switch (type) {
    case Json::Boolean:
        boolean = source.boolean;
        break;
    case Json::Number:
        number = source.number;
        break;
    case Json::String:
        str = new std::string(*source.str);
        break;
    case Json::Array:
        array = new std::vector<Json>(*source.array);
        break;
    case Json::Object:
        object = new JsonObject(*source.object);
        break;
    default:
    	break;
    }
}

NV_NAMESPACE_END
