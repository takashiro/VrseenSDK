/************************************************************************************

Filename    :   VrLocale.h
Content     :   Localization and internationalization (i18n) functionality.
Created     :   11/24/2914
Authors     :   Jonathan Wright

Copyright   :   Copyright 2014 Oculus VR, LLC. All Rights reserved.

*************************************************************************************/

#if !defined( OVR_VRLOCALE_H )
#define OVR_VRLOCALE_H

#include "VString.h"
#include <stdarg.h>
#include "jni.h"

struct ovrMobile;

namespace NervGear {

//==============================================================
// VrLocale
//
// Holds all localization functions.
class VrLocale
{
public:
	static char const *	LOCALIZED_KEY_PREFIX;
	static size_t		LOCALIZED_KEY_PREFIX_LEN;

	// Get's a localized UTF-8-encoded string from the string table.
	static bool 	GetString( JNIEnv* jni, jobject activityObject, char const * key, char const * defaultOut, VString & out );

	// Takes a UTF8 string and returns an identifier that can be used as an Android string id.
	static VString	MakeStringIdFromUTF8( char const * str );

	// Takes an ANSI string and returns an identifier that can be used as an Android string id.
	static VString	MakeStringIdFromANSI( char const * str );

	// Localization : Returns xliff formatted string
	// These are set to const char * to make sure that's all that's passed in - we support up to 9, add more functions as needed
	static VString GetXliffFormattedString( const VString & inXliffStr, const char * arg1 );
	static VString GetXliffFormattedString( const VString & inXliffStr, const char * arg1, const char * arg2 );
	static VString GetXliffFormattedString( const VString & inXliffStr, const char * arg1, const char * arg2, const char * arg3 );

	static VString ToString( char const * fmt, float const f );
	static VString ToString( char const * fmt, int const i );

	static jclass VrActivityClass;
};

} // namespace NervGear

#endif	// OVR_VRLOCALE_H
