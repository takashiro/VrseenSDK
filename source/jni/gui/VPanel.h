/*
 * VPanel.h
 *
 *  Created on: 2016年3月30日
 *      Author: yangkai
 */

#pragma once
#include "api/VGlShader.h"
#include "api/VGlGeometry.h"
#include "VMatrix.h"
NV_NAMESPACE_BEGIN
class VPanel
{
public:
    VGlShader       externalTextureProgram2;
    VGlGeometry     panelGeometry;      // used for dialogs
    void draw( const GLuint externalTextureId, const VMatrix4f & dialogMvp, const float alpha );

};
NV_NAMESPACE_END
