#include "test.h"
#include "VCircularQueueSync.h"
#include "VCircularQueue.h"

#include <time.h>

#define LOOPMAX 10000000

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
        assert(vqr.at(0) == 0);
        assert(vqr.at(1) == 1);
        assert(vqr.at(2) == 2);

        vqr.append(3);
        assert(vqr.at(0) == 1);
        assert(vqr.at(1) == 2);
        assert(vqr.at(2) == 3);

        vqr.prepend(0);
        assert(vqr.at(0) == 0);
        assert(vqr.at(1) == 1);
        assert(vqr.at(2) == 2);

        vqr.clear();

        vqr.append(1);
        vqr.prepend(0);
        vqr.append(2);
        assert(vqr.at(0) == 0);
        assert(vqr.at(1) == 1);
        assert(vqr.at(2) == 2);


        vqr.append(3);
        assert(vqr.at(0) == 1);
        assert(vqr.at(1) == 2);
        assert(vqr.at(2) == 3);

        vqr.prepend(0);
        assert(vqr.at(0) == 0);
        assert(vqr.at(1) == 1);
        assert(vqr.at(2) == 2);
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

        vq.clear();
        start = clock();
        for (int i = 0;i < LOOPMAX;i++) {
            vq.prepend(1);
        }
        vInfo("VCircularQueue::prepend  loop:" << LOOPMAX
                << " time:" << clock()-start);

        vqs.clear();
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



