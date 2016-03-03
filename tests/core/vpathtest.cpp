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
        assert(!path.hasExtension());
        assert(path.extension().isEmpty());
    }

    {
        VPath path("Project/NervGear.git");
        assert(!path.isAbsolute());
        assert(path.extension() == "git");
    }
}

ADD_TEST(VPath, test)

}
