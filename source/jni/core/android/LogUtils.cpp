/************************************************************************************

Filename    :   Log.cpp
Content     :   Macros and helpers for Android logging.
Created     :   4/15/2014
Authors     :   Jonathan E. Wright

Copyright   :   Copyright 2014 Oculus VR, LLC. All Rights reserved.

*************************************************************************************/

#include "LogUtils.h"

#include <unistd.h>			// for gettid()
#include <sys/syscall.h>	// for syscall()
#include <stdarg.h>
#include <string.h>
#include <assert.h>

#include "api/VGlOperation.h"

// GPU Timer queries cause instability on current
// Adreno drivers. Disable by default, but allow
// enabling via the local prefs file.
// TODO: Test this on new Qualcomm driver under Lollipop.
static bool AllowGpuTimerQueries = false;

void SetAllowGpuTimerQueries( bool allow )
{
	LOG( "SetAllowGpuTimerQueries( %d )", allow );
	AllowGpuTimerQueries = allow;
}

void SetCurrentThreadAffinityMask( int mask )
{
	int err, syscallres;
	pid_t pid = gettid();
	syscallres = syscall( __NR_sched_setaffinity, pid, sizeof( mask ), &mask );
	if ( syscallres )
	{
		err = errno;
		WARN( "Error in the syscall setaffinity: mask=%d=0x%x err=%d=0x%x", mask, mask, err, err );
	}
}

// Log with an explicit tag
void LogWithTag( int prio, const char * tag, const char * fmt, ... )
{
	va_list ap;
	va_start( ap, fmt );
	__android_log_vprint( prio, tag, fmt, ap );
	va_end( ap );
}

void LogWithFileTag( int prio, const char * fileTag, const char * fmt, ... )
{
	va_list ap;

	// fileTag will be something like "jni/App.cpp", which we
	// want to strip down to just "App"
	char strippedTag[128];

	// scan backwards from the end to the first slash
	const int len = strlen( fileTag );
	int	slash;
	for ( slash = len - 1; slash > 0 && fileTag[slash] != '/'; slash-- )
	{
	}
	if ( fileTag[slash] == '/' )
	{
		slash++;
	}
	// copy forward until a dot or 0
	size_t i;
	for ( i = 0; i < sizeof( strippedTag ) - 1; i++ )
	{
		const char c = fileTag[slash+i];
		if ( c == '.' || c == 0 )
		{
			break;
		}
		strippedTag[i] = c;
	}
	strippedTag[i] = 0;

	va_start( ap, fmt );
	__android_log_vprint( prio, strippedTag, fmt, ap );
	va_end( ap );
}

template< int NumTimers, int NumFrames >
LogGpuTime<NumTimers,NumFrames>::LogGpuTime() :
	UseTimerQuery( false ),
	UseQueryCounter( false ),
	TimerQuery(),
	BeginTimestamp(),
	DisjointOccurred(),
	TimeResultIndex(),
	TimeResultMilliseconds(),
	LastIndex( -1 )
{
}

template< int NumTimers, int NumFrames >
LogGpuTime<NumTimers,NumFrames>::~LogGpuTime()
{
    NervGear::VGlOperation glOperation;
	for ( int i = 0; i < NumTimers; i++ )
	{
		if ( TimerQuery[i] != 0 )
		{
            glOperation.glDeleteQueriesEXT( 1, &TimerQuery[i] );
		}
	}
}

template< int NumTimers, int NumFrames >
void LogGpuTime<NumTimers,NumFrames>::Begin( int index )
{
    NervGear::VGlOperation glOperation;
    NervGear::VGlOperation::GpuType gpuType = glOperation.eglGetGpuType();
	// don't enable by default on Mali because this issues a glFinish() to work around a driver bug
    UseTimerQuery = AllowGpuTimerQueries && ( ( gpuType & NervGear::VGlOperation::GPU_TYPE_MALI ) == 0 );
	// use glQueryCounterEXT on Mali to time GPU rendering to a non-default FBO
    UseQueryCounter = AllowGpuTimerQueries && ( ( gpuType & NervGear::VGlOperation::GPU_TYPE_MALI ) != 0 );

    if ( UseTimerQuery)
	{
		assert( index >= 0 && index < NumTimers );
		assert( LastIndex == -1 );
		LastIndex = index;

		if ( TimerQuery[index] )
		{
			for ( GLint available = 0; available == 0; )
			{
                glOperation.glGetQueryObjectivEXT( TimerQuery[index], GL_QUERY_RESULT_AVAILABLE, &available );
			}

            glGetIntegerv( glOperation.GL_GPU_DISJOINT_EXT, &DisjointOccurred[index] );

			GLuint64 elapsedGpuTime = 0;
            glOperation.glGetQueryObjectui64vEXT( TimerQuery[index], NervGear::VGlOperation::GL_QUERY_RESULT_EXT, &elapsedGpuTime );

			TimeResultMilliseconds[index][TimeResultIndex[index]] = ( elapsedGpuTime - (GLuint64)BeginTimestamp[index] ) * 0.000001;
			TimeResultIndex[index] = ( TimeResultIndex[index] + 1 ) % NumFrames;
		}
		else
		{
            glOperation.glGenQueriesEXT( 1, &TimerQuery[index] );
		}
		if ( !UseQueryCounter )
		{
			BeginTimestamp[index] = 0;
            glOperation.glBeginQueryEXT( NervGear::VGlOperation::GL_TIME_ELAPSED_EXT, TimerQuery[index] );
		}
		else
		{
            glOperation.glGetInteger64v(  NervGear::VGlOperation::GL_TIMESTAMP_EXT, &BeginTimestamp[index] );
		}
	}
}

template< int NumTimers, int NumFrames >
void LogGpuTime<NumTimers,NumFrames>::End( int index )
{
    NervGear::VGlOperation glOperation;
    if ( UseTimerQuery)
	{
		assert( index == LastIndex );
		LastIndex = -1;

		if ( !UseQueryCounter )
		{
            glOperation.glEndQueryEXT(  NervGear::VGlOperation::GL_TIME_ELAPSED_EXT );
		}
		else
		{
            glOperation.glQueryCounterEXT( TimerQuery[index],  NervGear::VGlOperation::GL_TIMESTAMP_EXT );
			// Mali workaround: check for availability once to make sure all the pending flushes are resolved
			GLint available = 0;
            glOperation.glGetQueryObjectivEXT( TimerQuery[index], GL_QUERY_RESULT_AVAILABLE, &available );
			// Mali workaround: need glFinish() when timing rendering to non-default FBO
			//glFinish();
		}
	}
}

template< int NumTimers, int NumFrames >
void LogGpuTime<NumTimers,NumFrames>::PrintTime( int index, const char * label ) const
{
    if ( UseTimerQuery)
	{
//		double averageTime = 0.0;
//		for ( int i = 0; i < NumFrames; i++ )
//		{
//			averageTime += TimeResultMilliseconds[index][i];
//		}
//		averageTime *= ( 1.0 / NumFrames );
//		LOG( "%s %i: %3.1f %s", label, index, averageTime, DisjointOccurred[index] ? "DISJOINT" : "" );
	}
}

template< int NumTimers, int NumFrames >
double LogGpuTime<NumTimers,NumFrames>::GetTime( int index ) const
{
	double averageTime = 0;
	for ( int i = 0; i < NumFrames; i++ )
	{
		averageTime += TimeResultMilliseconds[index][i];
	}
	averageTime *= ( 1.0 / NumFrames );
	return averageTime;
}

template< int NumTimers, int NumFrames >
double LogGpuTime<NumTimers,NumFrames>::GetTotalTime() const
{
	double totalTime = 0;
	for ( int j = 0; j < NumTimers; j++ )
	{
		for ( int i = 0; i < NumFrames; i++ )
		{
			totalTime += TimeResultMilliseconds[j][i];
		}
	}
	totalTime *= ( 1.0 / NumFrames );
	return totalTime;
}

template class LogGpuTime<2,10>;
template class LogGpuTime<8,10>;
