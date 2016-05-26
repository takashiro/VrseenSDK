#include "VBinaryStream.h"
#include "VIODevice.h"

NV_NAMESPACE_BEGIN

VBinaryStream::VBinaryStream(VIODevice *device)
    : m_device(device)
{
}

NV_NAMESPACE_END
