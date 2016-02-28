/************************************************************************************

Filename    :   VRMenuMgr.h
Content     :   Menuing system for VR apps.
Created     :   May 23, 2014
Authors     :   Jonathan E. Wright

Copyright   :   Copyright 2014 Oculus VR, LLC. All Rights reserved.


*************************************************************************************/

#if !defined( OVR_VRMenuMgr_h )
#define OVR_VRMenuMgr_h

#include "VRMenuObject.h"

namespace NervGear {

class BitmapFont;
class BitmapFontSurface;
struct GlProgram;
class OvrDebugLines;

enum eGUIProgramType
{
	PROGRAM_DIFFUSE_ONLY,				// has a diffuse only
	PROGRAM_ADDITIVE_ONLY,
	PROGRAM_DIFFUSE_PLUS_ADDITIVE,		// has a diffuse and an additive
	PROGRAM_DIFFUSE_COLOR_RAMP,			// has a diffuse and color ramp, and color ramp target is the diffuse
	PROGRAM_DIFFUSE_COLOR_RAMP_TARGET,	// has diffuse, color ramp, and a separate color ramp target
	PROGRAM_DIFFUSE_COMPOSITE,			// has two diffuse, with the second composited onto the first using it's alpha mask
	PROGRAM_MAX							// some other combo not supported, or no texture maps at all
};

//==============================================================
// OvrVRMenuMgr
class OvrVRMenuMgr
{
public:
	virtual						~OvrVRMenuMgr() { }

    static OvrVRMenuMgr *       Create();
    static void                 Free( OvrVRMenuMgr * & mgr );

	// Initialize the VRMenu system
    virtual void				init() = 0;
	// Shutdown the VRMenu syatem
    virtual void				shutdown() = 0;

	// creates a new menu object
    virtual menuHandle_t		createObject( VRMenuObjectParms const & parms ) = 0;
	// Frees a menu object.  If the object is a child of a parent object, this will
	// also remove the child from the parent.
    virtual void				freeObject( menuHandle_t const handle ) = 0;
	// Returns true if the handle is valid.
    virtual bool				isValid( menuHandle_t const handle ) const = 0;
	// Return the object for a menu handle or NULL if the object does not exist or the
	// handle is invalid;
    virtual VRMenuObject	*	toObject( menuHandle_t const handle ) const = 0;

	// Called once at the very beginning o f the frame before any submissions.
    virtual void				beginFrame() = 0;

	// Submits the specified menu object and its children
    virtual void				submitForRendering( OvrDebugLines & debugLines, BitmapFont const & font,
                                        BitmapFontSurface & fontSurface, menuHandle_t const handle, 
                                        Posef const & worldPose, VRMenuRenderFlags_t const & flags ) = 0;

	// Call once per frame before rendering to sort surfaces.
    virtual void				finish( Matrix4f const & viewMatrix ) = 0;

	// Render's all objects that have been submitted on the current frame.
    virtual void				renderSubmitted( Matrix4f const & worldMVP, Matrix4f const & viewMatrix ) const = 0;

    virtual GlProgram const *   getGUIGlProgram( eGUIProgramType const programType ) const = 0;
};

} // namespace NervGear

#endif // OVR_VRMenuMgr_h
