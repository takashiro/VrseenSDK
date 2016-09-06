#pragma once
#include "vglobal.h"
#include "VGraphicsItem.h"
#include "3rdparty/nanovg/nanovg.h"

#define ICON_SEARCH 0x1F50D
#define ICON_CIRCLED_CROSS 0x2716
#define ICON_CHEVRON_RIGHT 0xE75E
#define ICON_CHECK 0x2713
#define ICON_LOGIN 0xE740
#define ICON_TRASH 0xE729

NV_NAMESPACE_BEGIN

class VWidget : public VGraphicsItem
{
public:
    VWidget(){};
    ~VWidget(){};

    void drawButton(NVGcontext* vg, int preicon, const char* text, float x, float y, float w, float h, NVGcolor col);


protected:
    void paint(VGuiPainter *painter) override;
private:
};
NV_NAMESPACE_END
