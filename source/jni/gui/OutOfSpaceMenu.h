/************************************************************************************

Filename    :   OutOfSpaceMenu.h
Content     :
Created     :   Feb 18, 2015
Authors     :   Madhu Kalva

Copyright   :   Copyright 2014 Oculus VR, LLC. All Rights reserved.

*************************************************************************************/
#ifndef OUTOFSPACEMENU_H_
#define OUTOFSPACEMENU_H_

#include "VRMenu.h"

namespace NervGear {

	class App;

	class OvrOutOfSpaceMenu : public VRMenu
	{
	public:
		static char const *	MENU_NAME;
		static OvrOutOfSpaceMenu * Create( App * app );

        void 	buildMenu( int memoryInKB );

	private:
		OvrOutOfSpaceMenu( App * app );
        App * m_app;
	};
}

#endif /* OUTOFSPACEMENU_H_ */
