#pragma once

#include "vglobal.h"
#include "VArray.h"

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

protected:
    virtual void paint();

private:
    NV_DECLARE_PRIVATE
    NV_DISABLE_COPY(VItem)
};

NV_NAMESPACE_END
