#pragma once

#include "VColor.h"
#include "3rdparty/nanovg/nanovg.h"

NV_NAMESPACE_BEGIN

class VGraphicsItem;

class VGui
{
public:
    VGui();
    ~VGui();

    int viewWidth() const;
    void setViewWidth(int width);

    int viewHeight() const;
    void setViewHeight(int height);

    VColor backgroundColor() const;
    void setBackgroundColor(const VColor &color);

    void update();

    VGraphicsItem *root() const;

    void addItem(VGraphicsItem *item);

    NVGcontext * getNvContext() const;

private:
    NV_DECLARE_PRIVATE
    NV_DISABLE_COPY(VGui)
};

NV_NAMESPACE_END
