/*
 * VPanel.cpp
 *
 *  Created on: 2016年3月30日
 *      Author: yangkai
 */
#include "VPanel.h"
NV_NAMESPACE_BEGIN
void VPanel::draw( const GLuint externalTextureId, const VR4Matrixf & dialogMvp, const float alpha )
{
    const VGlShader & prog = externalTextureProgram2;
    glUseProgram( prog.program );
    glUniform4f(prog.uniformColor, 1, 1, 1, alpha );

    VR4Matrixf identity;
    identity.transpose();
    glUniformMatrix4fv(prog.uniformTexMatrix, 1, GL_FALSE, identity.data());
    glUniformMatrix4fv(prog.uniformModelViewProMatrix, 1, GL_FALSE, dialogMvp.transposed().cell[0] );

    // It is important that panels write to destination alpha, or they
    // might get covered by an overlay plane/cube in TimeWarp.
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_EXTERNAL_OES, externalTextureId);
    glEnable( GL_BLEND );
    glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
    panelGeometry.drawElements();
    glDisable( GL_BLEND );
    glBindTexture(GL_TEXTURE_EXTERNAL_OES, 0 ); // don't leave it bound
}
NV_NAMESPACE_END

