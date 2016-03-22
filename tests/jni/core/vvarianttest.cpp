#include "test.h"

#include <VVariant.h>

NV_USING_NAMESPACE

namespace {

void test()
{
    {
        VVariant null;
        assert(null.isNull());
    }

    {
        int num = rand();
        VVariant var(num);
        assert(var.isInt());
        assert(var.toInt() == num);

        num = rand();
        var = num;
        assert(var.toInt() == num);
    }

    {
        float num = (float) rand() / rand();
        VVariant var(num);
        assert(var.isFloat());
        assert(var.toFloat() == num);
    }

    {
        double num = (double) rand() / rand();
        VVariant var(num);
        assert(var.isDouble());
        assert(var.toDouble() == num);
    }

    {
        const char *str = "this is a test";
        VVariant var(str);
        assert(var.isString());
        assert(var.toString() == str);
    }

    {
        VString str = "this is another test";
        VVariant var(str);
        assert(var.isString());
        assert(var.toString() == str);
    }

    {
        VString str = "this is a test";
        VString *pointer = &str;
        VVariant var(pointer);
        assert(var.isPointer());
        assert(var.toPointer() == pointer);

        VVariant move(std::move(var));
        assert(move.isPointer());
        assert(move.toPointer() == pointer);
        assert(var.isNull());
    }

    {
        VVariantArray array;
        array << 1 << 2 << 3 << "this is a test";

        VVariant var(array);
        assert(var.length() == array.length());
        assert(var.at(0).toInt() == 1);
        assert(var.at(1).toInt() == 2);
        assert(var.at(2).toInt() == 3);
        assert(var.at(3).toString() == "this is a test");

        VVariant moved(std::move(array));
        assert(moved.length() == array.length());
        assert(moved.at(0).toInt() == 1);
        assert(moved.at(1).toInt() == 2);
        assert(moved.at(2).toInt() == 3);
        assert(moved.at(3).toString() == "this is a test");
    }

    {
        VVariantMap map;
        map["fang"] = 1994;
        map["yun"] = 10;
        map["zhe"] = 10;

        VVariant var(std::move(map));
        assert(var.value("fang").toInt() == 1994);
        assert(var.value("yun").toInt() == var.value("zhe").toInt());
    }
}

ADD_TEST(VVariant, test)

}
