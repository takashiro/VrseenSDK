#include "VModule.h"

NV_NAMESPACE_BEGIN

namespace {
    struct ModuleManager
    {
        ~ModuleManager()
        {
            const std::list<VModule *> &modules = VModule::List();
            for (VModule *module : modules) {
                delete module;
            }
        }
    };
    static ModuleManager manager;
}

static std::list<VModule *> &ModuleList()
{
    static std::list<VModule *> modules;
    return modules;
}

void VModule::Register(VModule *module)
{
    ModuleList().push_back(module);
}

const std::list<VModule *> &VModule::List()
{
    return ModuleList();
}

NV_NAMESPACE_END
