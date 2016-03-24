#pragma once

#include "vglobal.h"
#include "VBasicmath.h"
#include "VConstants.h"

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
    virtual	void		    Render( VR4Matrixf const & mvp ) const = 0;

    virtual	void		    AddLine( const V3Vectf & start, const V3Vectf & end,
                                    const V4Vectf & startColor, const V4Vectf & endColor,
						    		const long long endFrame, const bool depthTest ) = 0;

    virtual void		    AddPoint( const V3Vectf & pos, const float size,
                                    const V4Vectf & color, const long long endFrame,
						    		const bool depthTest ) = 0;

	// Add a debug point without a specified color. The axis lines will use default
	// colors: X = red, Y = green, Z = blue (same as Maya).
    virtual void		    AddPoint( const V3Vectf & pos, const float size,
						    		const long long endFrame, const bool depthTest ) = 0;

    virtual void		    AddBounds( VPosf const & pose, VBoxf const & bounds, V4Vectf const & color ) = 0;
};

NV_NAMESPACE_END
