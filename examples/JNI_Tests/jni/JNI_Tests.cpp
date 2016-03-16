#include <iostream>
#include <string>
#include <list>
#include "com_example_jni_tests_MainActivity.h"
#include "VArray.h"
#include "MArray.h"
#include "logcat_cpp.h"

#define LOG_TAG "JNI_Tests"
#define LOGD(...) _LOGD(LOG_TAG, __VA_ARGS__)
using namespace std;
using namespace NervGear;

void testVList()
{
    VArray<int> tester;
    Array<int> tester1;
    int count = 0;
    tester.append(2);
    tester.append(3);
    tester.prepend(1);
    tester.prepend(0);
    tester1.append(2);
    tester1.append(3);
//    tester.removeAt(2);
//    tester.removeAtUnordered(2);
    LOGD("%d.first():0 == %d",count++, tester[0]);
    LOGD("%d.first():0 == %d",count++, tester[1]);
    LOGD("%d.first():0 == %d",count++, tester[2]);
}

JNIEXPORT jstring JNICALL Java_com_example_jni_1tests_MainActivity_runTests
  (JNIEnv *env, jobject obj)
{
    testVList();
    return env->NewStringUTF("It works!");
}


