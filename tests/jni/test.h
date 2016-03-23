#pragma once

#include <VLog.h>
#ifdef ANDROID
#define assert(expression) vAssert(expression)
#else
#include <assert.h>
#undef vInfo
#define vInfo(args) cout << args << endl
#endif

#include <stdlib.h>

#include <list>
#include <iostream>

struct TestUnit
{
    typedef void (*Function)();
    std::string name;
    Function function;

    TestUnit(const std::string &name, Function function)
    {
        this->name = name;
        this->function = function;
    }
};

std::list<TestUnit> &Tests();

#define ADD_TEST(name, test) namespace {\
    struct TestAdder\
    {\
        TestAdder()\
        {\
            Tests().push_back(TestUnit(#name, test));\
        }\
    };\
    TestAdder adder;\
}
