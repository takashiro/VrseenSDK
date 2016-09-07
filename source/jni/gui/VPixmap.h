#pragma once

#include "VGraphicsItem.h"
#include "VTexture.h"

NV_NAMESPACE_BEGIN

class VPixmap : public VGraphicsItem
{
public:
    VPixmap(VGraphicsItem *parent = nullptr);
    VPixmap(const VTexture &texture, VGraphicsItem *parent = nullptr);
    ~VPixmap();

    void load(const VTexture &texture);

    const VRect3f &rect() const;
    void setRect(const VRect3f & rect);

protected:
    void paint(VPainter *painter) override;

private:
    NV_DECLARE_PRIVATE
    NV_DISABLE_COPY(VPixmap)
};

NV_NAMESPACE_END
