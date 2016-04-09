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
        uchar bytes[10];
        VVariant var(bytes);
        assert(var.isPointer());
        assert(var.toPointer() == bytes);
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
        uint test[3] = {1994, 10, 10};
        const void *pointer = test;
        VVariantArray array;
        array << 1 << 2 << 3 << "this is a test" << pointer;

        VVariant var(array);
        assert(var.length() == array.length());
        assert(var.at(0).toInt() == 1);
        assert(var.at(1).toInt() == 2);
        assert(var.at(2).toInt() == 3);
        assert(var.at(3).toString() == "this is a test");
        assert(var.at(4).toPointer() == pointer);

        int length = array.length();
        VVariant moved(std::move(array));
        assert(moved.length() != array.length());
        assert(moved.length() == length);
        assert(moved.at(0).toInt() == 1);
        assert(moved.at(1).toInt() == 2);
        assert(moved.at(2).toInt() == 3);
        assert(moved.at(3).toString() == "this is a test");
        assert(moved.at(4).toPointer() == pointer);

        uint *test2 = static_cast<uint *>(moved.at(4).toPointer());
        assert(test2[0] == 1994);
        assert(test2[1] == 10);
        assert(test2[2] == 10);

        VVariant copy(var);
        assert(copy.length() == var.length());
        assert(copy.at(0).toInt() == 1);
        assert(copy.at(1).toInt() == 2);
        assert(copy.at(2).toInt() == 3);
        assert(copy.at(3).toString() == "this is a test");
        assert(copy.at(4).toPointer() == pointer);
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
