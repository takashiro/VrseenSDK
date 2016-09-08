#pragma once

#include "VPixmap.h"
#include "VTexture.h"

NV_NAMESPACE_BEGIN

class VCursor : public VPixmap
{
public:
    VCursor(VGraphicsItem *parent = nullptr);
    VCursor(const VTexture &texture, VGraphicsItem *parent = nullptr);
    ~VCursor();

    void paint(VPainter *painter) override;

private:
    NV_DECLARE_PRIVATE
    NV_DISABLE_COPY(VCursor)
};

NV_NAMESPACE_END


