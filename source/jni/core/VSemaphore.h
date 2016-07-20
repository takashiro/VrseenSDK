#pragma once

#include "vglobal.h"

NV_NAMESPACE_BEGIN

class VSemaphore
{
public:
    VSemaphore(uint value = 0);
    ~VSemaphore();

    bool wait();
    bool post();

    int available() const;

private:
    NV_DECLARE_PRIVATE
    NV_DISABLE_COPY(VSemaphore)
};

NV_NAMESPACE_END
