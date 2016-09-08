#pragma once

#include "VGraphicsItem.h"

NV_NAMESPACE_BEGIN

class VProgressBar : public VGraphicsItem
{
public:
    VProgressBar(VGraphicsItem *parent = nullptr);
    ~VProgressBar();

protected:
    void paint(VPainter *painter) override;

private:
    NV_DECLARE_PRIVATE
    NV_DISABLE_COPY(VProgressBar)
};