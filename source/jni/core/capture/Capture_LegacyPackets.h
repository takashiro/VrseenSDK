#pragma once

#include "vglobal.h"

#include <Capture_Config.h>
#include <Capture_Types.h>
#include <Capture_Packets.h>

#pragma once

NV_NAMESPACE_BEGIN
namespace Capture
{

// force to 4-byte alignment on all platforms... this *should* cause these structs to ignore -malign-double
// This might cause a slight load/store penalty on uint64/double, but it should be exceedingly minor in the
// grand scheme of things, and likely worth it for bandwidth reduction.
#pragma pack(4)

    // Just as an example... a version=0 ThreadNamePacket
    template<> struct ThreadNamePacket_<0>
    {
        static const PacketIdentifier s_packetID       = Packet_ThreadName;
        static const UInt32           s_version        = 0;
        static const bool             s_hasPayload     = false;
        static const UInt32           s_nameMaxLength  = 16;

        char name[s_nameMaxLength];
    };
    OVR_CAPTURE_STATIC_ASSERT(sizeof(ThreadNamePacket_<0>)==16);


// restore default alignment...
#pragma pack()

} // namespace Capture
NV_NAMESPACE_END

#endif
