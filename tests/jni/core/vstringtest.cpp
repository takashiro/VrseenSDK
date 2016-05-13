#include "test.h"

#include <VString.h>
#include <string>

NV_USING_NAMESPACE

namespace {

inline double abs(double num) {
    return num >= 0 ? num : -num;
}

void test()
{
    //Empty String
    {
        VString null;
        assert(null.isEmpty());

        VString empty("");
        assert(empty.isEmpty());
    }

    //Copy constructor & copy assignement operator
    {
        VString str1 = "this is a test.";
        VString str2 = str1;
        VString str3 = "this is another test.";
        str3 = str1;
        assert(str1 == str2);
        assert(str1.data() != str2.data());
        assert(str1.data() != str3.data());
    }

    //Move constructor & move assignment operator
    {
        VString str1;
        str1 = "this is a test.";
        const void *addr = str1.data();
        VString str2(std::move(str1));
        assert(str2.data() == addr);
        assert(str2 == "this is a test.");
        VString str3;
        str3 = std::move(str2);
        assert(str3.data() == addr);
        assert(str3 == "this is a test.");
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
        str.insert(0, "res/raw/");
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
        str1.insert(7, " a");
        assert(size1 + 2 == str1.size());
        assert(str1 == "this is a test");

        VString str2("test");
        str2.insert(0, "this is a ");
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

    {
        //contains, startsWith, endsWith
        VString str("this is a test.");
        assert(str.contains("is"));
        assert(str.contains('.'));

        assert(str.startsWith('t'));
        assert(str.startsWith("t"));
        assert(str.startsWith("this"));

        assert(str.endsWith('.'));
        assert(str.endsWith("."));
        assert(str.endsWith("test."));

        //replace
        str.replace(' ', '_');
        assert(str == "this_is_a_test.");
    }

    //Double conversion
    {
        double pi = 3.1415;
        VString piStr = VString::number(pi);
        assert(piStr == "3.1415");
        double pi2 = piStr.toDouble();
        assert(abs(pi - pi2) <= 1e-4);
    }

    //Format input
    {
        VString str;
        str.sprintf("%d", 100);
        assert(str == "100");
    }
    {
        VString str;
        str.sprintf("vid=%04hx:pid=%04hx:ser=%s", 123, 456, "NervGear");
        assert(str == "vid=007b:pid=01c8:ser=NervGear");
    }
    {
        VString str;
        str.sprintf("%d %lf", 10, 10.10);
        str.sprintf("%d %g", 5, 26.1010);
        assert(str == "10 10.1000005 26.101");
    }

    //UTF-8/UTF-16 Conversion
    {
        const char16_t *rawUtf16 = u"私はあなただけをずっと見つめている";
        const char *rawUTf8 = "私はあなただけをずっと見つめている";
        VString utf16(rawUtf16);
        VByteArray utf8 = utf16.toUtf8();
        assert(utf8 == rawUTf8);

        VString newUtf16 = VString::fromUtf8(utf8);
        assert(utf16 == newUtf16);
    }

    //Latin1/UTF-16 Conversion
    {
        const char16_t *utf16 = u"You are the only one in my eyes";
        const char *latin1 = "You are the only one in my eyes";

        VString str1(utf16);
        assert(str1 == latin1);

        VByteArray str2 = str1.toLatin1();
        assert(str2 == latin1);

        VString str3 = VString::fromLatin1(str2);
        assert(str1 == str3);
    }

    //UTF-32/UTF-16 Conversion
    {
        std::u32string utf32 = U"向日葵的约定";
        VString str = VString::fromUcs4(utf32);
        assert(str == u"向日葵的约定");
    }

    //Case-insensitive compare
    {
        VString str1 = "this is a test.";
        VString str2 = "THIS IS A TEST.";
        assert(str1.icompare(str2) == 0);
        assert(str1.startsWith("ThIs", false));
        assert(str1.endsWith("TeSt.", false));
    }
}

ADD_TEST(VString, test)

}
