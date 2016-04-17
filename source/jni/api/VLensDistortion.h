#pragma once

#include "VLensDistortion.h"
#include "VGlGeometry.h"

NV_NAMESPACE_BEGIN

class VDevice;

class VLensDistortion
{
public:
    VLensDistortion();
    void initDistortionParmsByMobileType();
    const  static int MaxCoefficients = 21;

    float               kArray[MaxCoefficients];
    float               maxR;
    float               centMetersPerTanAngler;
    float               chromaticAberration[4];
    float               invKArray[MaxCoefficients];
    float               maxInvR;
    static VGlGeometry createDistortionGrid(const VDevice* device,const int numSlicesPerEye, const float fovScale,
                                             const bool cursorOnly);    
    static int xxGridNum;
    static int yyGridNum;
};

NV_NAMESPACE_END


