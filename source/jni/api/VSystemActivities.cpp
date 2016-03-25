/*
 * VSystemActivities.cpp
 *
 *  Created on: 2016年3月25日
 *      Author: yangkai
 */

#include "Android/LogUtils.h"
#include "Android/JniUtils.h"
#include "api/VrApi.h"
#include "api/VrApi_Android.h"

#include "VSystemActivities.h"
#include "VLog.h"

// System Activities Events -- Why They Exist
//
// When an System Activities activity returns to an application, it sends an intent to bring
// the application to the front. Unfortunately in the case of a Unity application the intent
// data cannot piggy-back on this intent because Unity doesn't overload its
// UnityNativePlayerActivity::onNewIntent function to set the app's intent data to the new
// intent data. This means the new intent data is lost after onNewIntent executes and is
// never accessible to Unity afterwards. This can be fixed by overloading Unity's activity
// class, but that is problematic for Unity developers to an extent that makes in untenable
// for an SDK.
// As a result of this Unity issue, we instead send a broadcast intent to the application
// that started the system activity with an embedded JSON object indicating the action that
// the initiating application should perform. When received, these actions are pushed onto
// a single-producer / single-consumer queue so that they can be handled on the appropriate
// thread natively.
// In order to allow applications to later extend their functionality without getting a
// full API update, applications can query pending events using ovr_nextPendingEvent.
// Unfortunately, the Unity integration again makes this more difficult because querying
// from Unity C# code means that the queries happen on the Unity main thread and not the
// Unity render thread. All of UnityPlugin.cpp is set up to run off of the render thread
// and the OvrMobile structure in UnityPlugin is specific to that thread. This made it
// impossible to call ovr_SendIntent() (which calls ovr_LeaveVrMode) from the same thread
// where the events where being queried.
// To deal with this case, we have two event queues. The first queue always accepts events
// from the Java broadcast receiver. This queue is then read from the VrThread in native
// apps or the Unity main thread in Unity apps. When an event is read that has to be
// processed on the Unity render thread, the event is popped from the main event queue
// and pushed onto the internal event queue. The internal event queue is then read by
// Unity during ovr_HandleHmtEvents() on the Unity render thread. In native apps, both
// queues are handled in VrThread (i.e. there is not really a need for two queues) because
// this greatly simplified the code and educed the footprint for event handling in VrAPI.

namespace NervGear {

//==============================================================
// VVEventData
//
class VEventData {
public:
    VEventData();
    VEventData( void const * data, size_t const size );
    ~VEventData();

    void    allocData( size_t const size );
    void    freeData();

    VEventData & operator = ( VEventData const & other );

    void const *    getData() const { return m_data; }
    size_t          getSize() const { return m_size; }

private:
    void *  m_data;
    size_t  m_size;
};

VEventData::VEventData() :
    m_data( nullptr ),
    m_size( 0 )
{
}

//
VEventData::VEventData( void const * inData, size_t const inSize ) :
    m_data( NULL ),
    m_size( 0 )
{
    allocData( inSize );
    memcpy( m_data, inData, m_size );
}

//
VEventData::~VEventData()
{
    freeData();
}

//
void VEventData::allocData( size_t const size )
{
    m_data = malloc( size );
    m_size = size;
}

//
void VEventData::freeData()
{
    free( m_data );
    m_data = nullptr;
    m_size = 0;
}

//
VEventData & VEventData::operator = ( VEventData const & other )
{
    if ( &other != this ) {
        allocData( other.getSize() );
        memcpy( m_data, other.getData(), m_size );
    }
    return *this;
}

//==============================================================
// VVEventQueue
// Simple single-producer / singler-consumer queue for events.
// One slot is always left open to distinquish the full vs. empty
// cases.
class VEventQueue {
public:
    static const int    QUEUE_SIZE = 32;

    VEventQueue() :
        m_eventList(),
        m_headIndex( 0 ),
        m_tailIndex( 0 )
    {
    }
    ~VEventQueue() {
        for ( int i = 0; i < QUEUE_SIZE; ++i )
        {
            delete m_eventList[i];
        }
    }

    bool            isEmpty() const { return m_headIndex == m_tailIndex; }
    bool            isFull() const;

    bool            enqueue( VEventData const * inData );

    // Dequeue into a pre-allocated buffer
    bool            dequeue( VEventData const * & outData );

private:
    VEventData const *   m_eventList[QUEUE_SIZE];
    volatile int        m_headIndex;
    volatile int        m_tailIndex;
};

//
bool VEventQueue::isFull() const
{
    int indexBeforeTail = m_tailIndex == 0 ? QUEUE_SIZE - 1 : m_tailIndex - 1;
    if ( m_headIndex == indexBeforeTail )
    {
        return true;
    }
    return false;
}

//
bool VEventQueue::enqueue( VEventData const * inData )
{
    if ( isFull() )
    {
        return false;
    }
    int nextIndex = ( m_headIndex + 1 ) % QUEUE_SIZE;

    m_eventList[m_headIndex] = inData;

    // move the head after we've written the new item
    m_headIndex = nextIndex;

    return true;
}

//
bool VEventQueue::dequeue( VEventData const * & outData )
{
    if ( isEmpty() )
    {
        return false;
    }

    outData = m_eventList[m_tailIndex];
    m_eventList[m_tailIndex] = nullptr;

    // move the tail once we've consumed the item
    m_tailIndex = ( m_tailIndex + 1 ) % QUEUE_SIZE;

    return true;
}

//==============================================================================================
// Default queue manangement - these functions operate on the main queue that is used by VrApi.
// There may be other queues external to this file, like the queue used to re-queue events for
// the Unity thread.
VEventQueue * InternalVEventQueue = nullptr;
VEventQueue * MainVEventQueue = nullptr;

void VSystemActivities::initEventQueues()
{
    InternalVEventQueue = new VEventQueue();
    MainVEventQueue = new VEventQueue();
}

void VSystemActivities::shutdownEventQueues()
{
    delete InternalVEventQueue;
    InternalVEventQueue = nullptr;

    delete MainVEventQueue;
    MainVEventQueue = nullptr;
}

void VSystemActivities::addInternalEvent( const VString& data )
{
    NervGear::VEventData * VEventData = new NervGear::VEventData( data.toCString(), data.length() + 1 );
    InternalVEventQueue->enqueue( VEventData );
    vInfo( "SystemActivities: queued internal event " << data.toCString());
}

void VSystemActivities::addEvent( const VString& data )
{
    NervGear::VEventData * VEventData = new NervGear::VEventData( data.toCString(), data.length() + 1 );
    MainVEventQueue->enqueue( VEventData );
    vInfo( "SystemActivities: queued event " << data.toCString() );
}

static eVrApiEventStatus nextPendingEvent( VEventQueue * queue, VString& buffer, unsigned int const bufferSize )
{
    if ( buffer.length() == 0 || bufferSize == 0 )
    {
        return VRAPI_EVENT_ERROR_INVALID_BUFFER;
    }

    if ( bufferSize < 2 )
    {
        buffer = "";
        return VRAPI_EVENT_ERROR_INVALID_BUFFER;
    }

    if ( queue == NULL )
    {
        return VRAPI_EVENT_ERROR_INTERNAL;
    }

    VEventQueue * q = reinterpret_cast< VEventQueue* >( queue );
    VEventData const * VEventData;
    if ( !q->dequeue( VEventData ) )
    {
        return VRAPI_EVENT_NOT_PENDING;
    }

//  OVR_strncpy( buffer, bufferSize, static_cast< char const * >( VEventData->GetData() ), VEventData->GetSize() );
    buffer = static_cast< char const * >( VEventData->getData() );
    bool overflowed = VEventData->getSize() >= bufferSize;

    delete VEventData;
    return overflowed ? VRAPI_EVENT_BUFFER_OVERFLOW : VRAPI_EVENT_PENDING;
}

eVrApiEventStatus VSystemActivities::nextPendingInternalEvent( VString& buffer, unsigned int const bufferSize )
{
    return nextPendingEvent( InternalVEventQueue, buffer, bufferSize );
}

eVrApiEventStatus VSystemActivities::nextPendingMainEvent( VString& buffer, unsigned int const bufferSize )
{
    return nextPendingEvent( MainVEventQueue, buffer, bufferSize );
}

VSystemActivities::VSystemActivities()
{

}

VSystemActivities *VSystemActivities::instance()
{
    static VSystemActivities internal;
    return &internal;
}

}



