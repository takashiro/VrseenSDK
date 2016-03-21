/*
 * JNI_Tests.cpp
 *
 *  Created on: 2016年3月9日
 *      Author: yangkai
 */
#include <iostream>
#include <string>
#include <list>
#include "com_example_jni_tests_MainActivity.h"
#include "VList.h"
#include "logcat_cpp.h"

#define LOG_TAG "JNI_Tests"
#define LOGD(...) _LOGD(LOG_TAG, __VA_ARGS__)
using namespace std;
using namespace NervGear;

class StrNode:public NodeOfVList<VList<StrNode>>
{
public:
    string* pstr;
    StrNode(string *p):pstr(p)
    {

    }
};
void testVList()
{
    list<int> yy;
    for (int e:yy) {
        cout << e;
    }
    VList<StrNode> tester;
    string* p = new string("Hi");
    tester.append(StrNode(p));
    LOGD("%s\n",tester.first().pstr->c_str());//Hi
    tester.first().removeNodeFromVList();
    LOGD("size:%d\n",tester.size());//0
    p = new string("world!");
    tester.prepend(StrNode(p));
    LOGD("%s\n",tester.last().pstr->c_str());//world
    p = new string("my ");
    tester.prepend(StrNode(p));
    LOGD("%s\n",tester.last().pstr->c_str());//my
    tester.clear();//有问题
    LOGD("size:%d\n",tester.size());//0
}
JNIEXPORT jstring JNICALL Java_com_example_jni_1tests_MainActivity_runTests
  (JNIEnv *env, jobject obj)
{
    testVList();
    return env->NewStringUTF("It works!");
}


