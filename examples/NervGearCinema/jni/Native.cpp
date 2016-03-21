#include "CinemaApp.h"
#include "Native.h"

namespace OculusCinema
{

extern "C" {

void Java_com_vrseen_nervgear_cinema_MainActivity_nativeSetAppInterface( JNIEnv *jni, jclass clazz, jobject activity,
		jstring fromPackageName, jstring commandString, jstring uriString )
{
	LOG( "nativeSetAppInterface" );
    (new CinemaApp(jni, clazz, activity))->onCreate(fromPackageName, commandString, uriString );
}

void Java_com_vrseen_nervgear_cinema_MainActivity_nativeSetVideoSize( JNIEnv *, jclass, int width, int height, int rotation, int duration ) {
	LOG( "nativeSetVideoSizes: width=%i height=%i rotation=%i duration=%i", width, height, rotation, duration );
    VJson data(VJson::Array);
    data << width << height << rotation << duration;
    vApp->eventLoop().post("video", data);
}

jobject Java_com_vrseen_nervgear_cinema_MainActivity_nativePrepareNewVideo(JNIEnv *, jclass)
{
	// set up a message queue to get the return message
	// TODO: make a class that encapsulates this work

    VEventLoop result(1);
    vApp->eventLoop().post("newVideo", reinterpret_cast<int>(&result));

	result.wait();
    VEvent event = result.next();
    jobject	texobj;
    if (event.name == "surfaceTexture") {
        texobj = reinterpret_cast<jobject>(event.data.toInt());
    }

	return texobj;
}

}	// extern "C"

//==============================================================

// Java method ids
static jmethodID 	getExternalCacheDirectoryMethodId = NULL;
static jmethodID	createVideoThumbnailMethodId = NULL;
static jmethodID	checkForMovieResumeId = NULL;
static jmethodID 	isPlayingMethodId = NULL;
static jmethodID 	isPlaybackFinishedMethodId = NULL;
static jmethodID 	hadPlaybackErrorMethodId = NULL;
static jmethodID	getPositionMethodId = NULL;
static jmethodID	getDurationMethodId = NULL;
static jmethodID	setPositionMethodId = NULL;
static jmethodID 	seekDeltaMethodId = NULL;
static jmethodID 	startMovieMethodId = NULL;
static jmethodID 	pauseMovieMethodId = NULL;
static jmethodID 	resumeMovieMethodId = NULL;
static jmethodID 	stopMovieMethodId = NULL;
static jmethodID 	togglePlayingMethodId = NULL;

// Error checks and exits on failure
static jmethodID GetMethodID( App *app, jclass cls, const char * name, const char * signature )
{
	jmethodID mid = app->vrJni()->GetMethodID( cls, name, signature );
	if ( !mid )
	{
    	FAIL( "Couldn't find %s methodID", name );
    }

	return mid;
}

void Native::OneTimeInit( App *app, jclass mainActivityClass )
{
	LOG( "Native::OneTimeInit" );

	const double start = ovr_GetTimeInSeconds();

	getExternalCacheDirectoryMethodId 	= GetMethodID( app, mainActivityClass, "getExternalCacheDirectory", "()Ljava/lang/String;" );
	createVideoThumbnailMethodId 		= GetMethodID( app, mainActivityClass, "createVideoThumbnail", "(Ljava/lang/String;Ljava/lang/String;II)Z" );
	checkForMovieResumeId 				= GetMethodID( app, mainActivityClass, "checkForMovieResume", "(Ljava/lang/String;)Z" );
	isPlayingMethodId 					= GetMethodID( app, mainActivityClass, "isPlaying", "()Z" );
	isPlaybackFinishedMethodId			= GetMethodID( app, mainActivityClass, "isPlaybackFinished", "()Z" );
	hadPlaybackErrorMethodId			= GetMethodID( app, mainActivityClass, "hadPlaybackError", "()Z" );
	getPositionMethodId 				= GetMethodID( app, mainActivityClass, "getPosition", "()I" );
	getDurationMethodId 				= GetMethodID( app, mainActivityClass, "getDuration", "()I" );
	setPositionMethodId 				= GetMethodID( app, mainActivityClass, "setPosition", "(I)V" );
	seekDeltaMethodId					= GetMethodID( app, mainActivityClass, "seekDelta", "(I)V" );
	startMovieMethodId 					= GetMethodID( app, mainActivityClass, "startMovie", "(Ljava/lang/String;ZZZ)V" );
	pauseMovieMethodId 					= GetMethodID( app, mainActivityClass, "pauseMovie", "()V" );
	resumeMovieMethodId 				= GetMethodID( app, mainActivityClass, "resumeMovie", "()V" );
	stopMovieMethodId 					= GetMethodID( app, mainActivityClass, "stopMovie", "()V" );
	togglePlayingMethodId 				= GetMethodID( app, mainActivityClass, "togglePlaying", "()Z" );

	LOG( "Native::OneTimeInit: %3.1f seconds", ovr_GetTimeInSeconds() - start );
}

void Native::OneTimeShutdown()
{
	LOG( "Native::OneTimeShutdown" );
}

VString Native::GetExternalCacheDirectory( App *app )
{
	jstring externalCacheDirectoryString = (jstring)app->vrJni()->CallObjectMethod( app->javaObject(), getExternalCacheDirectoryMethodId );

	const char *externalCacheDirectoryStringUTFChars = app->vrJni()->GetStringUTFChars( externalCacheDirectoryString, NULL );
	VString externalCacheDirectory = externalCacheDirectoryStringUTFChars;

	app->vrJni()->ReleaseStringUTFChars( externalCacheDirectoryString, externalCacheDirectoryStringUTFChars );
	app->vrJni()->DeleteLocalRef( externalCacheDirectoryString );

	return externalCacheDirectory;
}

bool Native::CreateVideoThumbnail( App *app, const char *videoFilePath, const char *outputFilePath, const int width, const int height )
{
	LOG( "CreateVideoThumbnail( %s, %s )", videoFilePath, outputFilePath );

	jstring jstrVideoFilePath = app->vrJni()->NewStringUTF( videoFilePath );
	jstring jstrOutputFilePath = app->vrJni()->NewStringUTF( outputFilePath );

	jboolean result = app->vrJni()->CallBooleanMethod( app->javaObject(), createVideoThumbnailMethodId, jstrVideoFilePath, jstrOutputFilePath, width, height );

	app->vrJni()->DeleteLocalRef( jstrVideoFilePath );
	app->vrJni()->DeleteLocalRef( jstrOutputFilePath );

	LOG( "CreateVideoThumbnail( %s, %s )", videoFilePath, outputFilePath );

	return result;
}

bool Native::CheckForMovieResume( App *app, const char * movieName )
{
	LOG( "CheckForMovieResume( %s )", movieName );

	jstring jstrMovieName = app->vrJni()->NewStringUTF( movieName );

	jboolean result = app->vrJni()->CallBooleanMethod( app->javaObject(), checkForMovieResumeId, jstrMovieName );

	app->vrJni()->DeleteLocalRef( jstrMovieName );

	return result;
}

bool Native::IsPlaying( App *app )
{
	LOG( "IsPlaying()" );
	return app->vrJni()->CallBooleanMethod( app->javaObject(), isPlayingMethodId );
}

bool Native::IsPlaybackFinished( App *app )
{
	jboolean result = app->vrJni()->CallBooleanMethod( app->javaObject(), isPlaybackFinishedMethodId );
	return ( result != 0 );
}

bool Native::HadPlaybackError( App *app )
{
	jboolean result = app->vrJni()->CallBooleanMethod( app->javaObject(), hadPlaybackErrorMethodId );
	return ( result != 0 );
}

int Native::GetPosition( App *app )
{
	LOG( "GetPosition()" );
	return app->vrJni()->CallIntMethod( app->javaObject(), getPositionMethodId );
}

int Native::GetDuration( App *app )
{
	LOG( "GetDuration()" );
	return app->vrJni()->CallIntMethod( app->javaObject(), getDurationMethodId );
}

void Native::SetPosition( App *app, int positionMS )
{
	LOG( "SetPosition()" );
	app->vrJni()->CallVoidMethod( app->javaObject(), setPositionMethodId, positionMS );
}

void Native::SeekDelta( App *app, int deltaMS )
{
	LOG( "SeekDelta()" );
	app->vrJni()->CallVoidMethod( app->javaObject(), seekDeltaMethodId, deltaMS );
}

void Native::StartMovie( App *app, const char * movieName, bool resumePlayback, bool isEncrypted, bool loop )
{
	LOG( "StartMovie( %s )", movieName );

	jstring jstrMovieName = app->vrJni()->NewStringUTF( movieName );

	app->vrJni()->CallVoidMethod( app->javaObject(), startMovieMethodId, jstrMovieName, resumePlayback, isEncrypted, loop );

	app->vrJni()->DeleteLocalRef( jstrMovieName );
}

void Native::PauseMovie( App *app )
{
	LOG( "PauseMovie()" );
	app->vrJni()->CallVoidMethod( app->javaObject(), pauseMovieMethodId );
}

void Native::ResumeMovie( App *app )
{
	LOG( "ResumeMovie()" );
	app->vrJni()->CallVoidMethod( app->javaObject(), resumeMovieMethodId );
}

void Native::StopMovie( App *app )
{
	LOG( "StopMovie()" );
	app->vrJni()->CallVoidMethod( app->javaObject(), stopMovieMethodId );
}

bool Native::TogglePlaying( App *app )
{
	LOG( "TogglePlaying()" );
	return app->vrJni()->CallBooleanMethod( app->javaObject(), togglePlayingMethodId );
}

} // namespace OculusCinema
