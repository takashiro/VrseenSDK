#pragma once

#include "VMatrix.h"

NV_NAMESPACE_BEGIN

class VModel
{
    public:
        VModel();
        ~VModel();
        bool load(VString& modelPath);
        void draw(int eye, const VMatrix4f & mvp );
    private:
        NV_DECLARE_PRIVATE
};


NV_NAMESPACE_END