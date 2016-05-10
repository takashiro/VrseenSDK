/************************************************************************************

Filename    :   FolderBrowser.h
Content     :   A menu for browsing a hierarchy of folders with items represented by thumbnails.
Created     :   July 25, 2014
Authors     :   Jonathan E. Wright, Warsam Osman, Madhu Kalva

Copyright   :   Copyright 2014 Oculus VR, LLC. All Rights reserved.


*************************************************************************************/

#if !defined( OVR_FolderBrowser_h )
#define OVR_FolderBrowser_h

#include "VRMenu.h"
#include "VEventLoop.h"
#include "MetaDataManager.h"
#include "ScrollManager.h"
#include "VArray.h"
NV_NAMESPACE_BEGIN

class OvrFolderBrowserRootComponent;
class OvrFolderSwipeComponent;
class OvrFolderBrowserSwipeComponent;
class OvrDefaultComponent;
class OvrPanel_OnUp;

//==============================================================
// OvrFolderBrowser
class OvrFolderBrowser : public VRMenu
{
public:
	struct PanelView
	{
		PanelView()
            : id( -1 )
		{}

        menuHandle_t			handle;				// Handle to the panel
        int						id;					// Unique id for thumbnail loading
        V2Vectf				size;				// Thumbnail texture size
	};

	struct FolderView
	{
		FolderView( const VString & name, const VString & tag )
            : categoryTag( tag )
            , localizedName( name )
            , maxRotation( 0.0f )
		{}
        const VString			categoryTag;
        const VString			localizedName;		// Store for rebuild of title
        menuHandle_t			handle;				// Handle to main root - parent to both Title and Panels
        menuHandle_t			titleRootHandle;	// Handle to the folder title root
        menuHandle_t			titleHandle;		// Handle to the folder title
        menuHandle_t			swipeHandle;		// Handle to root for panels
        menuHandle_t			scrollBarHandle;	// Handle to the scrollbar object
        menuHandle_t			wrapIndicatorHandle;
        float					maxRotation;		// Used by SwipeComponent
        VArray<PanelView>		panels;
	};

	static char const *			MENU_NAME;
    static  VRMenuId_t			ID_CENTER_ROOT;

    virtual void						oneTimeInit();
    // Builds the menu view using the passed in model. Builds only what's marked dirty - can be called multiple times.
    virtual void						buildDirtyMenu( OvrMetaData & metaData );
	// Swiping when the menu is inactive can cycle through files in
	// the directory.  Step can be 1 or -1.
    virtual	const OvrMetaDatum *		nextFileInDirectory( const int step );
	// Called if touch up is activated without a focused item
	// User returns true if consumed
    virtual bool						onTouchUpNoFocused()							{ return false; }

    FolderView *						getFolderView( const VString & categoryTag );
    FolderView *						getFolderView( int index );
    VEventLoop &						textureCommands()							{ return m_textureCommands;  }
    void								setPanelTextSpacingScale( const float scale )	{ m_panelTextSpacingScale = scale; }
    void								setFolderTitleSpacingScale( const float scale ) { m_folderTitleSpacingScale = scale; }
    void								setScrollBarSpacingScale( const float scale )	{ m_scrollBarSpacingScale = scale; }
    void								setScrollBarRadiusScale( const float scale )	{ m_scrollBarRadiusScale = scale; }
    void								setAllowPanelTouchUp( const bool allow )		{ m_allowPanelTouchUp = allow; }

	enum RootDirection
	{
		MOVE_ROOT_NONE,
		MOVE_ROOT_UP,
		MOVE_ROOT_DOWN
	};
	// Attempts to scroll - returns false if not possible due to being at boundary or currently scrolling
    bool								scrollRoot( const RootDirection direction, bool hideScrollHint = false );

	enum CategoryDirection
	{
		MOVE_PANELS_NONE,
		MOVE_PANELS_LEFT,
		MOVE_PANELS_RIGHT
	};
    bool								rotateCategory( const CategoryDirection direction );
    void								setCategoryRotation( const int folderIndex, const int panelIndex );

    void 								touchDown();
    void 								touchUp();
    void 								touchRelative( V3Vectf touchPos );

	// Accessors
    const FolderView *					getFolderView( int index ) const;
    int									numFolders() const					{ return m_folders.length(); }
    int									circumferencePanelSlots() const		{ return m_circumferencePanelSlots; }
    float								radius() const						{ return m_radius; }
    float 								panelHeight() const					{ return m_panelHeight; }
    float 								panelWidth() const					{ return m_panelWidth; }
	// The index for the folder that's at the center - considered the actively selected folder
    int									activeFolderIndex() const;
	// Returns the number of panels shown per folder - or Swipe Component
    int									numSwipePanels() const				{ return m_numSwipePanels; }
    unsigned							thumbWidth() const					{ return m_thumbWidth; }
    unsigned							thumbHeight() const					{ return m_thumbHeight;  }
    bool								hasNoMedia() const						{ return m_noMedia; }
    bool								gazingAtMenu() const;
    void								setWrapIndicatorVisible( FolderView & folder, const bool visible );

    OvrFolderSwipeComponent * 			swipeComponentForActiveFolder();

    void								setScrollHintVisible( const bool visible );
    void								setActiveFolder( int folderIdx );

    eScrollDirectionLockType			controllerDirectionLock()				{ return m_controllerDirectionLock; }
    eScrollDirectionLockType			touchDirectionLock()						{ return m_touchDirectionLocked; }
    bool								applyThumbAntialiasing( unsigned char * inOutBuffer, int width, int height ) const;

protected:
	OvrFolderBrowser( App * app,
				OvrMetaData & metaData,
				float panelWidth,
				float panelHeight,
				float radius,
				unsigned numSwipePanels,
				unsigned thumbWidth,
				unsigned thumbHeight );

    virtual ~OvrFolderBrowser();

	//================================================================================
	// Subclass protected interface

	// Called from the base class when building a cateory.
    virtual VString				getCategoryTitle(const VString &key, const VString &defaultStr ) const = 0;

	// Called from the base class when building a panel
    virtual VString				getPanelTitle( const OvrMetaDatum & panelData ) const = 0;

	// Called when a panel is activated
    virtual void				onPanelActivated( const OvrMetaDatum * panelData ) = 0;

	// Called on a background thread
	// The returned memory buffer will be free()'d after writing the thumbnail.
	// Return NULL if the thumbnail couldn't be created.
    virtual unsigned char *		createAndCacheThumbnail( const char * soureFile, const char * cacheDestinationFile, int & outWidth, int & outHeight ) = 0;

	// Called on a background thread to load thumbnail
    virtual	unsigned char *		loadThumbnail( const char * filename, int & width, int & height ) = 0;

	// Returns the proper thumbnail URL
    virtual VString				thumbUrl( const OvrMetaDatum * item ) { return item->url; }

	// Adds thumbnail extension to a file to find/create its thumbnail
    virtual VString				thumbName( const VString & s ) = 0;

	// Media not found - have subclass set the title, image and caption to display
    virtual void				onMediaNotFound( App * app, VString & title, VString & imageFile, VString & message ) = 0;

	// Optional interface
	//
	// Request external thumbnail - called on main thread
    virtual uchar *retrieveRemoteThumbnail(
            const VString &url,
            const VString &cacheDestinationFile,
			int folderId,
			int panelId,
			int & outWidth,
            int & outHeight );

	// If we fail to load one type of thumbnail, try an alternative
    virtual VString				alternateThumbName( const VString & s ) { return VString(); }

	// Called on opening menu
    virtual void				onBrowserOpen() {}

	//================================================================================

	// OnEnterMenuRootAdjust is set to be performed the
	// next time the menu is opened to adjust for a potentially deleted or added category
    void						setRootAdjust( const RootDirection dir )	{ m_onEnterMenuRootAdjust = dir;  }
    RootDirection				rootAdjust() const						{ return m_onEnterMenuRootAdjust; }

	// Rebuilds a folder using passed in data
    void						rebuildFolder( OvrMetaData & metaData, const int folderIndex, const VArray< const OvrMetaDatum * > & data );

protected:
    App *						m_app;
    int							m_mediaCount; // Used to determine if no media was loaded

private:
	static void *		ThumbnailThread( void * v );
    pthread_t			m_thumbnailThreadId;
    void				loadThumbnailToTexture(const VEvent &thumbnailCommand );

	friend class OvrPanel_OnUp;
    void				onPanelUp( const OvrMetaDatum * data );

    virtual void        frameImpl( App * app, VFrame const & vrFrame, OvrVRMenuMgr & menuMgr, BitmapFont const & font,
                                        BitmapFontSurface & fontSurface, gazeCursorUserId_t const gazeUserId );
    virtual void		openImpl( App * app, OvrGazeCursor & gazeCursor );

    void				buildFolder( OvrMetaData::Category & category, FolderView * const folder, const OvrMetaData & metaData, VRMenuId_t foldersRootId, const int folderIndex );
    void				loadFolderPanels( const OvrMetaData & metaData, const OvrMetaData::Category & category, const int folderIndex, FolderView & folder,
							 VArray< VRMenuObjectParms const * > & outParms );
    void				addPanelToFolder( const OvrMetaDatum * panoData, const int folderIndex, FolderView & folder, VArray< VRMenuObjectParms const * >& outParms );
    void				displaceFolder( int index, const V3Vectf & direction, float distance, bool startOffSelf = false );
    void				updateFolderTitle( const FolderView * folder  );
    float				calcFolderMaxRotation( const FolderView * folder ) const;

	// Members
    VRMenuId_t			m_foldersRootId;
    float				m_panelWidth;
    float				m_panelHeight;
    int					m_thumbWidth;
    int					m_thumbHeight;
    float				m_panelTextSpacingScale;		// Panel text placed with vertical position of -panelHeight * PanelTextSpacingScale
    float				m_folderTitleSpacingScale;	// Folder title placed with vertical position of PanelHeight * FolderTitleSpacingScale
    float				m_scrollBarSpacingScale;		// Scroll bar placed with vertical position of PanelHeight * ScrollBarSpacingScale
    float				m_scrollBarRadiusScale;		// Scroll bar placed with horizontal position of FWD * Radius * ScrollBarRadiusScale

    int					m_circumferencePanelSlots;
    unsigned			m_numSwipePanels;
    float				m_radius;
    float				m_visiblePanelsArcAngle;
    bool				m_swipeHeldDown;
    float				m_debounceTime;
    bool				m_noMedia;
    bool				m_allowPanelTouchUp;

    VArray< FolderView * >	m_folders;

    menuHandle_t 		m_scrollSuggestionRootHandle;

    RootDirection		m_onEnterMenuRootAdjust;

	// Checked at Frame() time for commands from the thumbnail/create thread
    VEventLoop		m_textureCommands;

	// Create / load thumbnails by background thread
	struct OvrCreateThumbCmd
	{
        VString sourceImagePath;
        VString thumbDestination;
        VVariantArray loadCmd;
	};
    VArray< OvrCreateThumbCmd > m_thumbCreateAndLoadCommands;
    VEventLoop		m_backgroundCommands;
    VArray< VString >		m_thumbSearchPaths;
    VString				m_appCachePath;

	// Keep a reference to Panel texture used for AA alpha when creating thumbnails
	static unsigned char *		ThumbPanelBG;

	// Default panel textures (base and highlight) - loaded once
    GLuint				m_defaultPanelTextureIds[ 2 ];

	// Restricted Scrolling
	static const float 						CONTROLER_COOL_DOWN; // Controller goes to rest very frequently so cool down helps
    eScrollDirectionLockType				m_controllerDirectionLock;
	float									LastControllerInputTimeStamp;

	static const float 						SCROLL_DIRECTION_DECIDING_DISTANCE;
    bool									m_isTouchDownPosistionTracked;
    V3Vectf 								m_touchDownPosistion; // First event in touch relative is considered as touch down position
    eScrollDirectionLockType				m_touchDirectionLocked;

};

NV_NAMESPACE_END

#endif // OVR_FolderBrowser_h
