#pragma once

#include "vglobal.h"

#include "VRMenu.h"

NV_NAMESPACE_BEGIN

class App;

class OvrOutOfSpaceMenu : public VRMenu
{
public:
    static char const *	MENU_NAME;
    static OvrOutOfSpaceMenu * Create( App * app );

    void 	buildMenu( int memoryInKB );

private:
    OvrOutOfSpaceMenu( App * app );
    App * m_app;
};

NV_NAMESPACE_END
