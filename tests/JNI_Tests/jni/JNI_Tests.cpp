/*
 * JNI_Tests.cpp
 *
 *  Created on: 2016年3月9日
 *      Author: yangkai
 */
#include <iostream>
#include <string>
#include <vector>
#include <list>
#include <thread>

#include "com_example_jni_tests_MainActivity.h"
#include "VList.h"
#include "logcat_cpp.h"
#include "VAtomicInt.h"

#define LOG_TAG "JNI_Tests"
#define LOGD(...) _LOGD(LOG_TAG, __VA_ARGS__)
using namespace std;
using namespace NervGear;
VAtomicInt ShareData(0);
static int count = 0;
class StrNode:public NodeOfVList<VList<StrNode*>>
{
public:
    string* pstr;//需要自己释放
    StrNode(string *p):pstr(p)
    {

    }
    bool operator == (const StrNode& a)
    {
        return *(a.pstr) == *(this->pstr);
    }
};
void testVList()
{
    VList<StrNode*> tester;
    StrNode* pStr = nullptr, *tmp = nullptr;
    int count = 0;
    LOGD("%d.isEmpty():1 == %d\n", count++, tester.isEmpty());
    pStr = new StrNode(new string("tester2"));
    pStr->pointToVList = &tester;
    tester.append(pStr);

    pStr = new StrNode(new string("tester3"));
    pStr->pointToVList = &tester;
    tester.append(pStr);

    pStr = new StrNode(new string("tester1"));
    pStr->pointToVList = &tester;
    tester.prepend(pStr);
    tmp = pStr;

    pStr = new StrNode(new string("tester0"));
    pStr->pointToVList = &tester;
    tester.prepend(pStr);

    tmp = tester.getNextByContent(tmp);
    LOGD("%d.getNextByContent():tester2 == %s", count++, tmp->pstr->c_str());
    LOGD("%d.isEmpty():0 == %d\n", count++, tester.isEmpty());

    LOGD("%d.size():4 == %d", count++, tester.size());

    LOGD("%d.first():tester0 == %s\n", count++, tester.first()->pstr->c_str());
    LOGD("%d.last():tester3 == %s\n", count++, tester.last()->pstr->c_str());

    VList<StrNode*>::iterator iter0 = tester.end();
    iter0--;

    tester.bringToFront(iter0);
    LOGD("%d.bringToFront():tester3 == %s\n", count++, tester.first()->pstr->c_str());

    tester.sendToBack(tester.begin());
    LOGD("%d.sendToBack():tester3 == %s\n", count++, tester.last()->pstr->c_str());

    LOGD("%d.contains():0 == %d\n", count++, tester.contains(new StrNode(new string("tester2"))));
    LOGD("%d.contains():1 == %d\n", count++, tester.contains(tester.front()));

    StrNode* p = tester.front();
    StrNode* &x=p;
    p->pointToVList->remove(p);
    LOGD("%d.remove():tester1 == %s\n", count++, tester.front()->pstr->c_str());

    tester.back()->pointToVList->remove(tester.back());
    LOGD("%d.remove():tester2 == %s\n", count++, tester.back()->pstr->c_str());

    LOGD("%d.size():2 == %d\n", count++, tester.size());
}

void thread0(int id)
{
    ShareData.exchangeAddAcquire(1);
    while (10 != ShareData.load()) {
        LOGD("Thread:%d", id);
    }
    LOGD("%d.thread(%d)$operator *= : 10 == %d", count++, id, ShareData.load());
    ShareData /= 2;
    while (20 != ShareData.load()) {
        LOGD("Thread:%d", id);
    }
    LOGD("%d.thread(%d)$operator >>= : 20 == %d", count++, id, ShareData.load());
}
void thread1(int id)
{
    while (1 != ShareData.load()) {
        LOGD("Thread:%d", id);
    }
    LOGD("%d.thread(%d)$exchangeAddAcquire():1 == %d", count++, id, ShareData.load());
    ShareData.incrementSync();
    while (5 != ShareData.load()) {
        LOGD("Thread:%d", id);
    }
    LOGD("%d.thread(%d)$operator /= : 5 == %d", count++, id, ShareData.load());
    ShareData <<= 3;
}
void thread2(int id)
{
    while(2 != ShareData.load()) {
        LOGD("Thread:%d", id);
    }
    LOGD("%d.thread(%d)$incrementSync():2 == %d", count++, id, ShareData.load());
    ShareData *= 5;
    while (40 != ShareData.load()) {
        LOGD("Thread:%d", id);
    }
    LOGD("%d.thread(%d)$operator <<= : 40 == %d", count++, id, ShareData.load());
    ShareData >>= 1;
}
void testVAtomicInt()
{
    vector<thread> pool;
    pool.push_back(thread(thread0, 0));
    pool.push_back(thread(thread1, 1));
    pool.push_back(thread(thread2, 2));
    for (auto &e : pool) {
        e.join();
    }
    LOGD("count:6 == %d", count);
}
JNIEXPORT jstring JNICALL Java_com_example_jni_1tests_MainActivity_runTests
  (JNIEnv *env, jobject obj)
{
    testVAtomicInt();
    return env->NewStringUTF("It works!");
}


