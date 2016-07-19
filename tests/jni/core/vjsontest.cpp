#include "test.h"

#include <fstream>
#include <sstream>

#include <VJson.h>

using namespace std;
NV_USING_NAMESPACE

namespace {

inline double abs(double num)
{
    return num >= 0 ? num : -num;
}

void test()
{
    {
        stringstream s;
        s << "null";
        VJson json;
        s >> json;
        assert(json.isNull());
    }

    {
        stringstream s;
        s << " null ";
        VJson json;
        s >> json;
        assert(json.isNull());
    }

    {
        stringstream s;
        s << "true";
        VJson json;
        s >> json;
        assert(json.toBool());
    }

    {
        stringstream s;
        s << "false";
        VJson json;
        s >> json;
        assert(!json.toBool());
    }

    {
        VJson num1(526);
        assert(num1.toInt() == 526);

        double pi = 3.14;
        VJson num2(pi);
        assert(abs(num2.toDouble() - pi) <= 1e-4);
    }

    for (int i = 0; i < 10; i++) {
        stringstream s;
        int value = rand();
        s << value;
        VJson json;
        s >> json;
        assert(json.toInt() == value);
    }

    for (int i = 0; i < 10; i++) {
        stringstream s;
        double value = rand() / (double) rand();
        s << value;
        VJson json;
        s >> json;
        double result = json.toDouble();
        assert(abs(result - value) <= 0.01);
    }

    {
        stringstream s;
        s << u8"{\"test\" : [1, 2, \"いつも好きです\"]}";
        VJson json;
        s >> json;
        assert(json.contains("test"));
        assert(!json.contains("what"));

        VJson test = json.value("test");
        assert(test.isArray());
        assert(test[0].toInt() == 1);
        assert(test[1].toInt() == 2);
        assert(test[2].toString() == u"いつも好きです");

        stringstream out;
        out << json;
        assert(out.str() == u8"{\"test\" : [1, 2, \"いつも好きです\"]}");
    }

    {
        stringstream s;
        s << "[\"\"]";
        VJson json;
        s >> json;
        assert(json.isArray());
        assert(json.size() == 1);
        assert(json.at(0).isString());
        assert(json.at(0).size() == 0);
    }

    {
        stringstream s;
        s << "[]";
        VJson json;
        s >> json;
        assert(json.isArray());
        assert(json.size() == 0);
    }

    {
        stringstream s;
        s << "{}";
        VJson json;
        s >> json;
        assert(json.isObject());
        assert(json.size() == 0);
    }
}

ADD_TEST(VJson, test)

}
