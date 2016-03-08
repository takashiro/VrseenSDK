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
        assert(path.dirName() == "Project");
    }

    {
        VPath path("Project/NervGear.git");
        assert(!path.isAbsolute());
        assert(path.extension() == "git");
        assert(path.fileName() == "NervGear.git");
        assert(path.baseName() == "NervGear");
        assert(path.dirName() == "Project");

        path.setExtension("svn");
        assert(path == "Project/NervGear.svn");

        VPath baseName = path.baseName();
        baseName.setExtension("jpg");
        assert(baseName == "NervGear.jpg");
    }

    {
        std::string fileName = "efigs_sdf.ktx";
        VPath path(fileName);
        assert(path.fileName() == fileName);
        assert(path.baseName() == "efigs_sdf");
        assert(path.dirName().isEmpty());
    }
}

ADD_TEST(VPath, test)

}
