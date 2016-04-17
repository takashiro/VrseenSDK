#pragma once

#include <jni.h>

#include "VEglDriver.h"
#include "VColor.h"

NV_NAMESPACE_BEGIN

class VEyeBuffer
{
public:
    enum CommonParameter
    {
        DepthFormat_0,
        DepthFormat_16,
        DepthFormat_24,
        DepthFormat_24_stencil_8,

        NearestTextureFilter,
        BilinearTextureFilter,
        Aniso2TextureFilter,
        Aniso4TextureFilter,

        MultiSampleOff,
        MultisampleRenderToTexture
    };

    struct Settings
    {
        Settings()
            : resolution(1024)
            , widthScale(1)
            , multisamples(2)
            , colorFormat(VColor::COLOR_8888)
            , commonParameterDepth(DepthFormat_24)
            , commonParameterTexture(NearestTextureFilter)
        {
        }

        int resolution;
        int widthScale;
        int multisamples;
        VColor::Format colorFormat;
        CommonParameter commonParameterDepth;
        CommonParameter commonParameterTexture;
    };

    struct CompletedEyes
    {
        VColor::Format colorFormat;
        GLuint textures[2];
    };

    VEyeBuffer();

    CompletedEyes completedEyes();

    void beginFrame(const Settings &bufferParms_);
    void beginRendering(const int eyeNum);
    void endRendering(const int eyeNum);

    void snapshot();

    Settings bufferParms;
    bool discardInsteadOfClear;
    long swapCount;
};

NV_NAMESPACE_END
