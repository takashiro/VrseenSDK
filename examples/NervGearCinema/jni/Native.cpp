#include "CinemaApp.h"
#include "Native.h"
#include "core/VTimer.h"

#include <android/JniUtils.h>

namespace OculusCinema
{

extern "C" {

void Java_com_vrseen_nervgear_cinema_MainActivity_nativeSetAppInterface( JNIEnv *jni, jclass clazz, jobject activity,
		jstring fromPackageName, jstring commandString, jstring uriString )
{
	vInfo("nativeSetAppInterface");
    (new CinemaApp(jni, clazz, activity))->onCreate(fromPackageName, commandString, uriString );
}

void Java_com_vrseen_nervgear_cinema_MainActivity_nativeSetVideoSize( JNIEnv *, jclass, int width, int height, int rotation, int duration ) {
	vInfo("nativeSetVideoSizes: width=" << width << "height=" << height << "rotation=" << rotation << "duration=" << duration);
    VVariantArray data;
    data << width << height << rotation << duration;
    vApp->eventLoop().post("video", std::move(data));
}

jobject Java_com_vrseen_nervgear_cinema_MainActivity_nativePrepareNewVideo(JNIEnv *, jclass)
{
	// set up a message queue to get the return message
	// TODO: make a class that encapsulates this work

    VEventLoop result(1);
    vApp->eventLoop().post("newVideo", &result);

	result.wait();
    VEvent event = result.next();
    jobject	texobj = nullptr;
    if (event.name == "surfaceTexture") {
        texobj = static_cast<jobject>(event.data.toPointer());
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
    	vFatal("Couldn't find" << name << "methodID");
    }

	return mid;
}

void Native::OneTimeInit( App *app, jclass mainActivityClass )
{
	vInfo("Native::OneTimeInit");

    const double start = VTimer::Seconds();

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

    vInfo("Native::OneTimeInit:" << (VTimer::Seconds() - start) << "seconds");
}

void Native::OneTimeShutdown()
{
	vInfo("Native::OneTimeShutdown");
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

bool Native::CreateVideoThumbnail(const VString &videoFilePath, const VString &outputFilePath, const int width, const int height )
{
    vInfo( "CreateVideoThumbnail" << videoFilePath << outputFilePath);

    jstring jstrVideoFilePath = JniUtils::Convert(vApp->vrJni(), videoFilePath);
    jstring jstrOutputFilePath = JniUtils::Convert(vApp->vrJni(), outputFilePath);

    jboolean result = vApp->vrJni()->CallBooleanMethod( vApp->javaObject(), createVideoThumbnailMethodId, jstrVideoFilePath, jstrOutputFilePath, width, height );

    vApp->vrJni()->DeleteLocalRef( jstrVideoFilePath );
    vApp->vrJni()->DeleteLocalRef( jstrOutputFilePath );

	return result;
}

bool Native::CheckForMovieResume(const VString &movieName)
{
	vInfo("CheckForMovieResume(" << movieName << ")");

    jstring jstrMovieName = JniUtils::Convert(vApp->vrJni(), movieName);

    jboolean result = vApp->vrJni()->CallBooleanMethod( vApp->javaObject(), checkForMovieResumeId, jstrMovieName );

    vApp->vrJni()->DeleteLocalRef( jstrMovieName );

	return result;
}

bool Native::IsPlaying( App *app )
{
	vInfo("IsPlaying()");
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
	vInfo("GetPosition()");
	return app->vrJni()->CallIntMethod( app->javaObject(), getPositionMethodId );
}

int Native::GetDuration( App *app )
{
	vInfo("GetDuration()");
	return app->vrJni()->CallIntMethod( app->javaObject(), getDurationMethodId );
}

void Native::SetPosition( App *app, int positionMS )
{
	vInfo("SetPosition()");
	app->vrJni()->CallVoidMethod( app->javaObject(), setPositionMethodId, positionMS );
}

void Native::SeekDelta( App *app, int deltaMS )
{
	vInfo("SeekDelta()");
	app->vrJni()->CallVoidMethod( app->javaObject(), seekDeltaMethodId, deltaMS );
}

void Native::StartMovie(const VString &movieName, bool resumePlayback, bool isEncrypted, bool loop )
{
	vInfo("StartMovie(" << movieName << ")");

    jstring jstrMovieName = JniUtils::Convert(vApp->vrJni(), movieName);

    vApp->vrJni()->CallVoidMethod( vApp->javaObject(), startMovieMethodId, jstrMovieName, resumePlayback, isEncrypted, loop );

    vApp->vrJni()->DeleteLocalRef( jstrMovieName );
}

void Native::PauseMovie( App *app )
{
	vInfo("PauseMovie()");
	app->vrJni()->CallVoidMethod( app->javaObject(), pauseMovieMethodId );
}

void Native::ResumeMovie( App *app )
{
	vInfo("ResumeMovie()");
	app->vrJni()->CallVoidMethod( app->javaObject(), resumeMovieMethodId );
}

void Native::StopMovie( App *app )
{
	vInfo("StopMovie()");
	app->vrJni()->CallVoidMethod( app->javaObject(), stopMovieMethodId );
}

bool Native::TogglePlaying( App *app )
{
	vInfo("TogglePlaying()");
	return app->vrJni()->CallBooleanMethod( app->javaObject(), togglePlayingMethodId );
}

} // namespace OculusCinema
