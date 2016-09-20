#include "VEyeItem.h"
#include <math.h>
#include <sys/syscall.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

#include "VString.h"
#include "VFile.h"
#include "VTexture.h"

NV_NAMESPACE_BEGIN

static int GL_TEXTURE_SYMBOL  = GL_TEXTURE_2D;
static int GL_FRAMEBUFFER_SYMBOL = GL_FRAMEBUFFER;

struct EyeBuffer {
    EyeBuffer() :
            Texture(0), DepthBuffer(0), CommonParameterBuffer(0), MultisampleColorBuffer(
            0), RenderFrameBuffer(0), ResolveFrameBuffer(0),DepthTexture(0) {
    }
    ~EyeBuffer() {
        Delete();
    }

    GLuint Texture;

    GLuint DepthBuffer;

    GLuint CommonParameterBuffer;

    GLuint MultisampleColorBuffer;

    GLuint RenderFrameBuffer;

    GLuint ResolveFrameBuffer;

    GLuint DepthTexture;

    void Delete() {
        if (Texture) {
            glDeleteTextures(1, &Texture);
            Texture = 0;
        }
        if (DepthBuffer) {
            glDeleteRenderbuffers(1, &DepthBuffer);
            DepthBuffer = 0;
        }
        if (MultisampleColorBuffer) {
            glDeleteRenderbuffers(1, &MultisampleColorBuffer);
            MultisampleColorBuffer = 0;
        }
        if (RenderFrameBuffer) {
            glDeleteFramebuffers(1, &RenderFrameBuffer);
            RenderFrameBuffer = 0;
        }
        if (ResolveFrameBuffer) {
            glDeleteFramebuffers(1, &ResolveFrameBuffer);
            ResolveFrameBuffer = 0;
        }
        if(DepthTexture){
            glDeleteRenderbuffers(1, &DepthTexture);
            DepthTexture = 0;
        }
    }
    void Allocate(const VEyeItem::Settings & bufferParms,
                  VEyeItem::CommonParameter multisampleMode) {
        Delete();

        GLenum depthFormat;
        switch (bufferParms.commonParameterDepth)
        {
            case VEyeItem::DepthFormat_24:
                depthFormat = GL_DEPTH_COMPONENT24_OES;
                break;
            case VEyeItem::DepthFormat_24_stencil_8:
                depthFormat = GL_DEPTH24_STENCIL8_OES;
                break;
            default:
                depthFormat = GL_DEPTH_COMPONENT16;
                break;
        }

        GLenum colorFormat;
        switch(bufferParms.colorFormat)
        {
            case VColor::COLOR_565:
                colorFormat = GL_RGB;
                break;
            case VColor::COLOR_5551:
                colorFormat = GL_RGB5_A1;
                break;
            case VColor::COLOR_8888_sRGB:
                colorFormat = GL_SRGB8_ALPHA8;
                break;
            default:
                colorFormat = GL_RGBA8;
                break;
        }

        GL_TEXTURE_SYMBOL  = bufferParms.useMultiview?GL_TEXTURE_2D_ARRAY:GL_TEXTURE_2D;
        GL_FRAMEBUFFER_SYMBOL = bufferParms.useMultiview?GL_DRAW_FRAMEBUFFER:GL_FRAMEBUFFER;

        glGenFramebuffers(1, &RenderFrameBuffer);
        glBindFramebuffer(GL_FRAMEBUFFER_SYMBOL, RenderFrameBuffer);

        glGenTextures(1, &Texture);
        glBindTexture(GL_TEXTURE_SYMBOL, Texture);

        glTexParameteri( GL_TEXTURE_SYMBOL, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri( GL_TEXTURE_SYMBOL, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        switch (bufferParms.commonParameterTexture)
        {
            case VEyeItem::NearestTextureFilter:
                glTexParameteri( GL_TEXTURE_SYMBOL, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
                glTexParameteri( GL_TEXTURE_SYMBOL, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
                break;
            case VEyeItem::BilinearTextureFilter:
                glTexParameteri( GL_TEXTURE_SYMBOL, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                glTexParameteri( GL_TEXTURE_SYMBOL, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                break;
            case VEyeItem::Aniso2TextureFilter:
                glTexParameteri( GL_TEXTURE_SYMBOL, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                glTexParameteri( GL_TEXTURE_SYMBOL, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                glTexParameterf( GL_TEXTURE_SYMBOL, GL_TEXTURE_MAX_ANISOTROPY_EXT, 2);
                break;
            case VEyeItem::Aniso4TextureFilter:
                glTexParameteri( GL_TEXTURE_SYMBOL, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                glTexParameteri( GL_TEXTURE_SYMBOL, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                glTexParameterf( GL_TEXTURE_SYMBOL, GL_TEXTURE_MAX_ANISOTROPY_EXT, 4);

                break;
            default:
                glTexParameteri( GL_TEXTURE_SYMBOL, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                glTexParameteri( GL_TEXTURE_SYMBOL, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                break;
        }

        if(bufferParms.useMultiview)
        {
            glTexStorage3D(GL_TEXTURE_SYMBOL, 1, colorFormat, bufferParms.widthScale * bufferParms.resolution, bufferParms.resolution, 2);

            if (multisampleMode == VEyeItem::MultisampleRenderToTexture && VEglDriver::glIsExtensionString("GL_OVR_multiview_multisampled_render_to_texture"))
            {
                vInfo("Making a " << bufferParms.multisamples << " sample buffer with glFramebufferTexture2DMultisample");

                //VEglDriver::glFramebufferTextureMultisampleMultiviewOVR(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, RenderFrameBuffer, 0,bufferParms.multisamples,0, 2);
                VEglDriver::glFramebufferTextureMultiviewOVR(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,Texture, 0, 0, 2);

                if (bufferParms.commonParameterDepth != VEyeItem::DepthFormat_0)
                {
                    glGenTextures(1, &DepthTexture);
                    glBindTexture(GL_TEXTURE_2D_ARRAY, DepthTexture);
                    //TODO glTexStorage3DMultisample
                    glTexStorage3D(GL_TEXTURE_2D_ARRAY, 1, depthFormat, bufferParms.widthScale * bufferParms.resolution, bufferParms.resolution, 2);

                    VEglDriver::glFramebufferTextureMultiviewOVR(GL_DRAW_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, DepthTexture, 0, 0, 2);

                    glBindTexture(GL_TEXTURE_2D_ARRAY, 0);
                }
            }
            else
            {
                vInfo("Making a single sample buffer");

                VEglDriver::glFramebufferTextureMultiviewOVR(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,Texture, 0, 0, 2);

                if (bufferParms.commonParameterDepth != VEyeItem::DepthFormat_0)
                {
                    glGenTextures(1, &DepthTexture);
                    glBindTexture(GL_TEXTURE_2D_ARRAY, DepthTexture);
                    glTexStorage3D(GL_TEXTURE_2D_ARRAY, 1, depthFormat, bufferParms.widthScale * bufferParms.resolution, bufferParms.resolution, 2);

                    VEglDriver::glFramebufferTextureMultiviewOVR(GL_DRAW_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, DepthTexture, 0, 0, 2);

                    glBindTexture(GL_TEXTURE_2D_ARRAY, 0);
                }
            }
        }
        else
        {
            glTexStorage2D(GL_TEXTURE_SYMBOL, 1, colorFormat, bufferParms.widthScale * bufferParms.resolution, bufferParms.resolution);

            if (multisampleMode == VEyeItem::MultisampleRenderToTexture)
            {
                vInfo("Making a " << bufferParms.multisamples << " sample buffer with glFramebufferTexture2DMultisample");

                if (bufferParms.commonParameterDepth != VEyeItem::DepthFormat_0)
                {
                    glGenRenderbuffers(1, &DepthBuffer);
                    glBindRenderbuffer( GL_RENDERBUFFER, DepthBuffer);
                    VEglDriver::glRenderbufferStorageMultisampleIMG(
                            GL_RENDERBUFFER, bufferParms.multisamples,
                            depthFormat,
                            bufferParms.widthScale * bufferParms.resolution,
                            bufferParms.resolution);

                    glFramebufferRenderbuffer( GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
                                               GL_RENDERBUFFER, DepthBuffer);
                    glBindRenderbuffer( GL_RENDERBUFFER, 0);
                }

                VEglDriver::glFramebufferTexture2DMultisampleIMG( GL_FRAMEBUFFER,
                                                                  GL_COLOR_ATTACHMENT0,
                                                                  GL_TEXTURE_2D, Texture, 0, bufferParms.multisamples);
            }
            else
            {
                vInfo("Making a single sample buffer");

                if (bufferParms.commonParameterDepth != VEyeItem::DepthFormat_0)
                {
                    glGenRenderbuffers(1, &DepthBuffer);
                    glBindRenderbuffer( GL_RENDERBUFFER, DepthBuffer);
                    glRenderbufferStorage( GL_RENDERBUFFER, depthFormat,
                                           bufferParms.resolution, bufferParms.resolution);
                    glFramebufferRenderbuffer( GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
                                               GL_RENDERBUFFER, DepthBuffer);
                    glBindRenderbuffer( GL_RENDERBUFFER, 0);
                }

                glFramebufferTexture2D( GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                                        GL_TEXTURE_2D, Texture, 0);
            }
        }

        GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER_SYMBOL);
        if (status != GL_FRAMEBUFFER_COMPLETE) {
            vFatal("render FBO " << GL_FRAMEBUFFER_SYMBOL << " is not complete: " << status); // TODO: fall back to something else
        }

        glScissor(0, 0, bufferParms.widthScale * bufferParms.resolution,
                  bufferParms.resolution);
        glViewport(0, 0, bufferParms.widthScale * bufferParms.resolution,
                   bufferParms.resolution);
        glClearColor(53.0f / 255, 166.0f / 255, 240.0f / 255, 1);
        glClear( GL_COLOR_BUFFER_BIT);
        glBindFramebuffer( GL_FRAMEBUFFER_SYMBOL, 0);
    }
};

struct EyePairs
{
    EyePairs() : MultisampleMode( VEyeItem::MultiSampleOff ) {}

    VEyeItem::Settings            BufferParms;
    VEyeItem::CommonParameter       MultisampleMode;
    EyeBuffer           eyeBuffer;
};

static const int MAX_EYE_SETS = 3;

VEyeItem::Settings VEyeItem::settings;

struct VEyeItem::Private
{
    EyePairs     BufferData[MAX_EYE_SETS][2];
};

VEyeItem::VEyeItem():discardInsteadOfClear( true ),swapCount( 0 ),d(new Private)
{

}

VEyeItem::~VEyeItem()
{
    delete d;
}

void VEyeItem::paint(const int eyeNum)
{
    if(eyeNum == 0) swapCount++;

    EyePairs & buffers = d->BufferData[ swapCount % MAX_EYE_SETS ][eyeNum];
    if ( buffers.eyeBuffer.Texture == 0
         || buffers.BufferParms.resolution != settings.resolution
         || buffers.BufferParms.multisamples != settings.multisamples
         || buffers.BufferParms.colorFormat != settings.colorFormat
         || buffers.BufferParms.commonParameterDepth != settings.commonParameterDepth
            )
    {
        vInfo("Reallocating buffers");
        buffers.BufferParms = settings;

        if (settings.multisamples > 1 ) {
            buffers.MultisampleMode = MultisampleRenderToTexture;
        } else {
            buffers.MultisampleMode = MultiSampleOff;
        }

        VEglDriver::logErrorsEnum( "Before framebuffer creation");
        buffers.eyeBuffer.Allocate(settings, buffers.MultisampleMode );
        VEglDriver::logErrorsEnum( "after framebuffer creation" );
    }

    const int resolution = buffers.BufferParms.resolution;
    EyePairs & pair = d->BufferData[ swapCount % MAX_EYE_SETS ][eyeNum];
    EyeBuffer & eye = pair.eyeBuffer;

    glBindFramebuffer( GL_FRAMEBUFFER_SYMBOL, eye.RenderFrameBuffer );
    glViewport( 0, 0, resolution, resolution );
    glScissor( 0, 0, resolution, resolution );
    glDepthMask( GL_TRUE );
    glEnable( GL_DEPTH_TEST );
    glDepthFunc( GL_LEQUAL );

    if ( discardInsteadOfClear )
    {
        VEglDriver::glDisableFramebuffer( true, true );
        glClear( GL_DEPTH_BUFFER_BIT );
    }
    else
    {
        glClearColor( 0, 0, 0, 1 );
        glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
    }
}

void VEyeItem::afterPaint(const int eyeNum)
{
    EyePairs & pair = d->BufferData[ swapCount % MAX_EYE_SETS ][eyeNum];
    EyeBuffer & eye = pair.eyeBuffer;
    int resolution = pair.BufferParms.resolution;

    VEglDriver::glDisableFramebuffer( false, true );

    if ( eye.ResolveFrameBuffer )
    {
        glBindFramebuffer( GL_READ_FRAMEBUFFER, eye.RenderFrameBuffer );
        glBindFramebuffer( GL_DRAW_FRAMEBUFFER, eye.ResolveFrameBuffer );
        VEglDriver::glBlitFramebuffer( 0, 0, resolution, resolution,
                0, 0, resolution, resolution,
                GL_COLOR_BUFFER_BIT, GL_NEAREST );
        VEglDriver::glDisableFramebuffer( true, false );
    }

    glFlush();
}

VEyeItem::CompletedEyes VEyeItem::completedEyes(const int eyeNum)
{
    CompletedEyes cmp;
    // The GPU commands are flushed for d->BufferData[ SwapCount % MAX_EYE_SETS ]
    EyePairs & currentBuffers = d->BufferData[ swapCount % MAX_EYE_SETS ][settings.useMultiview?0:eyeNum];

    EyePairs * buffers = &currentBuffers;

    cmp.textures = buffers->eyeBuffer.Texture;
    cmp.colorFormat = buffers->BufferParms.colorFormat;

    return cmp;
}

NV_NAMESPACE_END
