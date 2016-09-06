#pragma once

#include "VPos.h"
#include "VRect3.h"
#include "VMatrix4.h"

NV_NAMESPACE_BEGIN

class VPainter;

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

    double clickElapsedTime() const;
    void setClickElapsedTime(double elapsed);

protected:
    virtual void init(void *vg);

    virtual void onFocus();
    virtual void onBlur();
    virtual void onClick();

    virtual void paint(VPainter *painter);
    void setBoundingRect(const VRect3f &rect);

private:
    void onSensorChanged(const VMatrix4f &mvp);
    void setParent(VGraphicsItem *parent);

    NV_DECLARE_PRIVATE
    NV_DISABLE_COPY(VGraphicsItem)
};

NV_NAMESPACE_END
