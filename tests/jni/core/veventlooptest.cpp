#include "test.h"

#include <VEventLoop.h>
#include <VSemaphore.h>

#include <thread>

NV_USING_NAMESPACE

namespace {

void test()
{
    {
        VEventLoop loop(100);
        loop.post("test");

        VEvent event = loop.next();
        assert(event.name == "test");
    }

    {
        VEventLoop loop(100);
        std::thread listener([&]{
            loop.wait();
            VEvent event = loop.next();
            assert(event.name == "yunzhe");
        });
        loop.post("yunzhe");
        listener.join();
    }

    {
        VSemaphore release;
        VEventLoop loop(100);
        volatile bool finished = false;
        std::thread receiver([&]{
            loop.wait();
            finished = true;
            VEvent event = loop.next();
            assert(event.name == "yunzhe");
            release.post();
        });
        receiver.detach();
        loop.send("yunzhe");
        assert(finished);
        release.wait();
    }

    {
        VSemaphore release;
        VEventLoop loop(100);
        volatile int result = 0;
        std::thread worker([&]{
            forever {
                loop.wait();
                VEvent event = loop.next();
                if (event.name == "quit") {
                    release.post();
                    break;
                } else if (event.name == "plus") {
                    result++;
                }
            }
        });
        worker.detach();

        for (int i = 0; i < 10; i++) {
            loop.post("plus");
        }
        loop.send("quit");
        assert(result == 10);
        release.wait();
    }
}

ADD_TEST(VEventLoop, test)

}
