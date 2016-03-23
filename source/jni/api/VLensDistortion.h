#pragma once

#include "MemBuffer.h"
#include "sensor/DeviceConstants.h"
#include "VLensDistortion.h"
#include "VGlGeometry.h"

NV_NAMESPACE_BEGIN

struct hmdInfoInternal_t;

class VLens
{
public:
    VLens();
    void initLensByPhoneType(PhoneTypeEnum type);

    const  static int MaxCoefficients = 21;
    DistortionEqnType   Eqn;

    float               K[MaxCoefficients];
    float               MaxR;       // The highest R you're going to query for - the curve is unpredictable beyond it.

    float               MetersPerTanAngleAtCenter;

    // Additional per-channel scaling is applied after distortion:
    //  Index [0] - Red channel constant coefficient.
    //  Index [1] - Red channel r^2 coefficient.
    //  Index [2] - Blue channel constant coefficient.
    //  Index [3] - Blue channel r^2 coefficient.
    float               ChromaticAberration[4];

    float               InvK[MaxCoefficients];
    float               MaxInvR;
};

class VLensDistortion
{
public:
    static VGlGeometry CreateTessellatedMesh(const hmdInfoInternal_t & hmdInfo,const int numSlicesPerEye, const float fovScale,
                                             const bool cursorOnly);
    //default is 32*32
    static int tessellationsX;
    static int tessellationsY;
};

NV_NAMESPACE_END


