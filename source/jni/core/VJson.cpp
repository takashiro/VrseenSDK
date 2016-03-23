#include "VJson.h"

#include "VByteArray.h"
#include "VLog.h"

#include <sstream>
#include <fstream>

NV_NAMESPACE_BEGIN

VJson::VJson()
    : m_type(Null)
{
}

VJson::VJson(bool value)
    : m_type(Boolean)
{
    m_value.boolean = value;
}

VJson::VJson(int value)
    : m_type(Number)
{
    m_value.number = value;
}

VJson::VJson(double value)
    : m_type(Number)
{
    m_value.number = value;
}

VJson::VJson(const char *value)
{
    m_type = String;
    m_value.str = new VString(value);
}

VJson::VJson(const std::string &value)
    : m_type(String)
{
    m_value.str = new VString(value);
}

VJson::VJson(const VString &value)
    : m_type(String)
{
    m_value.str = new VString(value);
}

VJson::VJson(VString &&value)
    : m_type(String)
{
    m_value.str = new VString(value);
}

VJson::VJson(const VJsonArray &array)
    : m_type(Array)
{
    m_value.array = new VJsonArray(array);
}

VJson::VJson(VJsonArray &&array)
    : m_type(Array)
{
    m_value.array = new VJsonArray(array);
}

VJson::VJson(const VJsonObject &object)
    : m_type(Object)
{
    m_value.object = new VJsonObject(object);
}

VJson::VJson(VJsonObject &&object)
    : m_type(Object)
{
    m_value.object = new VJsonObject(object);
}

VJson::VJson(const VJson &source)
{
    copy(source);
}

VJson::VJson(VJson &&source)
    : m_type(source.m_type)
    , m_value(source.m_value)
{
    source.m_type = Null;
}

VJson::~VJson()
{
    release();
}

bool VJson::toBool() const
{
    switch (m_type) {
	case Boolean:
        return m_value.boolean;
	case Number:
        return (int) m_value.number != 0;
	case String:
        return !m_value.str->isEmpty();
	case Array:
        return m_value.array->size() > 0;
	case Object:
        return m_value.object->size() > 0;
	default:
		return false;
	}
}

double VJson::toDouble() const
{
    if (m_type == Number)
        return m_value.number;

    if (m_type == String) {
		std::stringstream s;
        s << m_value.str->toLatin1();
		double number;
		s >> number;
		return number;
	}

	return 0.0;
}

int VJson::toInt() const
{
    if (m_type == Number)
        return static_cast<int>(m_value.number);

    if (m_type == String) {
		std::stringstream s;
        s << m_value.str->toLatin1();
		int number;
		s >> number;
		return number;
	}

    return 0;
}

VString VJson::toString() const
{
    if (m_type == String)
        return *(m_value.str);

    if (m_type == Number) {
        std::stringstream s;
        s << m_value.number;
        std::string str;
        s >> str;
        return str;
    }

    return VString();
}

std::string VJson::toStdString() const
{
    if (m_type == String)
        return m_value.str->toLatin1();

    if (m_type == Number) {
		std::stringstream s;
        s << m_value.number;
		std::string str;
		s >> str;
		return str;
	}

    return std::string();
}

VJsonArray &VJson::array()
{
    vAssert(isArray());
    return *(m_value.array);
}

const VJsonArray &VJson::toArray() const
{
    vAssert(isArray());
    return *(m_value.array);
}

VJsonObject &VJson::object()
{
    vAssert(isObject());
    return *(m_value.object);
}

const VJsonObject &VJson::toObject() const
{
    vAssert(isObject());
    return *(m_value.object);
}

VJson &VJson::operator =(const VJson &source)
{
    release();
    copy(source);
    return *this;
}

VJson &VJson::operator = (VJson &&source)
{
    release();
    m_type = source.m_type;
    m_value = source.m_value;
    source.m_type = Null;
    return *this;
}

const VJson &VJson::value(const VString &key, const VJson &defaultValue) const
{
	if (contains(key))
		return value(key);
	return defaultValue;
}

uint VJson::size() const
{
	if (isArray())
        return m_value.array->size();
	else if (isObject())
        return m_value.object->size();
	return 0;
}

void VJson::clear()
{
    if (m_type == Array) {
        m_value.array->clear();
    } else if (m_type == Object) {
        m_value.object->clear();
    } else if (m_type == String) {
        m_value.str->clear();
    } else if (m_type == Number) {
        m_value.number = 0.0;
    }
}

void VJson::release()
{
    if (m_type == Array) {
        delete m_value.array;
    } else if (m_type == Object) {
        delete m_value.object;
    } else if (m_type == String) {
        delete m_value.str;
    }
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
    if (value.type() != VJson::Null)
        return in;

    if (json_try_read(in, "null")) {
        value.m_type = VJson::Null;
    } else if (json_try_read(in, "false")) {
        value.m_type = VJson::Boolean;
        value.m_value.boolean = false;
    } else if (json_try_read(in, "true")) {
        value.m_type = VJson::Boolean;
        value.m_value.boolean = true;
    } else if (json_try_read(in, '"')) {
        value.m_type = VJson::String;
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
        value.m_value.str = new VString(VString::fromUtf8(str));
    } else {
        char ch;
        in.get(ch);
        if (ch == '-' || (ch >= '0' && ch <= '9') || ch == '.') {
            in.unget();
            value.m_type = VJson::Number;
            in >> value.m_value.number;
        } else if (ch == '[') {
            value.m_type = VJson::Array;
            value.m_value.array = new VJsonArray();
            if (json_try_read(in, ']')) {
                return in;
            }

            VJsonArray &array = (*value.m_value.array);
            while (true) {
                VJson element;
                in >> element;
                array.append(std::move(element));

                if (!json_try_read(in, ',')) {
                    if (!(json_try_read(in, ']'))) {
                        vError("Expect ]");
                    } else {
                        break;
                    }
                }
            }
        } else if (ch == '{') {
            value.m_type = VJson::Object;
            value.m_value.object = new VJsonObject();
            if (json_try_read(in, '}')) {
                return in;
            }

            VJsonObject &object = *(value.m_value.object);
            while (true) {
                VJson key;
                in >> key;
                if (key.type() != VJson::String) {
                    vError("Expect JSON key string.");
                    break;
                }
                if (!json_try_read(in, ':')) {
                    vError("Expect :");
                    break;
                }

                VJson value;
                in >> value;
                object.insert(key.toString(), std::move(value));

                if (!json_try_read(in, ',')) {
                    if (!(json_try_read(in, '}'))) {
                        vError("Expect }");
                    } else {
                        break;
                    }
                }
            }
        } else {
            vError("Unexpected character: " << ch);
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
        const VJsonArray &elements = value.toArray();
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
        const VJsonObject &jsonObject = value.toObject();
        if (!jsonObject.empty()) {
            VJsonObject::const_iterator iter = jsonObject.begin();
            out << '"' << iter->first << "\" : " << iter->second;
            for (iter++; iter != jsonObject.end(); iter++) {
                out << ", \"" << iter->first << "\" : " << iter->second;
            }
        }
        out << '}';
        break;
    }
    default:
        vError("invalid JSON value can't be printed");
        break;
    }
    return out;
}

VJson VJson::Parse(const VByteArray &str)
{
	std::stringstream s;
	s << str;
    VJson json;
	s >> json;
	return json;
}

VJson VJson::Load(const VString &path)
{
    std::ifstream s(path.toUtf8(), std::ios::binary);
    VJson json;
	s >> json;
    return json;
}

void VJson::copy(const VJson &source)
{
    m_type = source.type();
    switch (m_type) {
    case VJson::Boolean:
        m_value.boolean = source.m_value.boolean;
        break;
    case VJson::Number:
        m_value.number = source.m_value.number;
        break;
    case VJson::String:
        m_value.str = new VString(*source.m_value.str);
        break;
    case VJson::Array:
        m_value.array = new VJsonArray(*source.m_value.array);
        break;
    case VJson::Object:
        m_value.object = new VJsonObject(*source.m_value.object);
        break;
    default:
        break;
    }
}

NV_NAMESPACE_END
