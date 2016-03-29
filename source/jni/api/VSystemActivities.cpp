/*
 * VSystemActivities.cpp
 *
 *  Created on: 2016年3月25日
 *      Author: yangkai
 */

#include "VSystemActivities.h"

#include "VLog.h"
#include "VEventLoop.h"

NV_NAMESPACE_BEGIN

static VEventLoop *InternalVEventQueue = nullptr;
static VEventLoop *MainVEventQueue = nullptr;

void VSystemActivities::initEventQueues()
{
    InternalVEventQueue = new VEventLoop(32);
    MainVEventQueue = new VEventLoop(32);
}

void VSystemActivities::shutdownEventQueues()
{
    delete InternalVEventQueue;
    InternalVEventQueue = nullptr;

    delete MainVEventQueue;
    MainVEventQueue = nullptr;
}

void VSystemActivities::addInternalEvent(const VString &data)
{
    VEvent event;
    event.data = data;
    InternalVEventQueue->post(event);
    vInfo("SystemActivities: queued internal event " << data);
}

void VSystemActivities::addEvent(const VString &data)
{
    VEvent event;
    event.data = data;
    MainVEventQueue->post(event);
    vInfo("SystemActivities: queued event " << data);
}

static eVrApiEventStatus nextPendingEvent(VEventLoop *queue, VString& buffer, const uint bufferSize)
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

    VEventLoop *q = reinterpret_cast< VEventLoop* >( queue );
    VEvent VEventData = q->next();
    if ( !VEventData.isValid() )
    {
        return VRAPI_EVENT_NOT_PENDING;
    }

    buffer = VEventData.data.toString();
    bool overflowed = buffer.size() >= bufferSize;
    return overflowed ? VRAPI_EVENT_BUFFER_OVERFLOW : VRAPI_EVENT_PENDING;
}

eVrApiEventStatus VSystemActivities::nextPendingInternalEvent(VString &buffer, const uint bufferSize)
{
    return nextPendingEvent(InternalVEventQueue, buffer, bufferSize);
}

eVrApiEventStatus VSystemActivities::nextPendingMainEvent(VString &buffer, const uint bufferSize)
{
    return nextPendingEvent(MainVEventQueue, buffer, bufferSize);
}

VSystemActivities::VSystemActivities()
{

}

VSystemActivities *VSystemActivities::instance()
{
    static VSystemActivities internal;
    return &internal;
}

NV_NAMESPACE_END


