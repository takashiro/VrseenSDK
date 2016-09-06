#pragma once

#include "VMatrix4.h"

NV_NAMESPACE_BEGIN

class VGraphicsItem;

class VGui
{
public:
    VGui();
    ~VGui();

    void update(const VMatrix4f &mvp);

    VGraphicsItem *root() const;

    void addItem(VGraphicsItem *item);

private:
    NV_DECLARE_PRIVATE
    NV_DISABLE_COPY(VGui)
};

NV_NAMESPACE_END
