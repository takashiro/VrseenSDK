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
    VDialog();
    VMatrix4f      dialogMatrix;
    float           dialogStopSeconds;
    SurfaceTexture * dialogTexture;

    void draw( VPanel &panel, const VMatrix4f & mvp );
};
NV_NAMESPACE_END
