#pragma once

#include "vglobal.h"
#include "VString.h"

NV_NAMESPACE_BEGIN

class VSoundManager
{
public:
    VSoundManager();
    ~VSoundManager();

    void loadSoundAssets();
    bool hasSound(const VString &soundName);
    bool getSound(const VString &soundName, VString &outSound);

private:
    NV_DECLARE_PRIVATE
    NV_DISABLE_COPY(VSoundManager)
};

NV_NAMESPACE_END
