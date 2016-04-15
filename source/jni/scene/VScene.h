#pragma once

#include "VColor.h"
#include "../core/VArray.h"

NV_NAMESPACE_BEGIN

class VItem;

class VScene
{
public:
    VScene();
    ~VScene();

    void add(VItem *item);
    void remove(VItem *item);

    const VColor &backgroundColor() const;
    void setBackgroundColor(const VColor &color);

    void update();

    VItem* addEyeItem(VItem* parent=0);
    VArray<VItem*> getEyeItemList();
private:
    NV_DECLARE_PRIVATE
    NV_DISABLE_COPY(VScene)
};

NV_NAMESPACE_END
