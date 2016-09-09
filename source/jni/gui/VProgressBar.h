#pragma once

#include "VGraphicsItem.h"
#include "VTexture.h"

NV_NAMESPACE_BEGIN

class VProgressBar : public VGraphicsItem
{
public:
    VProgressBar(VGraphicsItem *parent = nullptr);
    ~VProgressBar();

    void change(float ratio);

    void setRect(const VRect3f & rect3f);

    void setBarImage(const VTexture &tex);
    void setTagImage(const VTexture &tex);
    void setIncImage(const VTexture &tex);

    VPixmap * getBar();
    VPixmap * getTag();
    VPixmap * getInc();

protected:


private:
    NV_DECLARE_PRIVATE
    NV_DISABLE_COPY(VProgressBar)
};

NV_NAMESPACE_END