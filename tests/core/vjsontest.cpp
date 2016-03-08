#include "test.h"

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
        Json json;
        s >> json;
        assert(json.isNull());
    }

    {
        stringstream s;
        s << " null ";
        Json json;
        s >> json;
        assert(json.isNull());
    }

    {
        stringstream s;
        s << "true";
        Json json;
        s >> json;
        assert(json.toBool());
    }

    {
        stringstream s;
        s << "false";
        Json json;
        s >> json;
        assert(!json.toBool());
    }

    {
        Json num1(526);
        assert(num1.toInt() == 526);

        double pi = 3.14;
        Json num2(pi);
        assert(abs(num2.toDouble() - pi) <= 1e-4);
    }

    for (int i = 0; i < 10; i++) {
        stringstream s;
        int value = rand();
        s << value;
        Json json;
        s >> json;
        assert(json.toInt() == value);
    }

    for (int i = 0; i < 10; i++) {
        stringstream s;
        double value = rand() / (double) rand();
        s << value;
        Json json;
        s >> json;
        assert(abs(json.toDouble() - value) <= 1e-4);
    }

    {
        stringstream s;
        s << "{\"test\" : [1,2,3]}";
        Json json;
        s >> json;
        assert(json.contains("test"));
        assert(!json.contains("what"));

        Json test = json.value("test");
        assert(test.isArray());
        assert(test[0].toInt() == 1);
        assert(test[1].toInt() == 2);
        assert(test[2].toInt() == 3);
    }
}

ADD_TEST(VJson, test)

}
