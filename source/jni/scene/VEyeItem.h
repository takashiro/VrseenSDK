#pragma once

#include <jni.h>

#include "VEglDriver.h"
#include "VColor.h"

NV_NAMESPACE_BEGIN

class VEyeItem
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
                , wantSingleBuffer(true)
                , colorFormat(VColor::COLOR_8888)
                , commonParameterDepth(DepthFormat_24)
                , commonParameterTexture(NearestTextureFilter)
        {
            useMultiview = VEglDriver::glIsExtensionString("GL_OVR_multiview");
            if(useMultiview) vInfo("useMultiview true")
            else vInfo("useMultiview false");
        }

        int resolution;
        int widthScale;
        int multisamples;
        bool wantSingleBuffer;
        bool useMultiview;

        VColor::Format colorFormat;
        CommonParameter commonParameterDepth;
        CommonParameter commonParameterTexture;
    };

    struct CompletedEyes
    {
        VColor::Format colorFormat;
        GLuint textures;

        CompletedEyes()
            : colorFormat(VColor::COLOR_8888)
            , textures(0)
        {
        }
    };

    VEyeItem();
    virtual ~VEyeItem();

    CompletedEyes completedEyes(const int eyeNum);

    void paint(const int eyeNum);
    void afterPaint(const int eyeNum);

    bool discardInsteadOfClear;
    long swapCount;

    static Settings settings;

private:
    NV_DECLARE_PRIVATE
};

NV_NAMESPACE_END
