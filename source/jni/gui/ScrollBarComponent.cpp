/************************************************************************************

Filename    :   ScrollBarComponent.h
Content     :   A reusable component implementing a scroll bar.
Created     :   Jan 15, 2014
Authors     :   Warsam Osman

Copyright   :   Copyright 2014 Oculus VR, Inc. All Rights reserved.


*************************************************************************************/

#include "ScrollBarComponent.h"
#include "App.h"
#include "VRMenuMgr.h"
#include "VApkFile.h"
#include "VAlgorithm.h"

namespace NervGear {

char const * OvrScrollBarComponent::TYPE_NAME = "OvrScrollBarComponent";

static const V3Vectf FWD( 0.0f, 0.0f, -1.0f );
static const V3Vectf RIGHT( 1.0f, 0.0f, 0.0f );
static const V3Vectf DOWN( 0.0f, -1.0f, 0.0f );

static const float BASE_THUMB_WIDTH 		= 4.0f;
static const float THUMB_FROM_BASE_OFFSET 	= 0.001f;
static const float THUMB_SHRINK_FACTOR 		= 0.5f;

//==============================
// OvrScrollBarComponent::OvrScrollBarComponent
OvrScrollBarComponent::OvrScrollBarComponent( const VRMenuId_t rootId, const VRMenuId_t baseId,
	const VRMenuId_t thumbId, const int startElementIndex, const int numElements )
		: VRMenuComponent( VRMenuEventFlags_t( VRMENU_EVENT_FRAME_UPDATE ) )
		, m_fader( 0.0f )
		, m_fadeInRate( 1.0f / 0.2f )
		, m_fadeOutRate( 1.0f / 0.5f )
		, m_numOfItems( 0 )
		, m_scrollBarBaseWidth( 0.0f )
		, m_scrollBarBaseHeight( 0.0f )
		, m_scrollBarCurrentbaseLength( 0.0f )
		, m_scrollBarThumbWidth( 0.0f )
		, m_scrollBarThumbHeight( 0.0f )
		, m_scrollBarCurrentThumbLength( 0.0f )
		, m_scrollRootId( rootId )
		, m_scrollBarBaseId( baseId )
		, m_scrollBarThumbId( thumbId )
		, m_currentScrollState( SCROLL_STATE_HIDDEN )
		, m_isVertical( false )
{
}

void OvrScrollBarComponent::setScrollFrac( OvrVRMenuMgr & menuMgr, VRMenuObject * self, const float pos )
{
	if ( m_numOfItems <= 0 )
		return;

	// Updating thumb length when pulled beyond its bounds
	const float scrollEnd 			= (float)( m_numOfItems - 1 );
	const float outOfBoundsOffset 	= 0.5f;
	const float minThumbLength		= m_scrollBarCurrentThumbLength * THUMB_SHRINK_FACTOR;
	const float maxThumbLength		= m_scrollBarCurrentThumbLength;
	const float minThumbOffset		= 0.0f;
	const float maxThumbOffset		= ( maxThumbLength - minThumbLength ) * 0.5f;
	float currentThumbLength 		= m_scrollBarCurrentThumbLength;
	float thumbPosOffset 			= 0.0f;

	if ( pos < 0.0f )
	{
		currentThumbLength = LinearRangeMapFloat( pos, -outOfBoundsOffset, 0.0f, minThumbLength, maxThumbLength );
		thumbPosOffset = LinearRangeMapFloat( pos, -outOfBoundsOffset, 0.0f, -maxThumbOffset, minThumbOffset );
	}
	else if ( pos > scrollEnd )
	{
		currentThumbLength = LinearRangeMapFloat( pos, scrollEnd + outOfBoundsOffset, scrollEnd, minThumbLength, maxThumbLength );
		thumbPosOffset = LinearRangeMapFloat( pos, scrollEnd + outOfBoundsOffset, scrollEnd, maxThumbOffset, minThumbOffset );
	}

	float thumbWidth = m_scrollBarThumbWidth;
	float thumbHeight = m_scrollBarThumbHeight;

	if ( m_isVertical )
	{
		thumbHeight = currentThumbLength;
	}
	else
	{
		thumbWidth = currentThumbLength;
	}

	VRMenuObject * thumb = menuMgr.toObject( self->childHandleForId( menuMgr, m_scrollBarThumbId ) );
	if ( thumb != NULL && m_numOfItems > 0 )
	{
        thumb->setSurfaceDims( 0, V2Vectf( thumbWidth, thumbHeight ) );
		thumb->regenerateSurfaceGeometry( 0, false );
	}

	// Updating thumb position
	float clampedPos = VAlgorithm::Clamp( pos, 0.0f, (float)(m_numOfItems - 1) );
	float thumbPos = LinearRangeMapFloat( clampedPos, 0.0f, (float)(m_numOfItems - 1), 0.0f, ( m_scrollBarCurrentbaseLength - currentThumbLength ) );
	thumbPos -= ( m_scrollBarCurrentbaseLength - currentThumbLength ) * 0.5f;
	thumbPos += thumbPosOffset;
	thumbPos *= VRMenuObject::DEFAULT_TEXEL_SCALE;

	if ( thumb != NULL  )
	{
        V3Vectf direction = RIGHT;
		if ( m_isVertical )
		{
			direction = DOWN;
		}
		thumb->setLocalPosition( ( direction * thumbPos ) - ( FWD * THUMB_FROM_BASE_OFFSET ) );
	}
}

void OvrScrollBarComponent::updateScrollBar( OvrVRMenuMgr & menuMgr, VRMenuObject * self, const int numElements )
{
	m_numOfItems = numElements;

	if ( m_isVertical )
	{
		m_scrollBarCurrentbaseLength = m_scrollBarBaseHeight;
	}
	else
	{
		m_scrollBarCurrentbaseLength = m_scrollBarBaseWidth;
	}

	m_scrollBarCurrentThumbLength = m_scrollBarCurrentbaseLength / m_numOfItems;
	m_scrollBarCurrentThumbLength =  ( m_scrollBarCurrentThumbLength < BASE_THUMB_WIDTH ) ? BASE_THUMB_WIDTH : m_scrollBarCurrentThumbLength;

	if ( m_isVertical )
	{
		m_scrollBarThumbHeight = m_scrollBarCurrentThumbLength;
	}
	else
	{
		m_scrollBarThumbWidth = m_scrollBarCurrentThumbLength;
	}

	VRMenuObject * thumb = menuMgr.toObject( self->childHandleForId( menuMgr, m_scrollBarThumbId ) );
	if ( thumb != NULL && m_numOfItems > 0 )
	{
        thumb->setSurfaceDims( 0, V2Vectf( m_scrollBarThumbWidth, m_scrollBarThumbHeight ) );
		thumb->regenerateSurfaceGeometry( 0, false );
	}

	VRMenuObject * base = menuMgr.toObject( self->childHandleForId( menuMgr, m_scrollBarBaseId ) );
	if ( thumb != NULL )
	{
        base->setSurfaceDims( 0, V2Vectf( m_scrollBarBaseWidth, m_scrollBarBaseHeight ) );
		base->regenerateSurfaceGeometry( 0, false );
	}
}

void OvrScrollBarComponent::setBaseColor( OvrVRMenuMgr & menuMgr, VRMenuObject * self, const V4Vectf & color )
{
	// Set alpha on the base - move this to somewhere more explicit if needed
	VRMenuObject * base = menuMgr.toObject( self->childHandleForId( menuMgr, m_scrollBarBaseId ) );
	if ( base != NULL )
	{
		base->setSurfaceColor( 0, color );
	}
}

enum eScrollBarImage
{
	SCROLLBAR_IMAGE_BASE,
	SCROLLBAR_IMAGE_THUMB,
	SCROLLBAR_IMAGE_MAX
};

VString GetImage( eScrollBarImage const type, const bool vertical )
{
	static char const * images[ SCROLLBAR_IMAGE_MAX ] =
	{
		"res/raw/scrollbar_base_%s.png",
		"res/raw/scrollbar_thumb_%s.png",
	};

	char buff[ 256 ];
    sprintf(buff,images[type], vertical ? "vert" : "horz" );
	return VString( buff );
}

void OvrScrollBarComponent::getScrollBarParms( VRMenu & menu, float scrollBarLength, const VRMenuId_t parentId, const VRMenuId_t rootId, const VRMenuId_t xformId,
    const VRMenuId_t baseId, const VRMenuId_t thumbId, const VPosf & rootLocalPose, const VPosf & xformPose, const int startElementIndex,
    const int numElements, const bool verticalBar, const V4Vectf & thumbBorder, VArray< const VRMenuObjectParms* > & parms )
{
	// Build up the scrollbar parms
	OvrScrollBarComponent * scrollComponent = new OvrScrollBarComponent( rootId, baseId, thumbId, startElementIndex, numElements );
	scrollComponent->setVertical( verticalBar );

	// parms for the root object that holds all the scrollbar components
	{
		VArray< VRMenuComponent* > comps;
		comps.append( scrollComponent );
		VArray< VRMenuSurfaceParms > surfParms;
		char const * text = "scrollBarRoot";
        V3Vectf scale( 1.0f );
        VPosf pose( rootLocalPose );
        VPosf textPose( VQuatf(), V3Vectf( 0.0f ) );
        V3Vectf textScale( 1.0f );
		VRMenuFontParms fontParms;
		VRMenuObjectFlags_t objectFlags( VRMENUOBJECT_DONT_HIT_ALL );
		objectFlags |= VRMENUOBJECT_DONT_RENDER_TEXT;
		VRMenuObjectInitFlags_t initFlags( VRMENUOBJECT_INIT_FORCE_POSITION  );
		VRMenuObjectParms * itemParms = new VRMenuObjectParms( VRMENU_CONTAINER, comps,
			surfParms, text, pose, scale, textPose, textScale, fontParms, rootId,
			objectFlags, initFlags );
		itemParms->ParentId = parentId;
		parms.append( itemParms );
	}

	// add parms for the object that serves as a transform
	{
		VArray< VRMenuComponent* > comps;
		VArray< VRMenuSurfaceParms > surfParms;
		char const * text = "scrollBarTransform";
        V3Vectf scale( 1.0f );
        VPosf pose( xformPose );
        VPosf textPose( VQuatf(), V3Vectf( 0.0f ) );
        V3Vectf textScale( 1.0f );
		VRMenuFontParms fontParms;
		VRMenuObjectFlags_t objectFlags( VRMENUOBJECT_DONT_HIT_ALL );
		objectFlags |= VRMENUOBJECT_DONT_RENDER_TEXT;
		VRMenuObjectInitFlags_t initFlags( VRMENUOBJECT_INIT_FORCE_POSITION );
		VRMenuObjectParms * itemParms = new VRMenuObjectParms( VRMENU_CONTAINER, comps,
			surfParms, text, pose, scale, textPose, textScale, fontParms, xformId,
			objectFlags, initFlags );
		itemParms->ParentId = rootId;
		parms.append( itemParms );
	}

	// add parms for the base image that underlays the whole scrollbar
	{
		int sbWidth, sbHeight = 0;
		GLuint sbTexture = NervGear::LoadTextureFromApplicationPackage( GetImage( SCROLLBAR_IMAGE_BASE, verticalBar ), TextureFlags_t( TEXTUREFLAG_NO_DEFAULT ), sbWidth, sbHeight );
		if ( verticalBar )
		{
			scrollComponent->setScrollBarBaseWidth( (float)( sbWidth ) );
			scrollComponent->setScrollBarBaseHeight( scrollBarLength );
		}
		else
		{
			scrollComponent->setScrollBarBaseWidth( scrollBarLength );
			scrollComponent->setScrollBarBaseHeight( (float)( sbHeight ) );
		}

		VArray< VRMenuComponent* > comps;
		VArray< VRMenuSurfaceParms > surfParms;
		char const * text = "scrollBase";
		VRMenuSurfaceParms baseParms( text,
				sbTexture, sbWidth, sbHeight, SURFACE_TEXTURE_DIFFUSE,
				0, 0, 0, SURFACE_TEXTURE_MAX,
				0, 0, 0, SURFACE_TEXTURE_MAX );
		surfParms.append( baseParms );
        V3Vectf scale( 1.0f );
        VPosf pose( VQuatf(), V3Vectf( 0.0f ) );
        VPosf textPose( VQuatf(), V3Vectf( 0.0f ) );
        V3Vectf textScale( 1.0f );
		VRMenuFontParms fontParms;
		VRMenuObjectFlags_t objectFlags( VRMENUOBJECT_DONT_HIT_ALL );
		objectFlags |= VRMENUOBJECT_DONT_RENDER_TEXT;
		VRMenuObjectInitFlags_t initFlags( VRMENUOBJECT_INIT_FORCE_POSITION  );
		VRMenuObjectParms * itemParms = new VRMenuObjectParms( VRMENU_BUTTON, comps,
			surfParms, text, pose, scale, textPose, textScale, fontParms, baseId,
			objectFlags, initFlags );
		itemParms->ParentId = xformId;
		parms.append( itemParms );
	}

	// add parms for the thumb image of the scrollbar
	{
		int stWidth, stHeight = 0;
		GLuint stTexture = NervGear::LoadTextureFromApplicationPackage( GetImage( SCROLLBAR_IMAGE_THUMB, verticalBar ), TextureFlags_t( TEXTUREFLAG_NO_DEFAULT ), stWidth, stHeight );
		scrollComponent->setScrollBarThumbWidth(  (float)( stWidth ) );
		scrollComponent->setScrollBarThumbHeight( (float)( stHeight ) );

		VArray< VRMenuComponent* > comps;
		VArray< VRMenuSurfaceParms > surfParms;
		char const * text = "scrollThumb";
		VRMenuSurfaceParms thumbParms( text,
				stTexture, stWidth, stHeight, SURFACE_TEXTURE_DIFFUSE,
				0, 0, 0, SURFACE_TEXTURE_MAX,
				0, 0, 0, SURFACE_TEXTURE_MAX );
		//thumbParms.Border = thumbBorder;
		surfParms.append( thumbParms );
        V3Vectf scale( 1.0f );
        VPosf pose( VQuatf(), -FWD * THUMB_FROM_BASE_OFFSET );
		// Since we use left aligned anchors on the base and thumb, we offset the root once to center the scrollbar
        VPosf textPose( VQuatf(), V3Vectf( 0.0f ) );
        V3Vectf textScale( 1.0f );
		VRMenuFontParms fontParms;
		VRMenuObjectFlags_t objectFlags( VRMENUOBJECT_DONT_HIT_ALL );
		objectFlags |= VRMENUOBJECT_DONT_RENDER_TEXT;
		objectFlags |= VRMENUOBJECT_FLAG_POLYGON_OFFSET;
		VRMenuObjectInitFlags_t initFlags( VRMENUOBJECT_INIT_FORCE_POSITION  );
		VRMenuObjectParms * itemParms = new VRMenuObjectParms( VRMENU_BUTTON, comps,
			surfParms, text, pose, scale, textPose, textScale, fontParms, thumbId,
			objectFlags, initFlags );
		itemParms->ParentId = xformId;
		parms.append( itemParms );
	}
}

//==============================
// OvrScrollBarComponent::OnEvent_Impl
eMsgStatus OvrScrollBarComponent::onEventImpl( App * app, VrFrame const & vrFrame, OvrVRMenuMgr & menuMgr,
		VRMenuObject * self, VRMenuEvent const & event )
{
	switch ( event.eventType )
	{
		case VRMENU_EVENT_FRAME_UPDATE:
			return onFrameUpdate( app, vrFrame, menuMgr, self, event );
		default:
			vAssert( false );
			return MSG_STATUS_ALIVE;
	}
    return MSG_STATUS_CONSUMED;
}

const char * stateNames [ ] =
{
	"SCROLL_STATE_NONE",
	"SCROLL_STATE_FADE_IN",
	"SCROLL_STATE_VISIBLE",
	"SCROLL_STATE_FADE_OUT",
	"SCROLL_STATE_HIDDEN",
	"NUM_SCROLL_STATES"
};

const char* StateString( const OvrScrollBarComponent::eScrollBarState state )
{
	vAssert( state >= 0 && state < OvrScrollBarComponent::NUM_SCROLL_STATES );
	return stateNames[ state ];
}

//==============================
// OvrScrollBarComponent::SetScrollState
void OvrScrollBarComponent::setScrollState( VRMenuObject * self, const eScrollBarState state )
{
	eScrollBarState lastState = m_currentScrollState;
	m_currentScrollState = state;
	if ( m_currentScrollState == lastState )
	{
		return;
	}

	switch ( m_currentScrollState )
	{
	case SCROLL_STATE_NONE:
		break;
	case SCROLL_STATE_FADE_IN:
		if ( lastState == SCROLL_STATE_HIDDEN || lastState == SCROLL_STATE_FADE_OUT )
		{
			vInfo(StateString( lastState ) << "to" << StateString( m_currentScrollState ));
			m_fader.startFadeIn();
		}
		break;
	case SCROLL_STATE_VISIBLE:
		self->setVisible( true );
		break;
	case SCROLL_STATE_FADE_OUT:
		if ( lastState == SCROLL_STATE_VISIBLE || lastState == SCROLL_STATE_FADE_IN )
		{
			vInfo(StateString( lastState ) << "to" << StateString( m_currentScrollState ));
			m_fader.startFadeOut();
		}
		break;
	case SCROLL_STATE_HIDDEN:
		self->setVisible( false );
		break;
	default:
		vAssert( false );
		break;
	}
}

//==============================
// OvrScrollBarComponent::OnFrameUpdate
eMsgStatus OvrScrollBarComponent::onFrameUpdate( App * app, VrFrame const & vrFrame, OvrVRMenuMgr & menuMgr,
		VRMenuObject * self, VRMenuEvent const & event )
{
	vAssert( self != NULL );
    if ( m_fader.fadeState() != Fader::FADE_NONE )
	{
        const float fadeRate = ( m_fader.fadeState() == Fader::FADE_IN ) ? m_fadeInRate : m_fadeOutRate;
		m_fader.update( fadeRate, vrFrame.DeltaSeconds );
		const float CurrentFadeLevel = m_fader.finalAlpha();
        self->setColor( V4Vectf( 1.0f, 1.0f, 1.0f, CurrentFadeLevel ) );
	}
	else if ( m_fader.fadeAlpha() == 1.0f )
	{
		setScrollState( self, SCROLL_STATE_VISIBLE );
	}
	else if ( m_fader.fadeAlpha() == 0.0f )
	{
		setScrollState( self, SCROLL_STATE_HIDDEN );
	}

	return MSG_STATUS_ALIVE;
}

} // namespace NervGear
