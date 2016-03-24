#pragma once

#include "VString.h"
#include "VVariant.h"

NV_NAMESPACE_BEGIN

struct VEvent
{
    VEvent()
    {
    }

    VEvent(const VString &name)
        : name(name)
    {
    }

    bool isValid() const { return !name.isEmpty() || isExecutable(); }

    bool isExecutable() const { return data.isClosure(); }
    void execute() const { data.execute(); }

    VString name;
    VVariant data;
};

NV_NAMESPACE_END
