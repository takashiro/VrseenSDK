#pragma once

#include "VIODevice.h"

NV_NAMESPACE_BEGIN

class VBuffer : public VIODevice
{
public:
    VBuffer();
    ~VBuffer();

    vint64 bytesAvailable() const override;

protected:
    vint64 readData(char *data, vint64 maxSize) override;
    vint64 writeData(const char *data, vint64 maxSize) override;

private:
    NV_DECLARE_PRIVATE
    NV_DISABLE_COPY(VBuffer)
};

NV_NAMESPACE_END
