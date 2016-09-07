#pragma once

#include "VMainActivity.h"

#include "ModelView.h"
#include "VLockless.h"

NV_NAMESPACE_BEGIN

class VRLauncher : public VMainActivity
{
public:
	enum OvrMenuState
	{
		MENU_NONE,
		MENU_BACKGROUND_INIT,
		NUM_MENU_STATES
	};

	class DoubleBufferedTextureData
	{
	public:
		DoubleBufferedTextureData();
		~DoubleBufferedTextureData();

		// Returns the current texid to render
		GLuint		GetRenderTexId() const;

		// Returns the free texid for load
		GLuint		GetLoadTexId() const;

		// Set the texid after creating a new texture.
		void		SetLoadTexId( const GLuint texId );

		// Swaps the buffers
		void		Swap();

		// Update the last loaded size
		void		SetSize( const int width, const int height );

		// Return true if passed in size match the load index size
		bool		SameSize( const int width, const int height ) const;

	private:
		GLuint			TexId[ 2 ];
		int				Width[ 2 ];
		int				Height[ 2 ];
		volatile int	CurrentIndex;
	};

	VRLauncher(JNIEnv *jni, jclass activityClass, jobject activityObject);
	~VRLauncher();

    void init(const VString &, const VString &, const VString &) override;
    void onStart(const VString &url);
    void shutdown() override;
    void configureVrMode(VKernel* kernel) override;
    VMatrix4f drawEyeView( const int eye, const float fovDegrees ) override;
    VMatrix4f onNewFrame( VFrame vrFrame ) override;
    void command(const VEvent &event) override;
    bool wantSrgbFramebuffer() const override;

	void SetMenuState( const OvrMenuState state );
    OvrMenuState currentState() const { return  m_menuState; }

    bool useOverlay() const;
    VEventLoop &backgroundMessageQueue() { return m_backgroundCommands;  }

private:
	// Background textures loaded into GL by background thread using shared context
	static void *BackgroundGLLoadThread( void * v );
    void startBackgroundPanoLoad(const VString &filename );
    void loadRgbaCubeMap( const int resolution, const uchar * const rgba[ 6 ], const bool useSrgbFormat );
    void loadRgbaTexture( const uchar * data, int width, int height, const bool useSrgbFormat );

	// shared vars
    VGlGeometry			m_globe;

    VSceneView		m_scene;

    // Pano data and menus
    VString						m_PhotoUrl;

	// panorama vars
    DoubleBufferedTextureData	m_backgroundPanoTexData;
    DoubleBufferedTextureData	m_backgroundCubeTexData;
    bool				m_currentPanoIsCubeMap;

    VGlShader			m_texturedMvpProgram;
	VGlShader			m_cubeMapPanoProgram;

    VFrame				m_frameInput;
    OvrMenuState		m_menuState;

    bool				m_useOverlay;				// use the TimeWarp environment overlay
    bool				m_useSrgb;

	// Background texture commands produced by FileLoader consumed by BackgroundGLLoadThread
    VEventLoop		m_backgroundCommands;

	// The background loader loop will exit when this is set true.
    VLockless<bool>		m_shutdownRequest;

	// BackgroundGLLoadThread private GL context used for loading background textures
    EGLint				m_eglClientVersion;
    EGLDisplay			m_eglDisplay;
    EGLConfig			m_eglConfig;
    EGLSurface			m_eglPbufferSurface;
    EGLContext			m_eglShareContext;
};

NV_NAMESPACE_END
