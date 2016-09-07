#pragma once

#include "VPixmap.h"
#include "VTexture.h"

NV_NAMESPACE_BEGIN

class VLoading : public VPixmap
{
public:
    VLoading(VGraphicsItem *parent = nullptr);
    VLoading(const VTexture &texture, VGraphicsItem *parent = nullptr);
    ~VLoading();

    void setDuration(unsigned int duration);

protected:
    void paint(VPainter *painter) override;

private:
    NV_DECLARE_PRIVATE
    NV_DISABLE_COPY(VLoading)
};

NV_NAMESPACE_END

