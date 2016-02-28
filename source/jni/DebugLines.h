/************************************************************************************

Filename    :   DebugLines.h
Content     :   Class that manages and renders debug lines.
Created     :   April 22, 2014
Authors     :   Jonathan E. Wright

Copyright   :   Copyright 2014 Oculus VR, LLC. All Rights reserved.

*************************************************************************************/

#pragma once

#include "vglobal.h"
#include "VMath.h"

NV_NAMESPACE_BEGIN

//==============================================================
// OvrDebugLines
class OvrDebugLines
{
public:
	virtual				    ~OvrDebugLines() { }

    static OvrDebugLines *  Create();
    static void             Free( OvrDebugLines * & debugLines );

	virtual	void		    Init() = 0;
	virtual	void		    Shutdown() = 0;

	virtual	void		    BeginFrame( const long long frameNum ) = 0;
	virtual	void		    Render( Matrix4f const & mvp ) const = 0;

	virtual	void		    AddLine( const Vector3f & start, const Vector3f & end,
						    		const Vector4f & startColor, const Vector4f & endColor,
						    		const long long endFrame, const bool depthTest ) = 0;

	virtual void		    AddPoint( const Vector3f & pos, const float size,
						    		const Vector4f & color, const long long endFrame,
						    		const bool depthTest ) = 0;

	// Add a debug point without a specified color. The axis lines will use default
	// colors: X = red, Y = green, Z = blue (same as Maya).
	virtual void		    AddPoint( const Vector3f & pos, const float size,
						    		const long long endFrame, const bool depthTest ) = 0;

	virtual void		    AddBounds( Posef const & pose, Bounds3f const & bounds, Vector4f const & color ) = 0;
};

NV_NAMESPACE_END
