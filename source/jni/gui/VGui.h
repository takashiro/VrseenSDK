#pragma once

#include "VMatrix4.h"
#include "VColor.h"

NV_NAMESPACE_BEGIN

class VGraphicsItem;

class VGui
{
public:
    VGui();
    ~VGui();

    void init();

    void onTouchEvent(int action, float x, float y);
    void onKeyEvent(int keyCode, int repeatCount);

    void prepare();
    void update(const VMatrix4f &mvp);
    void commit();

    int viewWidth() const;
    void setViewWidth(int width);

    int viewHeight() const;
    void setViewHeight(int height);

    VColor backgroundColor() const;
    void setBackgroundColor(const VColor &color);

    VGraphicsItem *root() const;
    void addItem(VGraphicsItem *item);

    void showLoading(uint duration) const;
    void removeLoading() const;

    VMatrix4f getMVP() const;

private:
    NV_DECLARE_PRIVATE
    NV_DISABLE_COPY(VGui)
};

NV_NAMESPACE_END
