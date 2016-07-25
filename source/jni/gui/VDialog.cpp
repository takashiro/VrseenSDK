/*
 * VDialog.cpp
 *
 *  Created on: 2016年3月30日
 *      Author: yangkai
 */

#include "VDialog.h"
#include "VTimer.h"
NV_NAMESPACE_BEGIN
VDialog::VDialog():dialogStopSeconds(0.0f),
                    dialogTexture(nullptr)
{

}
void VDialog::draw( VPanel &panel, const VMatrix4f & mvp )
{
    // draw the pop-up dialog
    const float now = VTimer::Seconds();
    if ( now >= dialogStopSeconds )
    {
        return;
    }
    const VMatrix4f dialogMvp = mvp * dialogMatrix;

    const float fadeSeconds = 0.5f;
    const float f = now - ( dialogStopSeconds - fadeSeconds );
    const float clampF = f < 0.0f ? 0.0f : f;
    const float alpha = 1.0f - clampF;

    panel.draw( dialogTexture->textureId, dialogMvp, alpha );
}
NV_NAMESPACE_END


