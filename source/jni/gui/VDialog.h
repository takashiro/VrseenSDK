/*
 * VDialog.h
 *
 *  Created on: 2016年3月30日
 *      Author: yangkai
 */

#pragma once
#include "VMatrix.h"
#include "vglobal.h"
#include "VPanel.h"
#include "scene/SurfaceTexture.h"
NV_NAMESPACE_BEGIN
class VDialog
{
public:
    VR4Matrixf      dialogMatrix;
    float           dialogStopSeconds;
    SurfaceTexture * dialogTexture;

    void drawDialog( VPanel &panel, const VR4Matrixf & mvp );
};
NV_NAMESPACE_END
