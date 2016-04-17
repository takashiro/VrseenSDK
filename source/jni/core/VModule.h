#pragma once

#include "vglobal.h"

#include <list>

NV_NAMESPACE_BEGIN

class VModule
{
public:
    virtual ~VModule() {}

    virtual void onStart() {}
    virtual void onPause() {}
    virtual void onResume() {}
    virtual void onDestroy() {}

    static void Register(VModule *module);
    static const std::list<VModule *> &List();
};

#define NV_ADD_MODULE(module) namespace {\
    struct ModuleAdder\
    {\
        ModuleAdder()\
        {\
            VModule::Register(new module);\
        }\
    };\
    ModuleAdder adder;\
}

NV_NAMESPACE_END
