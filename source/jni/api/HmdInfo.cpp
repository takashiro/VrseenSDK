/************************************************************************************

Filename    :   HmdInfo.cpp
Content     :   Head Mounted Display Info
Created     :   January 30, 2014
Authors     :   John Carmack

Copyright   :   Copyright 2014 Oculus VR, LLC. All Rights reserved.

*************************************************************************************/

#include "HmdInfo.h"

#include <math.h>
#include <jni.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <unistd.h>						// usleep, etc

#include "OVR.h"
#include "Android/LogUtils.h"

namespace NervGear
{

#define CASE_FOR_TYPE(NAME) case NAME: str = #NAME; break;

static void LogHmdType(PhoneTypeEnum type )
{
	VString str;

	switch ( type )
	{
		CASE_FOR_TYPE( HMD_GALAXY_S4 );
		CASE_FOR_TYPE( HMD_GALAXY_S5 );
		CASE_FOR_TYPE( HMD_GALAXY_S5_WQHD );
		CASE_FOR_TYPE( HMD_NOTE_4 );
	default:
		str = "<type not known - add to LogHmdType function>";
	}

	LogText( "Detected HMD/Phone version: %s", str.toCString() );
}

static PhoneTypeEnum IdentifyHmdType( const char * buildModel )
{
	if ( strcmp( buildModel, "GT-I9506" )  == 0 )
	{
		return HMD_GALAXY_S4;
	}

	if ( ( strcmp( buildModel, "SM-G900F" )  == 0 ) || ( strcmp( buildModel, "SM-G900X" )  == 0 ) )
	{
		return HMD_GALAXY_S5;
	}

	if ( strcmp( buildModel, "SM-G906S" )  == 0 )
	{
		return HMD_GALAXY_S5_WQHD;
	}

	if ( ( strstr( buildModel, "SM-N910" ) != NULL ) || ( strstr( buildModel, "SM-N916" ) != NULL ) )
	{
		return HMD_NOTE_4;
	}

	LOG( "IdentifyHmdType: Model %s not found. Defaulting to Note4", buildModel );
	return HMD_NOTE_4;
}

static hmdInfoInternal_t GetHmdInfo( const PhoneTypeEnum type )
{
	hmdInfoInternal_t hmdInfo = {};

	hmdInfo.lens.initLensByPhoneType(type);
	hmdInfo.displayRefreshRate = 60.0f;
	hmdInfo.eyeTextureResolution[0] = 1024;
	hmdInfo.eyeTextureResolution[1] = 1024;
	hmdInfo.eyeTextureFov[0] = 90.0f;
	hmdInfo.eyeTextureFov[1] = 90.0f;

	// Screen params.
	switch( type )
	{
	case HMD_GALAXY_S4:			// Galaxy S4 in Samsung's holder
		hmdInfo.lensSeparation = 0.062f;
		hmdInfo.eyeTextureFov[0] = 95.0f;
		hmdInfo.eyeTextureFov[1] = 95.0f;
		break;

	case HMD_GALAXY_S5:      // Galaxy S5 1080 paired with version 2 lenses
		hmdInfo.lensSeparation = 0.062f;
		hmdInfo.eyeTextureFov[0] = 90.0f;
		hmdInfo.eyeTextureFov[1] = 90.0f;
		break;

	case HMD_GALAXY_S5_WQHD:            // Galaxy S5 1440 paired with version 2 lenses
		hmdInfo.lensSeparation = 0.062f;
		hmdInfo.eyeTextureFov[0] = 90.0f;  // 95.0f
		hmdInfo.eyeTextureFov[1] = 90.0f;  // 95.0f
		break;

	default:
	case HMD_NOTE_4:      // Note 4
		hmdInfo.lensSeparation = 0.063f;	// JDC: measured on 8/23/2014
		hmdInfo.eyeTextureFov[0] = 90.0f;
		hmdInfo.eyeTextureFov[1] = 90.0f;

		hmdInfo.widthMeters = 0.125f;		// not reported correctly by display metrics!
		hmdInfo.heightMeters = 0.0707f;
		break;
	}
	return hmdInfo;
}

// We need to do java calls to get certain device parameters, but for Unity we
// won't be called by java code we control, so explicitly query for the info.
static void QueryAndroidInfo( JNIEnv *env, jobject activity, jclass vrActivityClass, const char * buildModel,
				float & screenWidthMeters, float & screenHeightMeters, int & screenWidthPixels, int & screenHeightPixels )
{
	LOG( "QueryAndroidInfo (%p,%p,%p)", env, activity, vrActivityClass );

	if ( env->ExceptionOccurred() )
	{
		env->ExceptionClear();
		LOG( "Cleared JNI exception" );
	}

	jmethodID getDisplayWidth = env->GetStaticMethodID( vrActivityClass, "getDisplayWidth", "(Landroid/app/Activity;)F" );
	if ( !getDisplayWidth )
	{
		FAIL( "couldn't get getDisplayWidth" );
	}
	const float widthMeters = env->CallStaticFloatMethod( vrActivityClass, getDisplayWidth, activity );

	jmethodID getDisplayHeight = env->GetStaticMethodID( vrActivityClass, "getDisplayHeight", "(Landroid/app/Activity;)F" );
	if ( !getDisplayHeight )
	{
		FAIL( "couldn't get getDisplayHeight" );
	}
	const float heightMeters = env->CallStaticFloatMethod( vrActivityClass, getDisplayHeight, activity );

	screenWidthMeters = widthMeters;
	screenHeightMeters = heightMeters;
	screenWidthPixels = 0;
	screenHeightPixels = 0;

	LOG( "%s %f x %f", buildModel, screenWidthMeters, screenHeightMeters );
}

hmdInfoInternal_t GetDeviceHmdInfo( const char * buildModel, JNIEnv *env, jobject activity, jclass vrActivityClass )
{
	PhoneTypeEnum type = IdentifyHmdType( buildModel );
	LogHmdType( type );

	hmdInfoInternal_t hmdInfo = {};
	hmdInfo = GetHmdInfo( type );

	if ( env != NULL && activity != 0 && vrActivityClass != 0 )
	{
		float screenWidthMeters, screenHeightMeters;
		int screenWidthPixels, screenHeightPixels;

		QueryAndroidInfo( env, activity, vrActivityClass, buildModel, screenWidthMeters, screenHeightMeters, screenWidthPixels, screenHeightPixels );

		// Only use the Android info if we haven't explicitly set the screenWidth / height,
		// because they are reported wrong on the note.
		if ( hmdInfo.widthMeters == 0 )
		{
			hmdInfo.widthMeters = screenWidthMeters;
			hmdInfo.heightMeters = screenHeightMeters;
		}
		hmdInfo.widthPixels = screenWidthPixels;
		hmdInfo.heightPixels = screenHeightPixels;
	}

	return hmdInfo;
}

}
