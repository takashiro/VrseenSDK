#include "test.h"

#include <VVect2.h>

NV_USING_NAMESPACE

namespace {

void test()
{
    {
        //Default constructor
        VVect2i v1;
        assert(v1.x == 0);
        assert(v1.y == 0);

        //Direct access to x and y
        v1.x = rand();
        v1.y = rand();

        //Copy constructor
        VVect2i v2(v1);
        assert(v1.x == v2.x);
        assert(v1.y == v2.y);
        assert(v1 == v2);

        //1-parameter constructor
        VVect2i v3(rand());
        assert(v3.x == v3.y);

        //Copy assignment operator
        v1 = v3;
        assert(v1.x == v3.x);
        assert(v1.y == v3.y);
        assert(v1 == v3);

        // equal and inequal operator
        if (v1.x != v2.x || v1.y != v2.y) {
            assert(v1 != v2);
        } else {
            assert(v1 == v2);
        }

        //Plus operator
        {
            VVect2i v4 = v1 + v2;
            assert(v4.x == v1.x + v2.x);
            assert(v4.y == v1.y + v2.y);

            VVect2i v5 = v1;
            v5 += v2;
            assert(v4 == v5);
        }

        //Minus operator
        {
            VVect2i v4 = v1 - v2;
            assert(v4.x == v1.x - v2.x);
            assert(v4.y == v1.y - v2.y);

            VVect2i v5 = v1;
            v5 -= v2;
            assert(v4 == v5);
        }

        //Scalar *
        {
            int factor = rand();
            VVect2i v4 = v2 * factor;
            assert(v4.x == v2.x * factor);
            assert(v4.y == v2.y * factor);


            VVect2i v5 = v2;
            v5 *= factor;
            assert(v5 == v4);
        }

        //Scalar /
        {
            int factor = rand();
            VVect2i v4 = v2 / factor;
            assert(v4.x == v2.x / factor);
            assert(v4.y == v2.y / factor);


            VVect2i v5 = v2;
            v5 /= factor;
            assert(v5 == v4);
        }
    }
}

ADD_TEST(VVect, test)

}
