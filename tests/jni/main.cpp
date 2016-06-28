#include "test.h"

#include <time.h>
#include <iostream>

using namespace std;
NV_USING_NAMESPACE

list<TestUnit> &Tests()
{
    static list<TestUnit> tests;
    return tests;
}

int main()
{
    vInfo("NervGear Test");
    vInfo("======================");
    srand(time(nullptr));

    const list<TestUnit> &tests = Tests();
    for (const TestUnit &test : tests) {
        vInfo("Testing " << test.name << " ...");
        (*test.function)();
        vInfo(test.name << " has been tested.");
    }

    vInfo("======================");
    vInfo("Everything works fine!");

    return 0;
}

#ifdef ANDROID

#include <jni.h>

extern "C" {
    jint Java_com_vrseen_unittest_MainActivity_exec(JNIEnv *, jclass, jobject)
    {
        return main();
    }
}

#endif
