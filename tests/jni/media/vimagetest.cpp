#include "test.h"

#include <VImage.h>

NV_USING_NAMESPACE

#define VARRAY_LENGTH 1000

namespace {

void test()
{
    VImage image(VPath("assets/test.bmp"));
    assert(image.isValid());
    assert(image.width() == 1);
    assert(image.height() == 1);

    const uchar *data = image.data();
    assert(data[0] == 128);
    assert(data[1] == 130);
    assert(data[2] == 124);
    assert(data[3] == 0);

    {
        VColor pixel = image.at(0, 0);
        assert(pixel.red == 128);
        assert(pixel.green == 130);
        assert(pixel.blue == 124);
        assert(pixel.alpha == 0);
    }

    image.resize(1, 2);
    {
        VColor pixel = image.at(0, 0);
        assert(pixel.red == 128);
        assert(pixel.green == 130);
        assert(pixel.blue == 124);
        assert(pixel.alpha == 0);
        assert(pixel == image.at(0, 1));
    }

    {
        VImage image2 = image;
        assert(image2.data() != image.data());
        assert(image2 == image);
    }
    assert(image.isValid());

    {
        VImage image2 = image;
        const uchar *data = image2.data();
        VImage image3 = std::move(image2);
        assert(data == image3.data());

        uchar *raw = (uchar *) malloc(image3.length());
        memcpy(raw, image3.data(), image3.length());

        VImage image4(raw, image3.width(), image3.height());
        assert(image4 == image3);
    }
}

ADD_TEST(VArray, test)

}
