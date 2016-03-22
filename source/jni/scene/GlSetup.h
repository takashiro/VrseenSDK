//#pragma once
//
//#include "vglobal.h"
//#include "api/VGlOperation.h"
//
//NV_NAMESPACE_BEGIN
//
//struct eglSetup_t
//{
//	int			glEsVersion;		// 2 or 3
//    VGlOperation::GpuType		gpuType;
//	EGLDisplay	display;
//	EGLSurface	pbufferSurface;		// use to make context current when we don't have window surfaces
//	EGLConfig	config;
//	EGLContext	context;
//};
//
//// Create an appropriate config, a tiny pbuffer surface, and a context,
//// then make it current.  This combination can be used before and after
//// the actual window surfaces are available.
//// egl.context will be EGL_NO_CONTEXT if there was an error.
//// requestedGlEsVersion can be 2 or 3.  If 3 is requested, 2 might still
//// be returned in eglSetup_t.glEsVersion.
////
//// If contextPriority == EGL_CONTEXT_PRIORITY_MID_IMG, then no priority
//// attribute will be set, otherwise EGL_CONTEXT_PRIORITY_LEVEL_IMG will
//// be set with contextPriority.
//eglSetup_t	EglSetup( const EGLContext shareContext,
//		const int requestedGlEsVersion,
//		const int redBits, const int greenBits, const int blueBits,
//		const int depthBits, const int multisamples,
//		const GLuint contextPriority );
//
//void	EglShutdown( eglSetup_t & eglr );
//
//NV_NAMESPACE_END
