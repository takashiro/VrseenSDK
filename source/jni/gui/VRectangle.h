#pragma once

#include "VGraphicsItem.h"
#include "VColor.h"

NV_NAMESPACE_BEGIN

class VRectangle : public VGraphicsItem
{
public:
    VRectangle(VGraphicsItem *parent = nullptr);
    ~VRectangle();

    VRect3f rect() const;
    void setRect(const VRect3f &rect);

    VColor color() const;
    void setColor(const VColor &color);

protected:
    void paint(VGuiPainter *painter) override;

private:
    NV_DECLARE_PRIVATE
    NV_DISABLE_COPY(VRectangle)
};

NV_NAMESPACE_END
