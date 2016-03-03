#include "test.h"

#include <VPath.h>

NV_USING_NAMESPACE

namespace {

void test()
{
    {
        VPath path("/mnt/media/sdcard/");
        assert(path.isAbsolute());
    }

    {
        VPath path("file:///d/test");
        assert(path.isAbsolute());
    }

    {
        VPath path("D:/Project/NervGear");
        assert(path.isAbsolute());
    }
}

ADD_TEST(VPath, test)

}
