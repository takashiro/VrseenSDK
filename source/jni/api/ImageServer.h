#pragma once

#include "Lockless.h"
#include "Android/GlUtils.h"

#include "WarpGeometry.h"
#include "VGlShader.h"

NV_NAMESPACE_BEGIN

class ImageServerRequest
{
public:
    ImageServerRequest() : sequence( 0 ), resolution( 0 ) {}

    int				sequence;		// incremented each time
    int				resolution;
};

class ImageServerResponse
{
public:
    ImageServerResponse() : sequence( 0 ), resolution( 0 ), data( NULL ) {}

    int				sequence;		// incremented each time
    int				resolution;
    const void *	data;
};


class ImageServer
{
public:
	ImageServer();
	~ImageServer();

	// Called by TimeWarp before adding the KHR sync object.
    void				enterWarpSwap( int eyeTexture );

	// Called by TimeWarp after syncing to the previous frame's
	// sync object.
    void				leaveWarpSwap();

private:

    static void *		threadStarter( void * parm );
    void 				serverThread();
    void				freeBuffers();

	// When an image request arrives on the network socket,
	// it will be placed in request so TimeWarp can notice it.
    LocklessUpdater<ImageServerRequest>		m_request;

	// After TimeWarp has completed a resampling and asynchronous
	// readback of an eye buffer, the results will be placed here.
	// The data only remains valid until the next request is set.
    LocklessUpdater<ImageServerResponse>	m_response;

	// Write anything on this to shutdown the server thread
    int					m_shutdownSocket;

	// The eye texture is drawn to the ResampleRenderBuffer through
	// the FrameBufferObject, then copied to the PixelBufferObject
    int					m_currentResolution;
    int					m_sequenceCaptured;
    WarpGeometry		m_quad;
	VGlShader			m_resampleProg;
    GLuint				m_resampleRenderBuffer;
    GLuint				m_frameBufferObject;
    GLuint				m_pixelBufferObject;
    void *				m_pboMappedAddress;

    pthread_t			m_serverThread;		// posix pthread

    pthread_mutex_t		m_responseMutex;
    pthread_cond_t		m_responseCondition;

    pthread_mutex_t		m_startStopMutex;
    pthread_cond_t		m_startStopCondition;

    int					m_countdownToSend;
};

}


