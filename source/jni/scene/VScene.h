#pragma once

#include "vglobal.h"

NV_NAMESPACE_BEGIN

class VItem;

class VScene
{
public:
    VScene();
    ~VScene();

    void add(VItem *item);
    void remove(VItem *item);

    void update();

private:
    NV_DECLARE_PRIVATE
    NV_DISABLE_COPY(VScene)
};

NV_NAMESPACE_END
