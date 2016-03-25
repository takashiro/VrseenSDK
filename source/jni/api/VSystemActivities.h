/*
 * VSystemActivities.h
 *
 *  Created on: 2016年3月25日
 *      Author: yangkai
 */
#pragma once
#include "vglobal.h"
#include "VString.h"

NV_NAMESPACE_BEGIN
class VSystemActivities
{
private:
    VSystemActivities();
public:
    static VSystemActivities *instance();

    void initEventQueues();
    void shutdownEventQueues();
    void addEvent( const VString&  data );
    void addInternalEvent( const VString& data );
    eVrApiEventStatus nextPendingInternalEvent( VString& buffer, unsigned int const bufferSize );
    eVrApiEventStatus nextPendingMainEvent( VString& buffer, unsigned int const bufferSize );
};
NV_NAMESPACE_END
