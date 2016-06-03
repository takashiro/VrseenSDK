#include "VImage.h"

#include <math.h>
#include <3rdparty/stb/stb_image.h>

NV_NAMESPACE_BEGIN

struct VImage::Private
{
    uchar *data;
    int width;
    int height;

    Private()
        : data(nullptr)
        , width(0)
        , height(0)
    {
    }

    ~Private()
    {
        if (data) {
            free(data);
        }
    }

    void load(const VPath &path)
    {
        int cmp;
        data = stbi_load(path.toUtf8().data(), &width, &height, &cmp, 4);
    }

    void load(const VByteArray &encoded)
    {
        int cmp;
        data = stbi_load_from_memory(reinterpret_cast<const uchar *>(encoded.data()), encoded.size(), &width, &height, &cmp, 4);
    }
};

VImage::VImage()
    : d(new Private)
{
}

VImage::VImage(const VPath &path)
    : d(new Private)
{
    d->load(path);
}

VImage::VImage(const VImage &source)
    : d(new Private)
{
    d->data = (uchar *) malloc(source.length());
    memcpy(d->data, source.data(), source.length());
    d->width = source.width();
    d->height = source.height();
}

VImage::VImage(VImage &&source)
    : d(source.d)
{
    source.d = nullptr;
}

VImage::VImage(uchar *decoded, int width, int height)
    : d(new Private)
{
    d->data = decoded;
    d->width = width;
    d->height = height;
}

VImage::VImage(const VByteArray &encoded)
    : d(new Private)
{
    d->load(encoded);
}

VImage::~VImage()
{
    delete d;
}

bool VImage::load(const VPath &path)
{
    d->load(path);
    return isValid();
}

bool VImage::load(const VByteArray &data)
{
    d->load(data);
    return isValid();
}

bool VImage::isValid() const
{
    return d->data != nullptr;
}

int VImage::width() const
{
    return d->width;
}

int VImage::height() const
{
    return d->height;
}

const uchar *VImage::data() const
{
    return d->data;
}

uint VImage::length() const
{
    return d->width * d->height * 4;
}

VColor VImage::at(int x, int y) const
{
    VColor pixel;
    if (x < 0 || x >= d->width || y < 0 || y >= d->height) {
        return pixel;
    }

    uint index = y * d->width + x;
    VColor *pixels = reinterpret_cast<VColor *>(d->data);
    return pixels[index];
}

static float SRGBToLinear(float c)
{
    const float a = 0.055f;
    if (c <= 0.04045f) {
        return c * (1.0f / 12.92f);
    } else {
        return powf(((c + a) * (1.0f / (1.0f + a))), 2.4f);
    }
}

static const float BICUBIC_SHARPEN = 0.75f; // same as default PhotoShop bicubic filter

static void FilterWeights(float s, VImage::Filter filter, float weights[4])
{
    switch (filter) {
    case VImage::NearestFilter: {
        weights[0] = 1.0f;
        break;
    }
    case VImage::LinearFilter: {
        weights[0] = 1.0f - s;
        weights[1] = s;
        break;
    }
    case VImage::CubicFilter: {
        weights[0] = ((((+0.0f - BICUBIC_SHARPEN) * s + (+0.0f + 2.0f * BICUBIC_SHARPEN)) * s + -BICUBIC_SHARPEN) * s + 0.0f);
        weights[1] = ((((+2.0f - BICUBIC_SHARPEN) * s + (-3.0f + 1.0f * BICUBIC_SHARPEN)) * s + 0.0f) * s + 1.0f);
        weights[2] = ((((-2.0f + BICUBIC_SHARPEN) * s + (+3.0f - 2.0f * BICUBIC_SHARPEN)) * s + BICUBIC_SHARPEN) * s + 0.0f);
        weights[3] = ((((+0.0f + BICUBIC_SHARPEN) * s + (+0.0f - 1.0f * BICUBIC_SHARPEN)) * s + 0.0f) * s + 0.0f);
        break;
    }
    }
}

static float LinearToSRGB(float c)
{
    const float a = 0.055f;
    if (c <= 0.0031308f) {
        return c * 12.92f;
    } else {
        return (1.0f + a) * powf(c, (1.0f / 2.4f)) - a;
    }
}

void VImage::resize(int newWidth, int newHeight, Filter filter)
{
    int footprintMin = 0;
    int footprintMax = 0;
    int offsetX = 0;
    int offsetY = 0;
    switch (filter) {
    case NearestFilter: {
        footprintMin = 0;
        footprintMax = 0;
        offsetX = d->width;
        offsetY = d->height;
        break;
    }
    case LinearFilter: {
        footprintMin = 0;
        footprintMax = 1;
        offsetX = d->width - newWidth;
        offsetY = d->height - newHeight;
        break;
    }
    case CubicFilter: {
        footprintMin = -1;
        footprintMax = 2;
        offsetX = d->width - newWidth;
        offsetY = d->height - newHeight;
        break;
    }
    }

    uchar *scaled = (uchar *) malloc(newWidth * newHeight * 4);

    float *srcLinear = (float *) malloc(d->width * d->height * 4 * sizeof(float));
    float *scaledLinear = (float *) malloc(newWidth * newHeight * 4 * sizeof(float));

    float table[256];
    for (int i = 0; i < 256; i++) {
        table[i] = SRGBToLinear(i * (1.0f / 255.0f));
    }

    for (int y = 0; y < d->height; y++) {
        for (int x = 0; x < d->width; x++) {
            for (int c = 0; c < 4; c++) {
                srcLinear[(y * d->width + x) * 4 + c] = table[d->data[(y * d->width + x) * 4 + c]];
            }
        }
    }

    auto FracFloat = [](float x){
        return x - floorf(x);
    };

    for (int y = 0; y < newHeight; y++) {
        const int srcY = (y * d->height * 2 + offsetY) / (newHeight * 2);
        const float fracY = FracFloat(((float) y * d->height * 2.0f + offsetY) / (newHeight * 2.0f));

        float weightsY[4];
        FilterWeights(fracY, filter, weightsY);

        for (int x = 0; x < newWidth; x++) {
            const int srcX = ( x * d->width * 2 + offsetX ) / ( newWidth * 2 );
            const float fracX = FracFloat(((float) x * d->width * 2.0f + offsetX) / (newWidth * 2.0f));

            float weightsX[4];
            FilterWeights(fracX, filter, weightsX);

            float fR = 0.0f;
            float fG = 0.0f;
            float fB = 0.0f;
            float fA = 0.0f;

            for (int fpY = footprintMin; fpY <= footprintMax; fpY++) {
                const float wY = weightsY[fpY - footprintMin];

                for (int fpX = footprintMin; fpX <= footprintMax; fpX++) {
                    const float wX = weightsX[fpX - footprintMin];
                    const float wXY = wX * wY;

                    const int cx = std::min(std::max(0, srcX + fpX), d->width - 1);
                    const int cy = std::min(std::max(0, srcY + fpY), d->height - 1);
                    fR += srcLinear[(cy * d->width + cx) * 4 + 0] * wXY;
                    fG += srcLinear[(cy * d->width + cx) * 4 + 1] * wXY;
                    fB += srcLinear[(cy * d->width + cx) * 4 + 2] * wXY;
                    fA += srcLinear[(cy * d->width + cx) * 4 + 3] * wXY;
                }
            }

            scaledLinear[(y * newWidth + x) * 4 + 0] = fR;
            scaledLinear[(y * newWidth + x) * 4 + 1] = fG;
            scaledLinear[(y * newWidth + x) * 4 + 2] = fB;
            scaledLinear[(y * newWidth + x) * 4 + 3] = fA;
        }
    }

    for (int y = 0; y < newHeight; y++) {
        for (int x = 0; x < newWidth; x++) {
            for (int c = 0; c < 4; c++) {
                const float gamma = LinearToSRGB( scaledLinear[ ( y * newWidth + x ) * 4 + c ] );
                scaled[(y * newWidth + x) * 4 + c] = ( unsigned char ) std::min(std::max(0, (int) (gamma * 255.0f + 0.5f)), 255);
            }
        }
    }

    free(scaledLinear);
    free(srcLinear);

    free(d->data);
    d->data = scaled;
    d->width = newWidth;
    d->height = newHeight;
}

void VImage::quarter(bool srgb)
{
    float table[256];
    if (srgb) {
        for (int i = 0; i < 256; i++) {
            table[i] = SRGBToLinear(i * (1.0f / 255.0f));
        }
    }

    const int width = this->width();
    const int height = this->height();
    const int newWidth = std::max(1, width >> 1);
    const int newHeight = std::max(1, height >> 1);
    uchar *out = (uchar *) malloc(newWidth * newHeight * 4);
    uchar *out_p = out;
    for (int y = 0; y < newHeight; y++) {
        const uchar *in_p = d->data + y * 2 * width * 4;
        for (int x = 0; x < newWidth; x++) {
            for (int i = 0; i < 4; i++) {
                if (srgb) {
                    const float linear = (table[in_p[i]]
                            + table[in_p[4 + i]]
                            + table[in_p[width * 4 + i]]
                            + table[in_p[width * 4 + 4 + i]]) * 0.25f;
                    const float gamma = LinearToSRGB(linear);
                    out_p[i] = (uchar) std::min(std::max(0, (int) (gamma * 255.0f + 0.5f)), 255);
                } else {
                    out_p[i] = (in_p[i] + in_p[4 + i] + in_p[width * 4 + i] + in_p[width * 4 + 4 + i]) >> 2;
                }
            }
            out_p += 4;
            in_p += 8;
        }
    }
    free(d->data);
    d->data = out;
    d->width = newWidth;
    d->height = newHeight;
}

bool VImage::operator==(const VImage &source) const
{
    if (width() != source.width() || height() != source.height()) {
        return false;
    }

    const uchar *data = source.data();
    for (uint i = 0, max = length(); i < max; i++) {
        if (d->data[i] != data[i]) {
            return false;
        }
    }
    return true;
}

NV_NAMESPACE_END
