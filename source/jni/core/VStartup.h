#pragma once

#include "vglobal.h"

#include <list>

NV_NAMESPACE_BEGIN

//Start up functions called by App
extern std::list<void (*)()> NvAppStartupFunctions;

#define NV_ADD_STARTUP_FUNCTION(func) namespace {\
    struct FuncAdder\
    {\
        FuncAdder()\
        {\
            NvAppStartupFunctions.push_back(func);\
        }\
    };\
    FuncAdder adder;\
}

NV_NAMESPACE_END
