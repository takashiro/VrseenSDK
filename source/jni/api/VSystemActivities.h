/*
 * VSystemActivities.h
 *
 *  Created on: 2016年3月25日
 *      Author: yangkai
 */
#pragma once

#include "vglobal.h"
#include "VString.h"
#include "VrApi.h"

NV_NAMESPACE_BEGIN

class VSystemActivities
{
public:
    static VSystemActivities *instance();

    void initEventQueues();
    void shutdownEventQueues();
    void addEvent(const VString &data);
    void addInternalEvent(const VString &data);
    eVrApiEventStatus nextPendingInternalEvent(VString &buffer, const uint bufferSize);
    eVrApiEventStatus nextPendingMainEvent(VString &buffer, const uint bufferSize);

private:
    VSystemActivities();
};

NV_NAMESPACE_END
