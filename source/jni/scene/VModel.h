#pragma once

#include "VMatrix.h"
#include "VEvent.h"

NV_NAMESPACE_BEGIN

class VModel
{
    public:
        VModel();
        ~VModel();
        bool loadAsync(VString& modelPath,std::function<void()> completeListener);
        void draw(int eye);
        static void command(const VEvent &event );
    private:
        NV_DECLARE_PRIVATE
};


NV_NAMESPACE_END