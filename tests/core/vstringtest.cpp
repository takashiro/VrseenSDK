#include "test.h"

#include <VString.h>

NV_USING_NAMESPACE

namespace {

void test()
{
    //Empty String
    {
        VString null;
        assert(null.isEmpty());

        VString empty("");
        assert(empty.isEmpty());
    }

    //Converted from a C-style string
    {
        const char *str = "this is a test.";
        VString test(str);
        assert(test == str);
        assert(test.size() == strlen(str));
    }

    //append: Append to a null string
    {
        VString test;
        test.append("haha");
        assert(test.length() == 4);
        assert(test == "haha");
    }

    //append: Append to a string
    {
        VString test("this ");
        assert(test == "this ");
        test.append("is ");
        assert(test == "this is ");
        test.append("a test.");
        assert(test == "this is a test.");
    }

    //reserve: Reserve more spaces
    {
        VString test("test");
        assert(test == "test");
        test.reserve(50);
        assert(test == "test");
        assert(test.size() == 4);
        assert(50 <= test.capacity());
        const void *ptr1 = test.data();
        test.append("test");
        const void *ptr2 = test.data();
        assert(test == "testtest");
        assert(ptr1 == ptr2);
    }

    //remove: remove characters from a string
    {
        const char *str = "this is a stupid test.";
        VString test(str);
        test.remove(10, 7);
        assert(test == "this is a test.");
        test.remove(0, 4);
        assert(test == " is a test.");
        test.remove(5, 100);
        assert(test == " is a");
        test.remove(0, 100);
        assert(test == "");
        assert(test.isEmpty());
    }

    //clear: Clear up a string
    {
        VString test("this is a test");
        test.clear();
        assert(test.isEmpty());
    }

    //assign: Redefine a string
    {
        const char *str1 = "this is another test";
        const char *str2 = "this is an example";
        VString test("this is a test");
        test = str1;
        assert(test == str1);
        test.assign(str2);
        assert(test == str2);

        const char *str3 = "efigs.fnt";
        VString str;
        str = str3;
        assert(str == str3);
        str.insert("res/raw/", 0);
        assert(str == "res/raw/efigs.fnt");
    }

    //Access via index
    {
        const char *str = "this is another test";
        VString test(str);
        for (int i = 0, max = test.length(); i < max; i++) {
            assert(test.at(i) == str[i]);
            test[i] = 'A';
        }
        assert(test == "AAAAAAAAAAAAAAAAAAAA");
    }

    //Sustring
    {
        VString test("this is a test");
        assert(test.left(4) == "this");
        assert(test.right(4) == "test");
        assert(test.mid(5, 2) == "is");
        assert(test.range(5, 7) == "is");
    }

    //Lower and upper case conversion
    {
        VString upper;
        upper.reserve(26);
        for (char ch = 'A'; ch <= 'Z'; ch++) {
            upper.append(ch);
        }
        VString lower;
        lower.reserve(26);
        for (char ch = 'a'; ch <= 'z'; ch++) {
            lower.append(ch);
        }
        assert(upper.toLower() == lower);
        assert(upper.toUpper() == upper);
        assert(lower.toUpper() == upper);
        assert(lower.toLower() == lower);

        VString str1("this is 512");
        assert(str1.toUpper() == "THIS IS 512");

        VString str2("ThaT Is !@#");
        assert(str2.toLower() == "that is !@#");
    }

    //insert
    {
        VString str1("this is test");
        uint size1 = str1.size();
        str1.insert(" a", 7);
        assert(size1 + 2 == str1.size());
        std::cout << str1.toStdString() << std::endl;
        assert(str1 == "this is a test");

        VString str2("test");
        str2.insert("this is a ", 0);
        assert(str2 == "this is a test");
    }

    //Concat
    {
        VString str1("you");
        VString str2 = "help";
        assert(str1 + str2 == "youhelp");

        const char *str3 = "do";
        assert(str3 + str1 == "doyou");
        assert(str1 + str3 == "youdo");
    }

    //Convert numbers
    for (int i = 0; i < 100; i++) {
        int num1 = rand();
        VString numString = VString::number(num1);
        int num2 = numString.toInt();
        assert(num1 == num2);
    }

    //Converted to C String
    {
        const VString &str("res/raw/efigs_sdf.ktx");
        assert(strcmp(str.toCString(), "res/raw/efigs_sdf.ktx") == 0);
    }
}

ADD_TEST(VString, test)

}
