#pragma once

#include "vglobal.h"
#include "VString.h"
#include "VrApi.h"

NV_NAMESPACE_BEGIN

//==============================================================================================
// Internal to VrAPI
//==============================================================================================
void SystemActivities_InitEventQueues();
void SystemActivities_ShutdownEventQueues();
void SystemActivities_AddEvent( const VString&  data );
void SystemActivities_AddInternalEvent( const VString& data );
eVrApiEventStatus SystemActivities_nextPendingInternalEvent( VString& buffer, unsigned int const bufferSize );
eVrApiEventStatus SystemActivities_nextPendingMainEvent( VString& buffer, unsigned int const bufferSize );

NV_NAMESPACE_END


