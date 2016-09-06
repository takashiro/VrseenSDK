#pragma once

#include "VGraphicsItem.h"
#include "VColor.h"
#include "vglobal.h"
#include "VString.h"

NV_NAMESPACE_BEGIN

class VGuiText: public VGraphicsItem {

public:
    VGuiText(VGraphicsItem* parent = nullptr);
    ~VGuiText();

    VColor textColor() const;
    void setTextColor(const VColor &color);

    VString textValue() const;
    void setTextValue(const VString &text);

    void init(void *vg) override;


protected:
    void paint(VPainter *painter) override;

private:
    NV_DECLARE_PRIVATE;
    NV_DISABLE_COPY(VGuiText);
};

NV_NAMESPACE_BEGIN
