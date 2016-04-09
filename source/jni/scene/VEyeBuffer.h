#pragma once

#include <jni.h>

#include "vglobal.h"
#include "api/VGlOperation.h"

#include "VColor.h"

NV_NAMESPACE_BEGIN

class VEyeBuffer
{
public:
    VEyeBuffer();
    enum CommonParameter
    {
        DEPTHFORMAT_DEPTH_0,
        DEPTHFORMAT_DEPTH_16,
        DEPTHFORMAT_DEPTH_24,
        DEPTHFORMAT_DEPTH_24_STENCIL_8,
        TEXTUREFILTER_NEAREST,
        TEXTUREFILTER_BILINEAR,
        TEXTUREFILTER_ANISO_2,
        TEXTUREFILTER_ANISO_4,
        MULTISAMPLE_OFF,
        MULTISAMPLE_RENDER_TO_TEXTURE
    };

    struct EyeParms
    {
            EyeParms() :
                resolution( 1024 ),
                WidthScale( 1 ),
                multisamples( 2 ),
                colorFormat( VColor::COLOR_8888 ),
                commonParameterDepth(DEPTHFORMAT_DEPTH_24),
                commonParameterTexture(TEXTUREFILTER_NEAREST)
            {
            }

        int     resolution;

        int     WidthScale;

        int     multisamples;

        VColor::Format      colorFormat;

        CommonParameter     commonParameterDepth;

        CommonParameter     commonParameterTexture;

    };

    struct CompletedEyes
    {
        VColor::Format colorFormat;
        GLuint textures[2];
    };
    CompletedEyes   GetCompletedEyes();

    EyeParms    BufferParms;

    VGlOperation    glOperation;

    void    BeginFrame( const EyeParms &    bufferParms_ );

    void    BeginRenderingEye( const int eyeNum );

    void    EndRenderingEye( const int eyeNum );

    void    ScreenShot();

    bool    DiscardInsteadOfClear;

    long    SwapCount;

};

NV_NAMESPACE_END
