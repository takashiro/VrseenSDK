/************************************************************************************

Filename    :   TalkToJava.cpp
Content     :   Thread and JNI management for making java calls in the background
Created     :   February 26, 2014
Authors     :   John Carmack

Copyright   :   Copyright 2014 Oculus VR, LLC. All Rights reserved.

*************************************************************************************/

#include "TalkToJava.h"

#include <stdlib.h>
#include <string.h>

#include "Android/LogUtils.h"
#include "VLog.h"

namespace NervGear
{

void TalkToJava::Init(JavaVM *javaVM, TalkToJavaInterface *interface)
{
    Jvm = javaVM;
    Interface = interface;

	// spawn the VR thread
	const int createErr = pthread_create( &TtjThread, NULL /* default attributes */, &ThreadStarter, this );
	if ( createErr != 0 )
	{
		FAIL( "pthread_create returned %i", createErr );
	}
	else
	{
		pthread_setname_np(TtjThread, "TalkToJava" );
	}
}

TalkToJava::~TalkToJava()
{
	if ( TtjThread )
	{
		// Get the background thread to kill itself.
		LOG( "TtjMessageQueue.PostPrintf( quit )" );
		TtjMessageQueue.post( "quit" );
		const int ret = pthread_join( TtjThread, NULL );
		if ( ret != 0 )
		{
			WARN( "failed to join TtjThread (%i)", ret );
		}
	}
}

// Shim to call a C++ object from a posix thread start.
void *TalkToJava::ThreadStarter( void * parm )
{
	int result = pthread_setname_np( pthread_self(), "TalkToJava" );
	if ( result != 0 )
	{
		WARN( "TalkToJava: pthread_setname_np failed %s", strerror( result ) );
	}

	((TalkToJava *)parm)->TtjThreadFunction();

	return NULL;
}

/*
 * TtjThreadFunction
 *
 * Continuously waits for command messages and processes them.
 */
void TalkToJava::TtjThreadFunction()
{
	// The Java VM needs to be attached on each thread that will use it.
	LOG( "TalkToJava: Jvm->AttachCurrentThread" );
	const jint returnAttach = Jvm->AttachCurrentThread( &Jni, 0 );
	if ( returnAttach != JNI_OK )
	{
		LOG( "javaVM->AttachCurrentThread returned %i", returnAttach );
	}

	// Process all queued messages
	for ( ; ; )
	{
        VEvent event = TtjMessageQueue.next();
        if (!event.isValid()) {
			// Go dormant until something else arrives.
			TtjMessageQueue.wait();
			continue;
		}

        if (event.name == "quit") {
            break;
		}

		// Set up a local frame with room for at least 100
		// local references that will be auto-freed.
		Jni->PushLocalFrame( 100 );

		// Let whoever initialized us do what they want.
        Interface->TtjCommand(Jni, event);

		// If we don't clean up exceptions now, later
		// calls may fail.
		if ( Jni->ExceptionOccurred() )
		{
			Jni->ExceptionClear();
            vInfo("JNI exception after:" << event.name);
		}

		// Free any local references
		Jni->PopLocalFrame( NULL );
	}

	LOG( "TalkToJava: Jvm->DetachCurrentThread" );
	const jint returnDetach = Jvm->DetachCurrentThread();
	if ( returnDetach != JNI_OK )
	{
		LOG( "javaVM->DetachCurrentThread returned %i", returnDetach );
    }
}

}
