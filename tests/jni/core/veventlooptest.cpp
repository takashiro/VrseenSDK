#include "test.h"
#include <VEventLoop.h>
#include <thread>

NV_USING_NAMESPACE

namespace {

void test()
{
    VEventLoop loop(100);
    loop.post("test");

    VEvent event = loop.next();
    assert(event.name == "test");

    {
        std::thread listener([&]{
            loop.wait();
            VEvent event = loop.next();
            assert(event.name == "yunzhe");
        });
        loop.post("yunzhe");
        listener.join();
    }

    {
        bool finished = false;
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

        std::thread sender([&]{
            loop.send("yunzhe");
            assert(finished);
        });
        sender.join();
    }

    {
        int result = 0;
        std::thread worker([&]{
            forever {
                VEvent event = loop.next();
                if (event.name == "quit") {
                    break;
                }
                if (event.isExecutable()) {
                    event.execute();
                }
            }
        });
        worker.detach();

        std::thread master([&]{
            for (int i = 0; i < 10; i++) {
                loop.send([&]{
                    result++;
                });
            }
            loop.send("quit");
            assert(result == 10);
        });
        master.join();
    }
}

ADD_TEST(VEventLoop, test)

}
