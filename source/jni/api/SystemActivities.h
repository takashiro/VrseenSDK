#pragma once

#include "vglobal.h"

NV_NAMESPACE_BEGIN

//==============================================================================================
// Internal to VrAPI
//==============================================================================================
void SystemActivities_InitEventQueues();
void SystemActivities_ShutdownEventQueues();
void SystemActivities_AddEvent( char const * data );
void SystemActivities_AddInternalEvent( char const * data );
eVrApiEventStatus SystemActivities_nextPendingInternalEvent( char * buffer, unsigned int const bufferSize );
eVrApiEventStatus SystemActivities_nextPendingMainEvent( char * buffer, unsigned int const bufferSize );

NV_NAMESPACE_END


