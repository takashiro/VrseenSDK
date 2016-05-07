#pragma once

#include "../vglobal.h"
#include "../api/VEglDriver.h"

// A key aspect of our performance is the ability to render directly
// to a buffer being scanned out for display, instead of going through
// the standard Android swapbuffers path, which adds two or three frames
// of latency to the output.

NV_NAMESPACE_BEGIN

class VrSurfaceManager
{
public:
    VrSurfaceManager();
    ~VrSurfaceManager();

    void	init( JNIEnv * jni );
    void	shutdown();

    bool	setFrontBuffer( const EGLSurface surface, const bool set );
    void *	getFrontBufferAddress( const EGLSurface surface );

    void *	getSurfaceBufferAddress( const EGLSurface surface, int attribs[], const int attr_size, const int pitch );
    void *	getClientBufferAddress( const EGLSurface surface );

private:

    JNIEnv *	m_env;

    jclass		m_surfaceClass;
    jmethodID	m_setFrontBufferID;
    jmethodID	m_getFrontBufferAddressID;
    jmethodID	m_getSurfaceBufferAddressID;
    jmethodID	m_getClientBufferAddressID;
};

class DirectRender
{
public:
    DirectRender();

    // Makes the currently active EGL window surface into a front buffer if possible.
    // Queries all information from EGL so it will work inside Unity as well
    // as our native code.
    // Must be called by the same thread that will be rendering to the surfaces.
    void	initForCurrentSurface( JNIEnv * jni, bool m_wantFrontBuffer,int buildVersionSDK_);

    // Go back to a normal swapped window.
    void	shutdown();

    // Conventionally swapped rendering will return false.
    bool	isFrontBuffer() const;

    // Returns the full resolution as if the screen is in landscape orientation,
    // (1920x1080, etc) even if it is in portrait mode.
    void	getScreenResolution( int & m_width, int & m_height ) const;

    // This starts direct rendering to the front buffer, hopefully using vendor
    // specific extensions to only update half of the window without
    // incurring a tiled renderer pre-read and post-resolve of the unrendered
    // parts.
    // This implicitly sets the scissor rect, which should not be re-set
    // until after EndDirectRendering(), since drivers may use scissor
    // to infer areas to tile.
    void	beginDirectRendering( int x, int y, int m_width, int m_height );

    // This will automatically perform whatever flush is necessary to
    // get the rendering going before returning.
    void	endDirectRendering() const;

    // We still do a swapbuffers even when rendering to the front buffer,
    // because most tools expect swapbuffer delimited frames.
    // This has no effect when in front buffer mode.
    void	swapBuffers() const;

    EGLSurface 		windowSurface;		// swapbuffers will be called on this

private:
    bool				m_wantFrontBuffer;

    VrSurfaceManager	m_surfaceManager;

    EGLDisplay			m_display;
    EGLContext			m_context;

    // From eglQuerySurface
    int					m_width;
    int					m_height;

    bool				m_gvrFrontbufferExtension;

    VEglDriver      m_eglStatus;
};


NV_NAMESPACE_END

