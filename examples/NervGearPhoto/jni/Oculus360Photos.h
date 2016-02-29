/************************************************************************************

Filename    :   Oculus360Photos.h
Content     :   360 Panorama Viewer
Created     :   August 13, 2014
Authors     :   John Carmack, Warsam Osman

Copyright   :   Copyright 2014 Oculus VR, LLC. All Rights reserved.

This source code is licensed under the BSD-style license found in the
LICENSE file in the Oculus360Photos/ directory. An additional grant
of patent rights can be found in the PATENTS file in the same directory.

************************************************************************************/

#ifndef OCULUS360PHOTOS_H
#define OCULUS360PHOTOS_H

#include "ModelView.h"
#include "gui/Fader.h"
#include "Lockless.h"

namespace NervGear {

class PanoBrowser;
class OvrPanoMenu;
class OvrMetaData;
struct OvrMetaDatum;
class OvrPhotosMetaData;
struct OvrPhotosMetaDatum;

class Oculus360Photos : public VrAppInterface
{
public:
	enum OvrMenuState
	{
		MENU_NONE,
		MENU_BACKGROUND_INIT,
		MENU_BROWSER,
		MENU_PANO_LOADING,
		MENU_PANO_FADEIN,
		MENU_PANO_REOPEN_FADEIN,
		MENU_PANO_FULLY_VISIBLE,
		MENU_PANO_FADEOUT,
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

	Oculus360Photos();
	~Oculus360Photos();

    void OneTimeInit( const char * fromPackage, const char * launchIntentJSON, const char * launchIntentURI ) override;
    void OneTimeShutdown() override;
    void ConfigureVrMode( ovrModeParms & modeParms ) override;
    Matrix4f 	DrawEyeView( const int eye, const float fovDegrees ) override;
    Matrix4f 	Frame( VrFrame vrFrame ) override;
    void		Command( const char * msg ) override;
    bool 		onKeyEvent( const int keyCode, const KeyState::eKeyEventType eventType ) override;
    bool		wantSrgbFramebuffer() const override;

    void				onPanoActivated( const OvrMetaDatum * panoData );
    PanoBrowser *		browser()										{ return m_browser; }
    OvrPhotosMetaData *	metaData()										{ return m_metaData; }
    const OvrPhotosMetaDatum * activePano() const						{ return m_activePano; }
    void				setActivePano( const OvrPhotosMetaDatum * data )	{ OVR_ASSERT( data );  m_activePano = data; }
    float				fadeLevel() const								{ return m_currentFadeLevel;  }
    int					numPanosInActiveCategory() const;

	void				SetMenuState( const OvrMenuState state );
    OvrMenuState		currentState() const								{ return  m_menuState; }

    int					toggleCurrentAsFavorite();

    bool				useOverlay() const;
    bool				allowPanoInput() const;
    MessageQueue &		backgroundMessageQueue() { return m_backgroundCommands;  }

private:
	// Background textures loaded into GL by background thread using shared context
	static void *		BackgroundGLLoadThread( void * v );
    void				startBackgroundPanoLoad( const char * filename );
    const char *		menuStateString( const OvrMenuState state );
    bool 				loadMetaData( const char * metaFile );
    void				loadRgbaCubeMap( const int resolution, const unsigned char * const rgba[ 6 ], const bool useSrgbFormat );
    void				loadRgbaTexture( const unsigned char * data, int width, int height, const bool useSrgbFormat );

	// shared vars
    jclass				m_mainActivityClass;	// need to look up from main thread
    GlGeometry			m_globe;

    OvrSceneView		m_scene;
    SineFader			m_fader;

	// Pano data and menus
    Array< VString > 			m_searchPaths;
    OvrPhotosMetaData *			m_metaData;
    OvrPanoMenu *				m_panoMenu;
    PanoBrowser *				m_browser;
    const OvrPhotosMetaDatum *	m_activePano;
    VString						m_startupPano;

	// panorama vars
    DoubleBufferedTextureData	m_backgroundPanoTexData;
    DoubleBufferedTextureData	m_backgroundCubeTexData;
    bool				m_currentPanoIsCubeMap;

    GlProgram			m_texturedMvpProgram;
    GlProgram			m_cubeMapPanoProgram;
    GlProgram			m_panoramaProgram;

    VrFrame				m_frameInput;
    OvrMenuState		m_menuState;

    const float			m_fadeOutRate;
    const float			m_fadeInRate;
    const float			m_panoMenuVisibleTime;
    float				m_currentFadeRate;
    float				m_currentFadeLevel;
    float				m_panoMenuTimeLeft;
    float				m_browserOpenTime;

    bool				m_useOverlay;				// use the TimeWarp environment overlay
    bool				m_useSrgb;

	// Background texture commands produced by FileLoader consumed by BackgroundGLLoadThread
    MessageQueue		m_backgroundCommands;

	// The background loader loop will exit when this is set true.
    LocklessUpdater<bool>		m_shutdownRequest;

	// BackgroundGLLoadThread private GL context used for loading background textures
    EGLint				m_eglClientVersion;
    EGLDisplay			m_eglDisplay;
    EGLConfig			m_eglConfig;
    EGLSurface			m_eglPbufferSurface;
    EGLContext			m_eglShareContext;
};

}

#endif // OCULUS360PHOTOS_H
