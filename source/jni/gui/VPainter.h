#pragma once

#include "VMatrix4.h"

class VPainter
{
    friend class VGui;

public:
    VPainter();
    ~VPainter();

    const VMatrix4f &viewMatrix() const;

protected:
    void setViewMatrix(const VMatrix4f &viewMatrix);

private:
    NV_DECLARE_PRIVATE
    NV_DISABLE_COPY(VPainter)
};
