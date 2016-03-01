#ifndef OVRAPP_H
#define OVRAPP_H

#include "App.h"
#include "ModelView.h"

NV_USING_NAMESPACE

class OvrApp : public NervGear::VrAppInterface
{
public:
						OvrApp();
    virtual				~OvrApp();

	virtual void		OneTimeInit( const char * fromPackage, const char * launchIntentJSON, const char * launchIntentURI );
	virtual void		OneTimeShutdown();
	virtual Matrix4f 	DrawEyeView( const int eye, const float fovDegrees );
	virtual Matrix4f 	Frame( VrFrame vrFrame );
	virtual void		Command( const char * msg );

	OvrSceneView		Scene;
};

#endif
