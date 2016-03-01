#pragma once

#include "vglobal.h"

#include "Capture.h"
#include "Capture_Thread.h"

#pragma once

NV_NAMESPACE_BEGIN
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

NV_NAMESPACE_END

