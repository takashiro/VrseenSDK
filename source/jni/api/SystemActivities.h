/************************************************************************************

Filename    :   SystemActivities.h
Content     :   Event handling for system activities
Created     :   February 23, 2015
Authors     :   Jonathan E. Wright

Copyright   :   Copyright 2014 Oculus VR, LLC. All Rights reserved.

*************************************************************************************/
#ifndef OVR_SystemActivities_h
#define OVR_SystemActivities_h

namespace NervGear {

//==============================================================================================
// Internal to VrAPI
//==============================================================================================
void SystemActivities_InitEventQueues();
void SystemActivities_ShutdownEventQueues();
void SystemActivities_AddEvent( char const * data );
void SystemActivities_AddInternalEvent( char const * data );
eVrApiEventStatus SystemActivities_nextPendingInternalEvent( char * buffer, unsigned int const bufferSize );
eVrApiEventStatus SystemActivities_nextPendingMainEvent( char * buffer, unsigned int const bufferSize );

} // namespace NervGear

#endif // OVR_SystemActivities_h