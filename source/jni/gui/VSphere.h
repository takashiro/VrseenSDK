//
// Created by Gin on 2016/9/20.
//
#include "VGraphicsItem.h"
#include "VColor.h"

NV_NAMESPACE_BEGIN


class VSphere : public VGraphicsItem
{
public:
    VSphere(VGraphicsItem *parent = nullptr);
    ~VSphere();

    VRect3f rect() const;
    void setRect(const VRect3f &rect);

    VColor color() const;
    void setColor(const VColor &color);

    bool isFixed() const override ;

protected:
    void paint(VPainter *painter) override;

private:
    NV_DECLARE_PRIVATE
    NV_DISABLE_COPY(VSphere)
};


NV_NAMESPACE_END