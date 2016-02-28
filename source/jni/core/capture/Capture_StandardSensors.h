/************************************************************************************

PublicHeader:   OVR.h
Filename    :   OVR_Capture_Socket.h
Content     :   Oculus performance capture library. Support for standard builtin device sensors.
Created     :   January, 2015
Notes       :

Copyright   :   Copyright 2015 Oculus VR, LLC. All Rights reserved.

************************************************************************************/

#ifndef OVR_CAPTURE_STANDARDSENSORS_H
#define OVR_CAPTURE_STANDARDSENSORS_H

#include "Capture.h"
#include "Capture_Thread.h"

namespace NervGear
{
namespace Capture
{

    class StandardSensors : public Thread
    {
        public:
            StandardSensors(void);
            virtual ~StandardSensors(void);

        private:
            virtual void OnThreadExecute(void);

        private:

    };

} // namespace Capture
} // namespace NervGear

#endif
