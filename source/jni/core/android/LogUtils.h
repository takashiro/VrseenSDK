#pragma once

#include <stdlib.h>
#include <time.h>

class LogCpuTime
{
public:
    LogCpuTime(const char *label);
    ~LogCpuTime();

    static double GetNanoSeconds();

private:
    const char *m_label;
    double m_startTimeNanoSec;
};

template<int NumTimers, int NumFrames = 10>
class LogGpuTime
{
public:
    LogGpuTime();
    ~LogGpuTime();

    void begin( int index );
    void end( int index );
    void printTime( int index, const char * label ) const;
    double getTime( int index ) const;
    double totalTime() const;

private:
    bool m_useTimerQuery;
    bool m_useQueryCounter;
    uint32_t m_timerQuery[NumTimers];
    int64_t m_beginTimestamp[NumTimers];
    int32_t m_disjointOccurred[NumTimers];
    int32_t m_timeResultIndex[NumTimers];
    double m_timeResultMilliseconds[NumTimers][NumFrames];
    int m_lastIndex;
};
