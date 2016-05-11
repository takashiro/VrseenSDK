/*
 * vcircularqueuesync.cpp
 *
 *  Created on: 2016年5月11日
 *      Author: yangkai
 */
#include "test.h"
#include "core/VCircularQueueSync.h"
#include "core/VCircularQueue.h"
#include <ctime>
#define LOOPMAX 1000
NV_USING_NAMESPACE

namespace {
void test()
{
    VCircularQueue<int> vq;
    VCircularQueueSync<int> vqs;
    int start = clock();
    for (int i = 0;i < LOOPMAX;i++) {
        vq.append(1);
    }
    vInfo("VCircularQueue::append  loop:" << LOOPMAX
            << " time:" << clock()-start);

    start = clock();
    for (int i = 0;i < LOOPMAX;i++) {
        vqs.append(1);
    }
    vInfo("VCircularQueueSync::append  loop:" << LOOPMAX
            << " time:" << clock()-start);

    start = clock();
    for (int i = 0;i < LOOPMAX;i++) {
        vq.prepend(1);
    }
    vInfo("VCircularQueue::prepend  loop:" << LOOPMAX
            << " time:" << clock()-start);

    start = clock();
    for (int i = 0;i < LOOPMAX;i++) {
        vqs.prepend(1);
    }
    vInfo("VCircularQueueSync::prepend  loop:" << LOOPMAX
            << " time:" << clock()-start);
}

ADD_TEST(VCircularQueueSync, test)
}



