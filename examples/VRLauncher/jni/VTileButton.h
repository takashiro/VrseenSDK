#pragma once

#include "VTexture.h"
#include "VColor.h"
#include "VGraphicsItem.h"

NV_NAMESPACE_BEGIN

class VTileButton : public VGraphicsItem
{
public:
    VTileButton(VGraphicsItem *parent = nullptr);
    ~VTileButton();

    void setRect(const VRect3f &rect);
    void setImage(const VTexture &image);
    void setBackgroundColor(const VColor &color);

protected:
    void onFocus() override;
    void onBlur() override;

private:
    NV_DECLARE_PRIVATE
    NV_DISABLE_COPY(VTileButton)
};
