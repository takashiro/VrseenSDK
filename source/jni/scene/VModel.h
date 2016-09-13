#pragma once

#include "VMatrix.h"
#include "VEvent.h"

NV_NAMESPACE_BEGIN

class VModel
{
    public:
        VModel();
        ~VModel();
        bool load(VString& modelPath);
        void draw(int eye, const VMatrix4f & mvp );
        static void command(const VEvent &event );
    private:
        NV_DECLARE_PRIVATE
};


NV_NAMESPACE_END