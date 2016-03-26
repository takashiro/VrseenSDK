#pragma once

#include "MemBuffer.h"
#include "sensor/DeviceConstants.h"
#include "VLensDistortion.h"
#include "VGlGeometry.h"

NV_NAMESPACE_BEGIN

class VDevice;

class VLensDistortion
{
public:
    VLensDistortion();
    void initLensByPhoneType(PhoneTypeEnum type);
    const  static int MaxCoefficients = 21;
    DistortionEqnType   Eqn;

    float               K[MaxCoefficients];
    float               MaxR;
    float               MetersPerTanAngleAtCenter;   
    float               ChromaticAberration[4];
    float               InvK[MaxCoefficients];
    float               MaxInvR;
    static VGlGeometry CreateTessellatedMesh(const VDevice* device,const int numSlicesPerEye, const float fovScale,
                                             const bool cursorOnly);    
    static int tessellationsX;
    static int tessellationsY;
};

NV_NAMESPACE_END


