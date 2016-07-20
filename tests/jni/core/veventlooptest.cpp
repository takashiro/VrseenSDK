#include "test.h"
#include <VEventLoop.h>
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
        VEventLoop *loop = new VEventLoop(100);
        volatile bool finished = false;
        std::thread receiver([&]{
            loop->wait();
            finished = true;
            VEvent event = loop->next();
            assert(event.name == "yunzhe");
            delete loop;
        });
        receiver.detach();

        loop->send("yunzhe");
        assert(finished);
    }

    {
        VEventLoop *loop = new VEventLoop(100);
        volatile int result = 0;
        std::thread worker([&]{
            forever {
                loop->wait();
                VEvent event = loop->next();
                if (event.name == "quit") {
                    delete loop;
                    break;
                } else if (event.name == "plus") {
                    result++;
                }
            }
        });
        worker.detach();

        for (int i = 0; i < 10; i++) {
            loop->post("plus");
        }
        loop->send("quit");
        assert(result == 10);
    }
}

ADD_TEST(VEventLoop, test)

}
