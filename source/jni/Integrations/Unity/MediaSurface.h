#pragma once

#include "vglobal.h"

#include <jni.h>
#include "android/GlUtils.h"
#include "GlGeometry.h"
#include "../../api/VGlShader.h"
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
	GlGeometry		UnitSquare;
	jobject			SurfaceObject;
	long long		LastSurfaceTexNanoTimeStamp;
	int				TexId;
	int				TexIdWidth;
	int				TexIdHeight;
	GLuint			Fbo;
};

NV_NAMESPACE_END


