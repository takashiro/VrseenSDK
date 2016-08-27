#pragma once

#include "VPos.h"
#include "VRect3.h"

NV_NAMESPACE_BEGIN

class VGuiPainter;

class VGraphicsItem
{
    friend class VGui;

public:
    VGraphicsItem(VGraphicsItem *parent = nullptr);
    virtual ~VGraphicsItem();

    VGraphicsItem *parent() const;
    void addChild(VGraphicsItem *child);
    void removeChild(VGraphicsItem *child);

    VPosf pos() const;
    void setPos(const VPosf &pos);

    VPosf globalPos() const;

    VRect3f boundingRect() const;

protected:
    virtual void paint(VGuiPainter *painter);

private:
    void setParent(VGraphicsItem *parent);

    NV_DECLARE_PRIVATE
    NV_DISABLE_COPY(VGraphicsItem)
};

NV_NAMESPACE_END
