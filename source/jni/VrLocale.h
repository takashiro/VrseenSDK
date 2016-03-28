#pragma once

#include <stdarg.h>
#include <jni.h>

#include "VString.h"

NV_NAMESPACE_BEGIN

class VrLocale
{
public:
	static char const *	LOCALIZED_KEY_PREFIX;
	static size_t		LOCALIZED_KEY_PREFIX_LEN;

	// Get's a localized UTF-8-encoded string from the string table.
	static bool 	GetString( JNIEnv* jni, jobject activityObject, char const * key, char const * defaultOut, VString & out );

	// Takes an ANSI string and returns an identifier that can be used as an Android string id.
    static VString	MakeStringId(const VString &str);

	// Localization : Returns xliff formatted string
	// These are set to const char * to make sure that's all that's passed in - we support up to 9, add more functions as needed
	static VString GetXliffFormattedString( const VString & inXliffStr, const char * arg1 );
	static VString GetXliffFormattedString( const VString & inXliffStr, const char * arg1, const char * arg2 );
	static VString GetXliffFormattedString( const VString & inXliffStr, const char * arg1, const char * arg2, const char * arg3 );

	static jclass VrActivityClass;
};

NV_NAMESPACE_END

