/*
 * VDialog.h
 *
 *  Created on: 2016年3月30日
 *      Author: yangkai
 */

#pragma once

#include <api/VGlShader.h>
#include <api/VGlGeometry.h>
#include "VMatrix.h"
#include "vglobal.h"
#include "scene/SurfaceTexture.h"
NV_NAMESPACE_BEGIN
class VDialog
{
public:
    VDialog();
    ~VDialog();

    void init();
    VMatrix4f      dialogMatrix;
    float           dialogStopSeconds;
    SurfaceTexture * dialogTexture;

    void draw(int eye);
private:
    VGlShader       externalTextureProgram2;
    VGlGeometry     panelGeometry;      // used for dialogs
};
NV_NAMESPACE_END
