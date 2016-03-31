#include "LogUtils.h"

#include <unistd.h>			// for gettid()
#include <sys/syscall.h>	// for syscall()
#include <stdarg.h>
#include <string.h>

#include "VGlOperation.h"
#include "VLog.h"

NV_USING_NAMESPACE

static bool AllowGpuTimerQueries = false;

LogCpuTime::LogCpuTime(const char *label)
    : m_label( label )
{
    m_startTimeNanoSec = GetNanoSeconds();
}

LogCpuTime::~LogCpuTime()
{
    const double endTimeNanoSec = GetNanoSeconds();
    vInfo("LogCpuTime:" << m_label << "took" << ((endTimeNanoSec - m_startTimeNanoSec) * 1e-9) << "seconds");
}

double LogCpuTime::GetNanoSeconds()
{
    struct timespec now;
    clock_gettime( CLOCK_MONOTONIC, &now );
    return (double)now.tv_sec * 1e9 + now.tv_nsec;
}

template< int NumTimers, int NumFrames >
LogGpuTime<NumTimers,NumFrames>::LogGpuTime() :
    m_useTimerQuery( false ),
    m_useQueryCounter( false ),
    m_timerQuery(),
    m_beginTimestamp(),
    m_disjointOccurred(),
    m_timeResultIndex(),
    m_timeResultMilliseconds(),
    m_lastIndex( -1 )
{
}

template< int NumTimers, int NumFrames >
LogGpuTime<NumTimers,NumFrames>::~LogGpuTime()
{
    NervGear::VGlOperation glOperation;
	for ( int i = 0; i < NumTimers; i++ )
	{
        if ( m_timerQuery[i] != 0 )
		{
            glOperation.glDeleteQueriesEXT( 1, &m_timerQuery[i] );
		}
	}
}

template< int NumTimers, int NumFrames >
void LogGpuTime<NumTimers,NumFrames>::begin( int index )
{
    NervGear::VGlOperation glOperation;
    NervGear::VGlOperation::GpuType gpuType = glOperation.eglGetGpuType();
	// don't enable by default on Mali because this issues a glFinish() to work around a driver bug
    m_useTimerQuery = AllowGpuTimerQueries && ( ( gpuType & NervGear::VGlOperation::GPU_TYPE_MALI ) == 0 );
	// use glQueryCounterEXT on Mali to time GPU rendering to a non-default FBO
    m_useQueryCounter = AllowGpuTimerQueries && ( ( gpuType & NervGear::VGlOperation::GPU_TYPE_MALI ) != 0 );

    if ( m_useTimerQuery)
	{
		assert( index >= 0 && index < NumTimers );
        assert( m_lastIndex == -1 );
        m_lastIndex = index;

        if ( m_timerQuery[index] )
		{
			for ( GLint available = 0; available == 0; )
			{
                glOperation.glGetQueryObjectivEXT( m_timerQuery[index], GL_QUERY_RESULT_AVAILABLE, &available );
			}

            glGetIntegerv( glOperation.GL_GPU_DISJOINT_EXT, &m_disjointOccurred[index] );

			GLuint64 elapsedGpuTime = 0;
            glOperation.glGetQueryObjectui64vEXT( m_timerQuery[index], NervGear::VGlOperation::GL_QUERY_RESULT_EXT, &elapsedGpuTime );

            m_timeResultMilliseconds[index][m_timeResultIndex[index]] = ( elapsedGpuTime - (GLuint64)m_beginTimestamp[index] ) * 0.000001;
            m_timeResultIndex[index] = ( m_timeResultIndex[index] + 1 ) % NumFrames;
		}
		else
		{
            glOperation.glGenQueriesEXT( 1, &m_timerQuery[index] );
		}
        if ( !m_useQueryCounter )
		{
            m_beginTimestamp[index] = 0;
            glOperation.glBeginQueryEXT( NervGear::VGlOperation::GL_TIME_ELAPSED_EXT, m_timerQuery[index] );
		}
		else
		{
            glOperation.glGetInteger64v(  NervGear::VGlOperation::GL_TIMESTAMP_EXT, &m_beginTimestamp[index] );
		}
	}
}

template< int NumTimers, int NumFrames >
void LogGpuTime<NumTimers,NumFrames>::end( int index )
{
    NervGear::VGlOperation glOperation;
    if ( m_useTimerQuery)
	{
        assert( index == m_lastIndex );
        m_lastIndex = -1;

        if ( !m_useQueryCounter )
		{
            glOperation.glEndQueryEXT(  NervGear::VGlOperation::GL_TIME_ELAPSED_EXT );
		}
		else
		{
            glOperation.glQueryCounterEXT( m_timerQuery[index],  NervGear::VGlOperation::GL_TIMESTAMP_EXT );
			// Mali workaround: check for availability once to make sure all the pending flushes are resolved
			GLint available = 0;
            glOperation.glGetQueryObjectivEXT( m_timerQuery[index], GL_QUERY_RESULT_AVAILABLE, &available );
			// Mali workaround: need glFinish() when timing rendering to non-default FBO
			//glFinish();
		}
	}
}

template< int NumTimers, int NumFrames >
void LogGpuTime<NumTimers,NumFrames>::printTime( int index, const char * label ) const
{
    if ( m_useTimerQuery)
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
double LogGpuTime<NumTimers,NumFrames>::getTime( int index ) const
{
	double averageTime = 0;
	for ( int i = 0; i < NumFrames; i++ )
	{
        averageTime += m_timeResultMilliseconds[index][i];
	}
	averageTime *= ( 1.0 / NumFrames );
	return averageTime;
}

template< int NumTimers, int NumFrames >
double LogGpuTime<NumTimers,NumFrames>::totalTime() const
{
	double totalTime = 0;
	for ( int j = 0; j < NumTimers; j++ )
	{
		for ( int i = 0; i < NumFrames; i++ )
		{
            totalTime += m_timeResultMilliseconds[j][i];
		}
	}
	totalTime *= ( 1.0 / NumFrames );
	return totalTime;
}

template class LogGpuTime<2,10>;
template class LogGpuTime<8,10>;
