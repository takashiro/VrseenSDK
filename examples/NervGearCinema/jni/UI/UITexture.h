#pragma once

#include "VEglDriver.h"

namespace OculusCinema {

class UITexture
{
public:
    UITexture();
    ~UITexture();

    void LoadTextureFromApplicationPackage( const char *assetPath );

    int Width;
    int Height;
    GLuint Texture;
};

} // namespace OculusCinema
