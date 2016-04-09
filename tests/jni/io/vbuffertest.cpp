#include "test.h"

#include <VBuffer.h>

NV_USING_NAMESPACE

namespace {

void test()
{
    {
        const char *str = "this is a test.";
        VBuffer buffer;
        buffer.write(str);

        VByteArray bytes = buffer.readAll();
        assert(bytes == str);
    }

    {
        char bytes[128];
        for (char &byte : bytes) {
            byte = static_cast<char>(rand());
        }

        VBuffer buffer;
        buffer.write(bytes, 128);
        assert(buffer.bytesAvailable() == 128);

        VByteArray output = buffer.readAll();
        assert(output.length() == 128);
        for (int i = 0; i < 128; i++) {
            assert(bytes[i] == output[i]);
        }
    }
}

ADD_TEST(VBuffer, test)

}
