#include "test.h"

#include <VPath.h>

NV_USING_NAMESPACE

namespace {

void test()
{
    {
        VPath path("/mnt/media/sdcard/");
        assert(path.isAbsolute());
        assert(!path.hasProtocol());
    }

    {
        VPath path("file:///d/test");
        assert(path.isAbsolute());
        assert(path.protocol() == "file://");
    }

    {
        VPath path("D:/Project/NervGear");
        assert(path.isAbsolute());
        assert(!path.hasExtension());
        assert(path.extension().isEmpty());
        assert(path.fileName() == path.baseName());
    }

    {
        VPath path("Project/NervGear.git");
        assert(!path.isAbsolute());
        assert(path.extension() == "git");
        assert(path.fileName() == "NervGear.git");
        std::cout << path.baseName() << std::endl;
        assert(path.baseName() == "NervGear");
    }
}

ADD_TEST(VPath, test)

}
