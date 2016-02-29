/************************************************************************************

Filename    :   LocalPreferences.cpp
Content     :   Trivial local key / value data store
Created     :   August, 2014
Authors     :   John Carmack

Copyright   :   Copyright 2014 Oculus VR, LLC. All Rights reserved.


*************************************************************************************/

#include "LocalPreferences.h"

#include <stdio.h>

#include "Array.h"
#include "VString.h"
#include "MemBuffer.h"
#include "Android/LogUtils.h"

static bool AllowLocalPreferencesFile = false;
static const char * localPrefsFile = "/sdcard/.oculusprefs";

class KeyPair
{
public:
	NervGear::String	Key;
	NervGear::String	Value;
};

NervGear::Array<KeyPair>	LocalPreferences;

static NervGear::String ParseToken( const char * txt, const int start, const int stop, int & stopPoint  )
{
	// skip leading whitespace
	int	startPoint = start;
	for ( ; startPoint < stop && txt[startPoint] <= ' ' ; startPoint++ )
	{
	}

	for ( stopPoint = startPoint ; stopPoint < stop && txt[stopPoint] > ' ' ; stopPoint++ )
	{
	}

	return NervGear::String( txt + startPoint, stopPoint - startPoint );
}

// Called on each resume, synchronously fetches the data.
void ovr_UpdateLocalPreferences()
{
	if ( !AllowLocalPreferencesFile )
	{
		return;
	}

	LocalPreferences.clear();
	NervGear::MemBufferFile	file( localPrefsFile );

	// Don't bother checking for duplicate names, later ones
	// will be ignored and removed on write.
	const char * txt = (const char *)file.buffer;
	LOG( "%s is length %i", localPrefsFile, file.length );
	for ( int ofs = 0 ; ofs < file.length ; )
	{
		KeyPair	kp;
		int stopPos;
		kp.Key = ParseToken( txt, ofs, file.length, stopPos );
		ofs = stopPos;
		kp.Value = ParseToken( txt, ofs, file.length, stopPos );
		ofs = stopPos;

		if ( kp.Key.size() > 0 )
		{
			LOG( "%s = %s", kp.Key.toCString(), kp.Value.toCString() );
			LocalPreferences.append( kp );
		}
	}
}

// Query the in-memory preferences for a key / value pair.
// If the returned string is not defaultKeyValue, it will remain valid until the next ovr_UpdateLocalPreferences().
const char * ovr_GetLocalPreferenceValueForKey( const char * keyName, const char * defaultKeyValue )
{
	for ( int i = 0 ; i < LocalPreferences.sizeInt() ; i++ )
	{
		if ( 0 == LocalPreferences[i].Key.CompareNoCase( keyName ) )
		{
            LOG( "Key %s = %s", keyName, LocalPreferences[i].Value.toCString() );
            return LocalPreferences[i].Value.toCString();
		}
	}
	//LOG( "Key %s not found, returning default %s", keyName, defaultKeyValue );

	return defaultKeyValue;
}

// Updates the in-memory data and synchronously writes it to storage.
void ovr_SetLocalPreferenceValueForKey( const char * keyName, const char * keyValue )
{
	LOG( "Set( %s, %s )", keyName, keyValue );

	int i = 0;
	for ( ; i < LocalPreferences.sizeInt() ; i++ )
	{
        if ( !strcmp( keyName, LocalPreferences[i].Key.toCString() ) )
		{
			LocalPreferences[i].Value = keyValue;
		}
	}
	if ( i == LocalPreferences.sizeInt() )
	{
		KeyPair	kp;
		kp.Key = keyName;
		kp.Value = keyValue;
		LocalPreferences.append( kp );
	}

	// don't write out if prefs are disabled because we could overwrite with default values
	if ( !AllowLocalPreferencesFile )
	{
		return;
	}

	// write the file
	FILE * f = fopen( localPrefsFile, "w" );
	if ( !f )
	{
		LOG( "Couldn't open %s for writing", localPrefsFile );
		return;
	}
	for ( int i = 0 ; i < LocalPreferences.sizeInt() ; i++ )
	{
        fprintf( f, "%s %s", LocalPreferences[i].Key.toCString(), LocalPreferences[i].Value.toCString() );
	}
	fclose( f );
}

void ovr_SetAllowLocalPreferencesFile( const bool allow )
{
	LOG( "ovr_SetAllowLocalPreferences : %d", allow );
	AllowLocalPreferencesFile = allow;
}

void ovr_ShutdownLocalPreferences()
{
	LOG( "ovr_ShutdownLocalPreferences" );
	LocalPreferences.clearAndRelease();
}
