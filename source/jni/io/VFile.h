#pragma once

#include "VIODevice.h"

NV_NAMESPACE_BEGIN

class VFile : public VIODevice
{
public:
    VFile();

protected:
    vint64 readData(char *data, vint64 maxSize) override;
    vint64 writeData(const char *data, vint64 maxSize) override;
};

NV_NAMESPACE_END
