#ifndef OVRMEDIASURFACE_H
#define OVRMEDIASURFACE_H

#include <jni.h>
#include "VEglDriver.h"
#include "VGlGeometry.h"
#include "VGlShader.h"
#include "SurfaceTexture.h"

NV_NAMESPACE_BEGIN

class MediaSurface
{
public:
					MediaSurface();

	// Must be called on the launch thread so Android Java classes
	// can be looked up.
	void			Init( JNIEnv * jni );
	void			Shutdown();

	// Designates the target texId that Update will render to.
	jobject			Bind( int toTexId );

	// Must be called with a current OpenGL context
	void			Update();

	JNIEnv * 		jni;
	SurfaceTexture	* AndroidSurfaceTexture;
	VGlShader		CopyMovieProgram;
	VGlGeometry		UnitSquare;
	jobject			SurfaceObject;
	long long		LastSurfaceTexNanoTimeStamp;
	int				TexId;
	int				TexIdWidth;
	int				TexIdHeight;
	GLuint			Fbo;
};

NV_NAMESPACE_END

#endif // OVRMEDIASURFACE_H
