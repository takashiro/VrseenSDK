#include "test.h"
#include "VCircularQueue.h"

#include <time.h>
#include <deque>

NV_USING_NAMESPACE

namespace {

void test()
{
    //logic
    {
        VCircularQueue<int> vqr(3);
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


    constexpr int loopMax = 1000000;
    //performance
    {
        std::deque<int> q1;
        VCircularQueue<int> q2;

        int timestamp = clock();
        for (int i = 0; i < loopMax; i++) {
            q1.push_back(1);
            if (q1.size() > 500) {
                q1.pop_front();
            }
        }
        int t1 = clock() - timestamp;

        timestamp = clock();
        for (int i = 0; i < loopMax; i++) {
            q2.append(1);
        }
        int t2 = clock() - timestamp;

        assert(t2 < t1);
    }

    {
        int timestamp = clock();
        std::deque<int> q1;
        for (int i = 0; i < loopMax; i++) {
            q1.push_front(1);
        }
        int t1 = clock() - timestamp;

        timestamp = clock();
        VCircularQueue<int> q2;
        for (int i = 0; i < loopMax; i++) {
            q2.prepend(1);
        }
        int t2 = clock() - timestamp;

        assert(t2 < t1);
    }
}

ADD_TEST(VCircularQueue, test)
}



