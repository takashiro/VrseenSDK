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

#include "3rdParty/stb/stb_image_write.h"
#include "VTexture.h"

#include "io/VFileOperation.h"

NV_NAMESPACE_BEGIN

struct EyeBuffer {
    EyeBuffer() :
            Texture(0), DepthBuffer(0), CommonParameterBuffer(0), MultisampleColorBuffer(
            0), RenderFrameBuffer(0), ResolveFrameBuffer(0) {
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
    }
    void Allocate(const VEyeItem::Settings & bufferParms,
                  VEyeItem::CommonParameter multisampleMode) {
        Delete();

        GLenum commonParameterDepth;
        switch (bufferParms.commonParameterDepth) {
            case VEyeItem::DepthFormat_24:
                commonParameterDepth = GL_DEPTH_COMPONENT24_OES;
                break;
            case VEyeItem::DepthFormat_24_stencil_8:
                commonParameterDepth = GL_DEPTH24_STENCIL8_OES;
                break;
            default:
                commonParameterDepth = GL_DEPTH_COMPONENT16;
                break;
        }

        glGenTextures(1, &Texture);
        glBindTexture( GL_TEXTURE_2D, Texture);

        if (bufferParms.colorFormat == VColor::COLOR_565) {
            glTexImage2D( GL_TEXTURE_2D, 0, GL_RGB,
                          bufferParms.widthScale * bufferParms.resolution,
                          bufferParms.resolution, 0,
                          GL_RGB, GL_UNSIGNED_SHORT_5_6_5, NULL);
        } else if (bufferParms.colorFormat == VColor::COLOR_5551) {
            glTexImage2D( GL_TEXTURE_2D, 0, GL_RGB5_A1,
                          bufferParms.widthScale * bufferParms.resolution,
                          bufferParms.resolution, 0,
                          GL_RGBA, GL_UNSIGNED_SHORT_5_5_5_1, NULL);
        } else if (bufferParms.colorFormat == VColor::COLOR_8888_sRGB) {
            glTexImage2D( GL_TEXTURE_2D, 0, GL_SRGB8_ALPHA8,
                          bufferParms.widthScale * bufferParms.resolution,
                          bufferParms.resolution, 0,
                          GL_RGBA, GL_UNSIGNED_BYTE, NULL);
        } else {
            glTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA8,
                          bufferParms.widthScale * bufferParms.resolution,
                          bufferParms.resolution, 0,
                          GL_RGBA, GL_UNSIGNED_BYTE, NULL);
        }

        glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        switch (bufferParms.commonParameterTexture) {
            case VEyeItem::NearestTextureFilter:
                glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
                glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
                vInfo("textureFilter = TEXTURE_FILTER_NEAREST")
                        ;
                break;
            case VEyeItem::BilinearTextureFilter:
                glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                vInfo("textureFilter = TEXTURE_FILTER_BILINEAR")
                        ;
                break;
            case VEyeItem::Aniso2TextureFilter:
                glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, 2);
                vInfo("textureFilter = TEXTURE_FILTER_ANISO_2")
                        ;
                break;
            case VEyeItem::Aniso4TextureFilter:
                glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, 4);
                vInfo("textureFilter = TEXTURE_FILTER_ANISO_4")
                        ;
                break;
            default:
                glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                vInfo("textureFilter = TEXTURE_FILTER_BILINEAR")
                        ;
                break;
        }

        if (multisampleMode == VEyeItem::MultisampleRenderToTexture) {
            vInfo(
                    "Making a " << bufferParms.multisamples << " sample buffer with glFramebufferTexture2DMultisample");

            if (bufferParms.commonParameterDepth
                != VEyeItem::DepthFormat_0) {
                glGenRenderbuffers(1, &DepthBuffer);
                glBindRenderbuffer( GL_RENDERBUFFER, DepthBuffer);
                VEglDriver::glRenderbufferStorageMultisampleIMG(
                        GL_RENDERBUFFER, bufferParms.multisamples,
                        commonParameterDepth,
                        bufferParms.widthScale * bufferParms.resolution,
                        bufferParms.resolution);

                glBindRenderbuffer( GL_RENDERBUFFER, 0);
            }

            glGenFramebuffers(1, &RenderFrameBuffer);
            glBindFramebuffer( GL_FRAMEBUFFER, RenderFrameBuffer);

            VEglDriver::glFramebufferTexture2DMultisampleIMG( GL_FRAMEBUFFER,
                                                              GL_COLOR_ATTACHMENT0,
                                                              GL_TEXTURE_2D, Texture, 0, bufferParms.multisamples);

            if (bufferParms.commonParameterDepth
                != VEyeItem::DepthFormat_0) {
                glFramebufferRenderbuffer( GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
                                           GL_RENDERBUFFER, DepthBuffer);
            }
            VEglDriver::logErrorsEnum(
                    "glRenderbufferStorageMultisampleIMG MSAA");
        } else {
            vInfo("Making a single sample buffer");

            if (bufferParms.commonParameterDepth
                != VEyeItem::DepthFormat_0) {
                glGenRenderbuffers(1, &DepthBuffer);
                glBindRenderbuffer( GL_RENDERBUFFER, DepthBuffer);
                glRenderbufferStorage( GL_RENDERBUFFER, commonParameterDepth,
                                       bufferParms.resolution, bufferParms.resolution);

                glBindRenderbuffer( GL_RENDERBUFFER, 0);
            }

            glGenFramebuffers(1, &RenderFrameBuffer);
            glBindFramebuffer( GL_FRAMEBUFFER, RenderFrameBuffer);
            glFramebufferTexture2D( GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                                    GL_TEXTURE_2D, Texture, 0);

            if (bufferParms.commonParameterDepth
                != VEyeItem::DepthFormat_0) {
                glFramebufferRenderbuffer( GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
                                           GL_RENDERBUFFER, DepthBuffer);
            }

            VEglDriver::logErrorsEnum("NO MSAA");
        }

        GLenum status = glCheckFramebufferStatus( GL_FRAMEBUFFER);
        if (status != GL_FRAMEBUFFER_COMPLETE) {
            vFatal(
                    "render FBO " << RenderFrameBuffer << " is not complete: " << status); // TODO: fall back to something else
        }

        glScissor(0, 0, bufferParms.widthScale * bufferParms.resolution,
                  bufferParms.resolution);
        glViewport(0, 0, bufferParms.widthScale * bufferParms.resolution,
                   bufferParms.resolution);
        glClearColor(0, 1, 0, 1);
        glClear( GL_COLOR_BUFFER_BIT);
        glBindFramebuffer( GL_FRAMEBUFFER, 0);
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

static int FindUnusedFilename(const char *format, int max)
{
    for (int i = 0; i <= max; i++) {
        VString fileName;
        fileName.sprintf(format, i);
        if (!VFile::Exists(fileName)) {
            return i;
        }
    }
    return max;
}

static void ScreenShotTexture( const int eyeResolution, const GLuint texId )
{
    GLuint	fbo;
    glGenFramebuffers( 1, &fbo );
    glBindFramebuffer( GL_FRAMEBUFFER, fbo );
    glFramebufferTexture2D( GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texId, 0 );

    unsigned char * buf = (unsigned char *)malloc( eyeResolution * eyeResolution * 8 );
    glReadPixels( 0, 0, eyeResolution, eyeResolution, GL_RGBA, GL_UNSIGNED_BYTE, (void *)buf );
    glDeleteFramebuffers( 1, &fbo );

    for ( int y = 0 ; y < eyeResolution ; y++ )
    {
        const int iy = eyeResolution - 1 - y;
        unsigned char * src = buf + y * eyeResolution * 4;
        unsigned char * dest = buf + (eyeResolution + iy ) * eyeResolution * 4;
        memcpy( dest, src, eyeResolution*4 );
        for ( int x = 0 ; x < eyeResolution ; x++ )
        {
            dest[x*4+3] = 255;
        }
    }


    const char * fmt = "/sdcard/Oculus/screenshot%03i.bmp";
    const int v = FindUnusedFilename( fmt, 999 );
    VString filename;
    filename.sprintf(fmt, v);

    const unsigned char * flipped = (buf + eyeResolution*eyeResolution*4);
    stbi_write_bmp( filename.toUtf8().data(), eyeResolution, eyeResolution, 4, (void *)flipped );

    unsigned char * shrunk1 = VFileOperation::QuarterImageSize( flipped, eyeResolution, eyeResolution, true );
    unsigned char * shrunk2 = VFileOperation::QuarterImageSize( shrunk1, eyeResolution>>1, eyeResolution>>1, true );
    VString filename2;
    filename2.sprintf("/sdcard/Oculus/thumbnail%03i.pvr", v);
    VFileOperation::Write32BitPvrTexture( filename2.toUtf8().data(), shrunk2, eyeResolution>>2, eyeResolution>>2 );

    free( buf );
    free( shrunk1 );
    free( shrunk2 );
}

VEyeItem::Settings VEyeItem::settings;

struct VEyeItem::Private
{
    EyePairs     BufferData[MAX_EYE_SETS];
};

VEyeItem::VEyeItem():discardInsteadOfClear( true ),swapCount( 0 ),d(new Private)
{
    
}

VEyeItem::~VEyeItem()
{
    delete d;
}

void VEyeItem::paint()
{
    swapCount++;

    EyePairs & buffers = d->BufferData[ swapCount % MAX_EYE_SETS ];
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
    EyePairs & pair = d->BufferData[ swapCount % MAX_EYE_SETS ];
    EyeBuffer & eye = pair.eyeBuffer;

    glBindFramebuffer( GL_FRAMEBUFFER, eye.RenderFrameBuffer );
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

void VEyeItem::afterPaint()
{
    EyePairs & pair = d->BufferData[ swapCount % MAX_EYE_SETS ];
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

VEyeItem::CompletedEyes VEyeItem::completedEyes()
{
    CompletedEyes	cmp = {};
    // The GPU commands are flushed for BufferData[ SwapCount % MAX_EYE_SETS ]
    EyePairs & currentBuffers = d->BufferData[ swapCount % MAX_EYE_SETS ];

    EyePairs * buffers = &currentBuffers;

    cmp.textures = buffers->eyeBuffer.Texture;
    cmp.colorFormat = buffers->BufferParms.colorFormat;

    return cmp;
}

void VEyeItem::snapshot()
{
    ScreenShotTexture(d->BufferData[0].BufferParms.resolution, d->BufferData[0].eyeBuffer.Texture );
}

NV_NAMESPACE_END
