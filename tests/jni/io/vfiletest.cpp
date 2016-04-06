#include "test.h"

#include <VFile.h>

NV_USING_NAMESPACE

namespace {

void test()
{
    {
        VFile file;
        assert(!file.isOpen());
    }

    const char *str = "this is a test";
    {
        VFile file("test.txt", VFile::WriteOnly);
        assert(file.isOpen());
        assert(file.openMode() == VFile::WriteOnly);
        file.write(str);
    }

    {
        VFile file("test.txt", VFile::ReadOnly);
        assert(file.isOpen());
        assert(file.openMode() == VFile::ReadOnly);

        VByteArray data = file.readAll();
        assert(data == str);
    }

    unlink("test.txt");

    {
        char bytes[1024];
        for (char &ch : bytes) {
            ch = static_cast<char>(rand());
        }

        {
            VFile file("test.bin", VFile::WriteOnly);
            assert(file.write(bytes, 512) == 512);
        }

        {
            VFile file("test.bin", VFile::WriteOnly | VFile::Append);
            assert(file.isOpen());
            assert(file.openMode().contains(VFile::Append));
            assert(file.write(bytes + 512, 512) == 512);
        }

        {
            VFile file("test.bin", VFile::ReadOnly);
            VByteArray data = file.readAll();
            assert(data.size() == 1024);
            for (uint i = 0; i < 1024; i++) {
                assert(data[i] == bytes[i]);
            }
        }
    }

    unlink("test.bin");
}

ADD_TEST(VFile, test)

}
