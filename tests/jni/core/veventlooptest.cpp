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
        std::thread sender([&]{
            VEventLoop loop(100);
            volatile bool finished = false;
            std::thread receiver([&]{
                VString name;
                forever {
                        loop.wait();
                        finished = true;
                        VEvent event = loop.next();
                        name = event.name;
                }
                assert(name == "yunzhe");
            });
            receiver.detach();

            loop.send("yunzhe");
            assert(finished);
        });
        sender.detach();
    }

    {
        std::thread sender([&]{
            VEventLoop loop(100);
            volatile int result = 0;
            volatile bool stopped = false;
            std::thread worker([&]{
                forever {
                    loop.wait();
                    VEvent event = loop.next();
                    if (event.name == "quit") {
                        stopped = true;
                        vInfo("stopped");
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
            assert(stopped);
            assert(result == 10);
        });
        sender.detach();
    }
}

ADD_TEST(VEventLoop, test)

}
