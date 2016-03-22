/*
 * JNI_Tests.cpp
 *
 *  Created on: 2016年3月9日
 *      Author: yangkai
 */

#include "test.h"

#include <VAtomicInt.h>

#include <thread>
#include <vector>

using namespace std;
NV_USING_NAMESPACE

namespace {

VAtomicInt ShareData(0);

void thread0(int)
{
    ShareData.exchangeAddAcquire(1);
    while (10 != ShareData.load()) { }
    assert(10 == ShareData.load());
    ShareData /= 2;
    while (20 != ShareData.load()) { }
    assert(20 == ShareData.load());
}

void thread1(int)
{
    while (1 != ShareData.load()) { }
    assert(1 == ShareData.load());
    ShareData.incrementSync();
    while (5 != ShareData.load()) { }
    assert(5 == ShareData.load());
    ShareData <<= 3;
}

void thread2(int)
{
    while(2 != ShareData.load()) { }
    assert(2 == ShareData.load());
    ShareData *= 5;
    while (40 != ShareData.load()) { }
    assert(40 == ShareData.load());
    ShareData >>= 1;
}

void test()
{
    vector<thread *> pool;
    pool.push_back(new thread(thread0, 0));
    pool.push_back(new thread(thread1, 1));
    pool.push_back(new thread(thread2, 2));
    for (thread *e : pool) {
        e->join();
    }

    for (thread *e : pool) {
        delete e;
    }
}

ADD_TEST(VAtomicInt, test)

} //namespace
