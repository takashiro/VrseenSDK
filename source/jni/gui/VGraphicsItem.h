#pragma once

#include "VRect3.h"
#include "VMatrix4.h"
#include "VTouchEvent.h"
#include "VKeyEvent.h"

#include <functional>

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

    const VVect3f &pos() const;
    void setPos(const VVect3f &pos);

    bool isVisible() const;
    void setVisible(bool visible);

    virtual bool isFixed() const;
    virtual bool needCursor(const VMatrix4f &mvp) const;

    VVect3f globalPos() const;

    const VRect3f &boundingRect() const;

    double stareElapsedTime() const;
    void setStareElapsedTime(double elapsed);

    bool hasFocus() const;

    const VMatrix4f &transform() const;
    void updateTransform();

    void setOnFocusListener(const std::function<void()> &listener);
    void setOnBlurListener(const std::function<void()> &listener);
    void setOnStareListener(const std::function<void()> &listener);
    void setOnTouchListener(const std::function<void(const VTouchEvent &)> &listener);
    void setOnKeyPressListener(const std::function<void(const VKeyEvent &)> &listener);

protected:
    virtual void init(void *vg);

    virtual void onFocus();
    virtual void onBlur();
    virtual void onStare();
    virtual void onTouch(const VTouchEvent &event);
    virtual void onKeyPress(const VKeyEvent &event);

    virtual void paint(VPainter *painter);
    void setBoundingRect(const VRect3f &rect);

private:
    void onTouchEvent(const VTouchEvent &event);
    void onKeyEvent(const VKeyEvent &event);
    void onSensorChanged(const VMatrix4f &mvp);
    void setParent(VGraphicsItem *parent);

    NV_DECLARE_PRIVATE
    NV_DISABLE_COPY(VGraphicsItem)
};

NV_NAMESPACE_END
