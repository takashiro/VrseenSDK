#include "test.h"

#include <VBuffer.h>
#include <VBinaryStream.h>

NV_USING_NAMESPACE

namespace {

void test()
{
    {
        int number = rand();
        const char *data = reinterpret_cast<char *>(&number);
        VBuffer buffer;
        buffer.write(data, sizeof(int));

        VBinaryStream stream(&buffer);
        int output;
        stream >> output;
        assert(output == number);
    }

    {
        VBuffer buffer;
        VBinaryStream stream(&buffer);

        int number = rand();
        stream << number;

        int output;
        stream >> output;
        assert(output == number);
    }

    {
        VBuffer buffer;
        VBinaryStream stream(&buffer);

        struct Pos
        {
            uint x;
            uint y;
            uint z;
            bool active;
        };
        Pos p1;
        p1.x = rand();
        p1.y = rand();
        p1.z = rand();
        p1.active = (rand() & 1) == 1;

        double d1 = (double) rand() / (double) rand();
        longlong l1 = rand();

        stream << d1 << p1 << l1;

        Pos p2;
        double d2;
        longlong l2;
        stream >> d2 >> p2 >> l2;

        assert(d1 == d2);
        assert(l1 == l2);
        assert(p1.x == p2.x);
        assert(p1.y == p2.y);
        assert(p1.z == p2.z);
        assert(p1.active == p2.active);

        int i1 = rand();
        stream << i1;
        int i2;
        stream >> i2;
        assert(i1 == i2);
    }

    {
        VBuffer buffer;
        VBinaryStream stream(&buffer);

        VArray<int> nums;
        for (int i = 0; i < 10; i++) {
            int num = rand();
            stream << num;
            nums.append(num);
        }

        VArray<int> output;
        stream.read(output, 10);

        for (int i = 0; i < 10; i++) {
            assert(nums[i] == output[i]);
        }
    }
}

ADD_TEST(VBinaryStream, test)

}
