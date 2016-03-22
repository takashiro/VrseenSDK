#include "VJson.h"
#include "VByteArray.h"

#include <iostream>
#include <sstream>
#include <fstream>

NV_NAMESPACE_BEGIN

VJson::VJson()
{
    p_ptr->type = None;
}

VJson::VJson(bool value)
{
    p_ptr->type = Boolean;
    p_ptr->boolean = value;
}

VJson::VJson(int value)
{
    p_ptr->type = Number;
    p_ptr->number = value;
}

VJson::VJson(double value)
{
    p_ptr->type = Number;
    p_ptr->number = value;
}

VJson::VJson(const std::string &value)
{
    p_ptr->type = String;
    p_ptr->str = new VString(value);
}

VJson::VJson(const VString &value)
{
    p_ptr->type = String;
    p_ptr->str = new VString(value);
}

VJson::VJson(const char *value)
{
    p_ptr->type = String;
    p_ptr->str = new VString(value);
}

VJson::VJson(Type type)
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
        p_ptr->str = new VString();
        break;
    case Array:
        p_ptr->array = new std::vector<VJson>();
        break;
    case Object:
        p_ptr->object = new JsonObject();
        break;
    default:
        p_ptr->type = Null;
        break;
    }
}

VJson::Type VJson::type() const
{
	return p_ptr->type;
}

bool VJson::toBool() const
{
	switch (p_ptr->type) {
	case Boolean:
		return p_ptr->boolean;
	case Number:
		return (int) p_ptr->number != 0;
	case String:
        return !p_ptr->str->isEmpty();
	case Array:
		return p_ptr->array->size() > 0;
	case Object:
		return p_ptr->object->size() > 0;
	default:
		return false;
	}
}

double VJson::toDouble() const
{
	if (p_ptr->type == Number)
		return p_ptr->number;

	if (p_ptr->type == String) {
		std::stringstream s;
        s << p_ptr->str->toLatin1();
		double number;
		s >> number;
		return number;
	}

	return 0.0;
}

int VJson::toInt() const
{
	if (p_ptr->type == Number)
		return static_cast<int>(p_ptr->number);

	if (p_ptr->type == String) {
		std::stringstream s;
        s << p_ptr->str->toLatin1();
		int number;
		s >> number;
		return number;
	}

    return 0;
}

VString VJson::toString() const
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

    return VString();
}

std::string VJson::toStdString() const
{
	if (p_ptr->type == String)
        return p_ptr->str->toLatin1();

	if (p_ptr->type == Number) {
		std::stringstream s;
		s << p_ptr->number;
		std::string str;
		s >> str;
		return str;
	}

	return std::string();
}

const JsonArray &VJson::toArray() const
{
    return *(p_ptr->array);
}

const JsonObject &VJson::toObject() const
{
    return *(p_ptr->object);
}

bool VJson::operator==(bool value) const
{
	return p_ptr->type == Boolean && p_ptr->boolean == value;
}

bool VJson::operator==(double value) const
{
	return p_ptr->type == Number && p_ptr->number == value;
}

bool VJson::operator==(const std::string &value) const
{
	return p_ptr->type == String && *(p_ptr->str) == value;
}

bool VJson::operator==(const VJson &value) const
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

void VJson::append(const VJson &child)
{
    p_ptr.detach();
    p_ptr->array->push_back(child);
}

VJson &VJson::operator <<(const VJson &value)
{
    append(value);
    return *this;
}

void VJson::removeAt(int index)
{
    p_ptr.detach();
	p_ptr->array->erase(p_ptr->array->begin() + index);
}

VJson &VJson::operator[](int index)
{
    p_ptr.detach();
	return p_ptr->array->operator[](index);
}

const VJson &VJson::operator[](int index) const
{
	return p_ptr->array->operator[](index);
}

const VJson &VJson::at(int index) const
{
	return p_ptr->array->operator[](index);
}

void VJson::removeOne(const VJson &value)
{
    p_ptr.detach();
	for (auto i = p_ptr->array->begin(); i != p_ptr->array->end(); i++) {
		if (*i == value) {
			p_ptr->array->erase(i);
			break;
		}
	}
}

void VJson::removeAll(const VJson &value)
{
    p_ptr.detach();
	for (auto i = p_ptr->array->begin(); i != p_ptr->array->end(); i++) {
		if (*i == value) {
			i = p_ptr->array->erase(i);
		}
    }
}

void VJson::insert(const std::string &key, const VJson &value)
{
    p_ptr.detach();
	p_ptr->object->operator[](key) = value;
}

void VJson::remove(const std::string &key)
{
    p_ptr.detach();
    p_ptr->object->erase(key);
}

VJson &VJson::operator[](const std::string &key)
{
    p_ptr.detach();
	return p_ptr->object->operator[](key);
}

const VJson &VJson::operator[](const std::string &key) const
{
	return p_ptr->object->operator[](key);
}

bool VJson::contains(const std::string &key) const
{
	return p_ptr->object->find(key) != p_ptr->object->end();
}

const VJson &VJson::value(const std::string &key) const
{
	return p_ptr->object->operator[](key);
}

const VJson &VJson::value(const std::string &key, const VJson &defaultValue) const
{
	if (contains(key))
		return value(key);
	return defaultValue;
}

size_t VJson::size() const
{
	if (isArray())
		return p_ptr->array->size();
	else if (isObject())
		return p_ptr->object->size();
	return 0;
}

void VJson::clear()
{
    if (p_ptr->type == Array) {
        p_ptr.reset();
        p_ptr->type = Array;
        p_ptr->array = new std::vector<VJson>();
    } else if (p_ptr->type == Object) {
        p_ptr.reset();
        p_ptr->type = Object;
        p_ptr->object = new JsonObject();
    }
}

VString &VJson::string()
{
	return *(p_ptr->str);
}

std::vector<VJson> &VJson::array()
{
	return *(p_ptr->array);
}

JsonObject &VJson::object()
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

std::istream &operator>>(std::istream &in, VJson &value)
{
    if (value.type() != VJson::None)
        return in;

    if (json_try_read(in, "null")) {
        value.p_ptr->type = VJson::Null;
    } else if (json_try_read(in, "false")) {
        value.p_ptr->type = VJson::Boolean;
        value.p_ptr->boolean = false;
    } else if (json_try_read(in, "true")) {
        value.p_ptr->type = VJson::Boolean;
        value.p_ptr->boolean = true;
    } else if (json_try_read(in, '"')) {
        value.p_ptr->type = VJson::String;
        std::string str;
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
        value.p_ptr->str = new VString(VString::fromUtf8(str));
    } else {
        char ch;
        in.get(ch);
        if (ch == '-' || (ch >= '0' && ch <= '9') || ch == '.') {
            in.unget();
            value.p_ptr->type = VJson::Number;
            in >> value.p_ptr->number;
        } else if (ch == '[') {
            value.p_ptr->type = VJson::Array;
            value.p_ptr->array = new std::vector<VJson>();
            std::vector<VJson> &array = (*value.p_ptr->array);
            while (true) {
                VJson element;
                in >> element;
                if (element.type() == VJson::None) {
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
            value.p_ptr->type = VJson::Object;
            value.p_ptr->object = new JsonObject();

            JsonObject &object = *(value.p_ptr->object);
            while (true) {
                VJson key;
                in >> key;
                if (key.type() != VJson::String) {
                    std::cerr << "Expect JSON key string." << std::endl;
                    break;
                }
                if (!json_try_read(in, ':')) {
                    std::cerr << "Expect :" << std::endl;
                    break;
                }

                VJson value;
                in >> value;
                if (value.type() == VJson::None) {
                    std::cerr << "Invalid value of JSON object" << std::endl;
                    break;
                }

                object[key.string().toUtf8()] = value;

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

std::ostream &operator<<(std::ostream &out, const VJson &value)
{
    switch (value.type()) {
    case VJson::Null:
        out << "null";
        break;
    case VJson::Boolean:
        if (value.toBool())
            out << "true";
        else
            out << "false";
        break;
    case VJson::Number:
        out << value.toDouble();
        break;
    case VJson::String:
        out << '"' << value.toString() << '"';
        break;
    case VJson::Array:{
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
    case VJson::Object:{
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

VJson VJson::Parse(const char* str)
{
	std::stringstream s;
	s << str;
    VJson json;
	s >> json;
	return json;
}

VJson VJson::Load(const char *path)
{
	std::ifstream s(path, std::ios::binary);
    VJson json;
	s >> json;
	return json;
}

void JsonData::cloneData(const JsonData &source)
{
    type = source.type;
    switch (type) {
    case VJson::Boolean:
        boolean = source.boolean;
        break;
    case VJson::Number:
        number = source.number;
        break;
    case VJson::String:
        str = new VString(*source.str);
        break;
    case VJson::Array:
        array = new std::vector<VJson>(*source.array);
        break;
    case VJson::Object:
        object = new JsonObject(*source.object);
        break;
    default:
    	break;
    }
}

NV_NAMESPACE_END
