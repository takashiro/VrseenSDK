#pragma once

#include "vglobal.h"

NV_NAMESPACE_BEGIN

class VPath;
class VIODevice;
class VImage;

class VImageLoader
{
public:
	virtual ~VImageLoader() = default;

    virtual bool isValid(const VPath &fileName) const = 0;
    virtual bool isValid(VIODevice *input) const = 0;

    virtual VImage *load(VIODevice *input) const = 0;
};

NV_NAMESPACE_END
