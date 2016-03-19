#include <iostream>
#include <string>
#include <vector>
#include <list>
#include <thread>

#include "com_example_jni_tests_MainActivity.h"
#include "logcat_cpp.h"
#include "VAtomicInt.h"

#define LOG_TAG "JNI_Tests"
#define LOGD(...) _LOGD(LOG_TAG, __VA_ARGS__)
using namespace std;
using namespace NervGear;

VAtomicInt ShareData(0);

void thread0(int id)
{
    ShareData.exchangeAddAcquire(1);
}
void thread1(int id)
{
    while(1 != ShareData.load())
    {

    }
    LOGD("thread(%d)$exchangeAddAcquire():1 == %d", id, ShareData.load());
    ShareData.incrementSync();
}
void thread2(int id)
{
    while(2 != ShareData.load())
    {

    }
    LOGD("thread(%d)$incrementSync():2 == %d", id, ShareData.load());
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
}
JNIEXPORT jstring JNICALL Java_com_example_jni_1tests_MainActivity_runTests
  (JNIEnv *env, jobject obj)
{
    testVAtomicInt();
    return env->NewStringUTF("It works!");
}


