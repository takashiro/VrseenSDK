#include "test.h"

#include <VMutex.h>
#include <VSemaphore.h>
#include <thread>

NV_USING_NAMESPACE

namespace {

void test()
{
    //Non-recursive mutex
    {
        VMutex mutex;
        VSemaphore sem;
        int count = 0;
        int times = 0xFFFFFF;

        auto task = [&]{
            for (int i = 0; i < times; i++) {
                mutex.lock();
                count++;
                mutex.unlock();
            }
            sem.post();
        };

        std::thread worker1(task);
        std::thread worker2(task);

        worker1.detach();
        worker2.detach();

        sem.wait();
        sem.wait();

        assert(count == times * 2);
    }

    //Recursive mutex
    {
        VMutex mutex(true);
        assert(mutex.isRecursive());

        mutex.lock();
        assert(mutex.tryLock());
        mutex.unlock();
    }
}

ADD_TEST(VMutex, test)

}
