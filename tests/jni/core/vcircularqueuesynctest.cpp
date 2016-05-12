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
#define LOOPMAX 10000
NV_USING_NAMESPACE

namespace {
void test()
{
    //logic
    {
        VCircularQueueSync<int> vqr(3);
        vqr.append(1);
        vqr.prepend(0);
        vqr.append(2);
        for (int i = 0;i < vqr.size();i++) {
            vInfo("real: " << vqr.get(i) << " except: " << i);
        }
        vqr.append(3);
        vqr.prepend(0);
        for (int i = 0;i < vqr.size();i++) {
            vInfo("real: " << vqr.get(i) << " except: " << i);
        }
    }

    //performance
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
}

ADD_TEST(VCircularQueueSync, test)
}



