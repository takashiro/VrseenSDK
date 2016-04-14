#pragma once

#include "vglobal.h"

#include <list>

NV_NAMESPACE_BEGIN

typedef void (*VStartup)();

std::list<VStartup> &VStartupList();

#define NV_ADD_STARTUP_FUNCTION(func) namespace {\
    struct FuncAdder\
    {\
        FuncAdder()\
        {\
            VStartupList().push_back(func);\
        }\
    };\
    FuncAdder adder;\
}

NV_NAMESPACE_END
