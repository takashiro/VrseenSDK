/************************************************************************************

Filename    :   PanoBrowser.h
Content     :   Subclass for panoramic image files
Created     :   August 13, 2014
Authors     :   John Carmack, Warsam Osman

Copyright   :   Copyright 2014 Oculus VR, LLC. All Rights reserved.

This source code is licensed under the BSD-style license found in the
LICENSE file in the Oculus360Photos/ directory. An additional grant
of patent rights can be found in the PATENTS file in the same directory.

************************************************************************************/

#ifndef OVR_PanoSwipeDir_h
#define OVR_PanoSwipeDir_h

#include "gui/FolderBrowser.h"

namespace NervGear
{
	class PanoBrowser : public OvrFolderBrowser
	{
	public:
		// only one of these every needs to be created
		static  PanoBrowser *		Create(
			App *					app,
			OvrMetaData &			metaData,
			unsigned				thumbWidth,
			float					horizontalPadding,
			unsigned				thumbHeight,
			float					verticalPadding,
			unsigned 				numSwipePanels,
			float					swipeRadius );

		// Swiping when the menu is inactive can cycle through files in
		// the directory.  Step can be 1 or -1.
        virtual	const OvrMetaDatum *nextFileInDirectory( const int step );
        void addToFavorites( const OvrMetaDatum * panoData );
        void removeFromFavorites( const OvrMetaDatum * panoData );

		// Called when a panel is activated
        void onPanelActivated( const OvrMetaDatum * panelData ) override;

		// Called on opening menu - used to rebuild favorites using FavoritesBuffer
        void onBrowserOpen() override;

		// Reload FavoritesBuffer with what's currently in favorites folder in FolderBrowser
		void						ReloadFavoritesBuffer();

		// Called on a background thread
		//
		// Create the thumbnail image for the file, which will
		// be saved out as a _thumb.jpg.
        unsigned char *createAndCacheThumbnail( const char * soureFile, const char * cacheDestinationFile, int & width, int & height ) override;

		// Called on a background thread to load thumbnail
        unsigned char *loadThumbnail( const char * filename, int & width, int & height ) override;

		// Adds thumbnail extension to a file to find/create its thumbnail
        VString thumbName( const VString & s ) override;

		// Called when no media found
        void onMediaNotFound( App * app, VString & title, VString & imageFile, VString & message ) override;

		// Called if touch up is activated without a focused item
        bool onTouchUpNoFocused() override;

        int numPanosInActive() const;

	protected:
		// Called from the base class when building category.
        VString getCategoryTitle( char const * key, char const * defaultStr ) const override;

		// Called from the base class when building a panel.
        VString getPanelTitle( const OvrMetaDatum & panelData ) const override;

	private:
		PanoBrowser(
			App * app,
			OvrMetaData & metaData,
			float panelWidth,
			float panelHeight,
			float radius,
			unsigned numSwipePanels,
			unsigned thumbWidth,
			unsigned thumbHeight )
			: OvrFolderBrowser( app, metaData, panelWidth, panelHeight, radius, numSwipePanels, thumbWidth, thumbHeight )
            , m_bufferDirty( false )
		{
		}

		virtual ~PanoBrowser()
		{
		}

		// Favorites buffer - this is built by PanoMenu and used to rebuild Favorite's menu
		// if an item in it is null, it's been deleted from favorites
		struct Favorite
		{
			Favorite()
            : data( nullptr )
            , isFavorite( false )
			{}
            const OvrMetaDatum *	data;
            bool					isFavorite;
		};
        Array< Favorite >	m_favoritesBuffer;
        bool				m_bufferDirty;
	};

	unsigned char * LoadPageJpg( const char * cbzFileName, int pageNum, int & width, int & height );

}

#endif	// OVR_PanoSwipeDir_h
