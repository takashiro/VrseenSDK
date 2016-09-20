#pragma once

#include "VGraphicsItem.h"
#include "VColor.h"

NV_NAMESPACE_BEGIN

class VShadow : public VGraphicsItem
{
public:
    VShadow(VGraphicsItem *parent = nullptr);
    ~VShadow();

    VRect3f rect() const;
    void setRect(const VRect3f &rect);

    VColor color() const;
    void setColor(const VColor &color);

    bool isFixed() const override ;

protected:
    void paint(VPainter *painter) override;

private:
    NV_DECLARE_PRIVATE
    NV_DISABLE_COPY(VShadow)
};

NV_NAMESPACE_END
