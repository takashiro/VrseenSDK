#pragma once

#include "vglobal.h"
#include "VArray.h"
#include "VPos.h"

NV_NAMESPACE_BEGIN

class VItem
{
public:
    VItem(VItem *parent = nullptr);
    virtual ~VItem();

    void addChild(VItem *item);
    void removeChild(VItem *item);
    const VArray<VItem *> &children() const;

    VItem *parent() const;
    void setParent(VItem *item);

    void paintAll();

    void setPos(const VPosF &pos);
    VPosF &pos();
    VPosF pos() const;

    VPosF globalPos() const;

    void setX(vreal x);
    vreal x() const;

    void setY(vreal y);
    vreal y() const;

    void setZ(vreal z);
    vreal z() const;

    void setVisible(bool visible);
    bool isVisible() const;

protected:
    virtual void paint();

private:
    NV_DECLARE_PRIVATE
    NV_DISABLE_COPY(VItem)
};

NV_NAMESPACE_END
