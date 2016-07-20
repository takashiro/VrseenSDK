#include "test.h"

#include <thread>
#include <VSemaphore.h>

NV_USING_NAMESPACE

namespace {

void test()
{
    int result = 0;
    VSemaphore mutex(1);
    VSemaphore finished(0);

    auto task = [&](){
        for (int i = 0; i < 100; i++) {
            mutex.wait();
            int tmp = result;
            tmp++;
            result = tmp;
            mutex.post();
        }
        finished.post();
    };

    std::thread worker1(task);
    std::thread worker2(task);
    worker1.detach();
    worker2.detach();

    finished.wait();
    finished.wait();

    assert(result == 200);
}

ADD_TEST(VSemaphore, test)

}
