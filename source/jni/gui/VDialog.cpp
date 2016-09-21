/*
 * VDialog.cpp
 *
 *  Created on: 2016年3月30日
 *      Author: yangkai
 */

#include <App.h>
#include "VDialog.h"
#include "VTimer.h"
NV_NAMESPACE_BEGIN
VDialog::VDialog():dialogStopSeconds(0.0f),
                    dialogTexture(nullptr)
{

}

void VDialog::draw(int eye )
{
    // draw the pop-up dialog
    const float now = VTimer::Seconds();
    if ( now >= dialogStopSeconds ) return;

    const float fadeSeconds = 0.5f;
    const float f = now - ( dialogStopSeconds - fadeSeconds );
    const float clampF = f < 0.0f ? 0.0f : f;
    const float alpha = 1.0f - clampF;

    const VGlShader & prog = externalTextureProgram2;
    glUseProgram( prog.program );
    glUniform4f(prog.uniformColor, 1, 1, 1, alpha );

    if(VEyeItem::settings.useMultiview)
    {
        VMatrix4f modelViewProMatrix[2];
        modelViewProMatrix[0] = vApp->getModelViewProMatrix(0);
        modelViewProMatrix[1] = vApp->getModelViewProMatrix(1);
        glUniformMatrix4fv(prog.uniformModelViewProMatrix, 2, GL_TRUE,modelViewProMatrix[0].data());
    }
    else glUniformMatrix4fv(prog.uniformModelViewProMatrix, 1, GL_FALSE, vApp->getModelViewProMatrix(eye).transposed().cell[0] );

    // It is important that panels write to destination alpha, or they
    // might get covered by an overlay plane/cube in TimeWarp.
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_EXTERNAL_OES, dialogTexture->textureId);
    glEnable( GL_BLEND );
    glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
    panelGeometry.drawElements();
    glDisable( GL_BLEND );
    glBindTexture(GL_TEXTURE_EXTERNAL_OES, 0 ); // don't leave it bound
}

VDialog::~VDialog()
{
    externalTextureProgram2.destroy();
    panelGeometry.destroy();
}

void VDialog::init()
{
    externalTextureProgram2.useMultiview = true;
    externalTextureProgram2.initShader( VGlShader::getAdditionalVertexShaderSource(), VGlShader::getAdditionalFragmentShaderSource() );
    panelGeometry.createPlaneQuadGrid( 32, 16 );
}


NV_NAMESPACE_END


