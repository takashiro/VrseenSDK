/************************************************************************************

Filename    :   FileLoader.cpp
Content     :
Created     :   August 13, 2014
Authors     :   John Carmack

Copyright   :   Copyright 2014 Oculus VR, LLC. All Rights reserved.

This source code is licensed under the BSD-style license found in the
LICENSE file in the Oculus360Photos/ directory. An additional grant
of patent rights can be found in the PATENTS file in the same directory.

************************************************************************************/

#include "VThread.h"
#include "FileLoader.h"
#include "Oculus360Photos.h"

#include <fstream>

#include <VImage.h>
#include <VLog.h>
#include <VZipFile.h>

namespace NervGear {

VEventLoop		Queue1( 4000 );	// big enough for all the thumbnails that might be needed
VEventLoop		Queue3( 1 );

pthread_mutex_t QueueMutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t	QueueWake = PTHREAD_COND_INITIALIZER;
bool			QueueHasCleared = true;

void * Queue1Thread( void * v )
{
	int result = pthread_setname_np( pthread_self(), "FileQueue1" );
	if ( result != 0 )
	{
		vInfo("InitFileQueue: pthread_setname_np failed" << strerror( result ));
	}

	// Process incoming messages until queue is empty
	for ( ; ; )
	{
		Queue1.wait();

		// If Queue3 hasn't finished our last output, sleep until it has.
		pthread_mutex_lock( &QueueMutex );
		while ( !QueueHasCleared )
		{
			// Atomically unlock the mutex and block until we get a message.
			pthread_cond_wait( &QueueWake, &QueueMutex );
		}
		pthread_mutex_unlock( &QueueMutex );

        VEvent event = Queue1.next();
        VString filename = event.data.toString();

        VEventLoop *queue = &Queue3;
        if (filename.endsWith("_nz.jpg", false)) {
			// cube map
			const char * const cubeSuffix[6] = { "_px.jpg", "_nx.jpg", "_py.jpg", "_ny.jpg", "_pz.jpg", "_nz.jpg" };

            VString filenameWithoutSuffix = filename.left(filename.size() - 7);

            VVariantArray args;
			int side = 0;
			for ( ; side < 6; side++ )
			{
                VString sideFilename = filenameWithoutSuffix + cubeSuffix[side];
                std::fstream fileBuffer;
                fileBuffer.open(sideFilename.toUtf8().data());
                fileBuffer.seekg(0, std::ios_base::end);
                uint fileLength = 0;
                fileLength = fileBuffer.tellg();
                fileBuffer.seekg(0, std::ios_base::beg);
                void *buffer = NULL;
                buffer = malloc(fileLength);
                fileBuffer.read(reinterpret_cast<std::istream::char_type*>(buffer), fileLength);
                if ( fileLength == 0 || buffer == NULL )
				{
                    const VZipFile &apk = vApp->apkFile();
                    if (!apk.read(sideFilename, buffer, fileLength)) {
						break;
					}
				}
                 args << buffer << fileLength;
                vInfo( "Queue1 loaded" << sideFilename);
			}
            queue->post(event.name, args);
		}
		else
		{
			// non-cube map
            std::fstream fileBuffer;
            fileBuffer.open(filename.toUtf8().data());
            fileBuffer.seekg(0, std::ios_base::end);
            uint fileLength = 0;
            fileLength = fileBuffer.tellg();
            fileBuffer.seekg(0, std::ios_base::beg);
            void *buffer = NULL;
            buffer = malloc(fileLength);
            fileBuffer.read(reinterpret_cast<std::istream::char_type*>(buffer), fileLength);
            if ( fileLength <= 0 || buffer == NULL )
			{
                const VZipFile &apk = vApp->apkFile();
                if (!apk.read(filename,buffer, fileLength)) {
					continue;
				}
			}

            VVariantArray args;
            args << buffer << fileLength;
            queue->post(event.name, args);
		}
	}
	return NULL;
}

void * Queue3Thread( void * v )
{
	int result = pthread_setname_np( pthread_self(), "FileQueue3" );
	if ( result != 0 )
	{
		vInfo("InitFileQueue: pthread_setname_np failed" << strerror( result ));
	}

	// Process incoming messages until queue is empty
    forever {
		Queue3.wait();
        VEvent event = Queue3.next();

        vInfo("Queue3 msg =" << event.name);

		// Note that Queue3 has cleared the message
		pthread_mutex_lock( &QueueMutex );
		QueueHasCleared = true;
		pthread_cond_signal( &QueueWake );
		pthread_mutex_unlock( &QueueMutex );

        uint *b[6] = {};
		int blen[6];
        int numBuffers = 1;
        if (event.name != "cube") {
            b[0] = static_cast<uint *>(event.data.at(0).toPointer());
            blen[0] = event.data.at(1).toInt();
        } else {
            numBuffers = 6;
            int k = 0;
            for (int i = 0; i < 6; i++) {
                b[i] = static_cast<uint *>(event.data.at(k).toPointer());
                k++;
                blen[i] = event.data.at(k).toInt();
                k++;
            }
		}

#define USE_TURBO_JPEG
#if !defined( USE_TURBO_JPEG )
		stbi_uc * data[6];
#else
		unsigned char * data[6];
#endif

		int resolutionX = 0;
		int resolutionY = 0;
		int buffCount = 0;
		for ( ; buffCount < numBuffers; buffCount++ )
		{
			int	x, y;
			unsigned * b1 = b[buffCount];
			int b1len = blen[buffCount];


            VImage image(VByteArray(reinterpret_cast<const char *>(b1), b1len));
            x = image.width();
            y = image.height();
            data[buffCount] = (uchar *) malloc(image.length());
            memcpy(data[buffCount], image.data(), image.length());

            if ( buffCount == 0 )
			{
				resolutionX = x;
				resolutionY = y;
			}

			// done with the loading buffer now
			free( b1 );

			if ( data[buffCount] == NULL )
			{
				vInfo("LoadingThread: failed to load from buffer");
				break;
			}
		}

		if ( buffCount != numBuffers )	// an image load failed, free everything and abort this load
		{
			for ( int i = 0; i < numBuffers; ++i )
			{
				free( data[i] );
				data[i] = NULL;
			}
		}
		else
		{
			if ( numBuffers == 1 )
			{
                vAssert( data[0] != NULL );
                VVariantArray args;
                args << data[0] << resolutionX << resolutionY;
                ( ( Oculus360Photos * )v )->backgroundMessageQueue().post(event.name, std::move(args));
			}
			else
			{
                vAssert(numBuffers == 6);
                VVariantArray args;
                args << resolutionX << data[0] << data[1] << data[2] << data[3] << data[4] << data[5];
                ( ( Oculus360Photos * )v )->backgroundMessageQueue().post(event.name, std::move(args));
			}
		}
	}
	return NULL;
}

static const int NUM_QUEUE_THREADS = 2;

void InitFileQueue( App * app, Oculus360Photos * photos )
{
    // spawn the queue threads
	void * (*funcs[NUM_QUEUE_THREADS])( void *) = { Queue1Thread, Queue3Thread };

	for ( int i = 0; i < NUM_QUEUE_THREADS; i++ )
	{
		pthread_attr_t loadingThreadAttr;
		pthread_attr_init( &loadingThreadAttr );
		sched_param sparam;
        sparam.sched_priority = VThread::GetOSPriority(VThread::NormalPriority);
		pthread_attr_setschedparam( &loadingThreadAttr, &sparam );
		pthread_t	loadingThread;
		int createLoadingThreadErr = -1;
		if ( i == 1 )
		{
			createLoadingThreadErr = pthread_create( &loadingThread, &loadingThreadAttr, funcs[ i ], ( void * )photos );
		}
		else
		{
			createLoadingThreadErr = pthread_create( &loadingThread, &loadingThreadAttr, funcs[ i ], ( void * )app );
		}
		if ( createLoadingThreadErr != 0 )
		{
			vInfo("loadingThread: pthread_create returned" << createLoadingThreadErr);
		}
	}
}

}	// namespace NervGear
