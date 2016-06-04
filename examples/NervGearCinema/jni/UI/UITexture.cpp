#include "UI/UITexture.h"
#include "CinemaApp.h"
#include "VZipFile.h"
#include "VResource.h"

namespace OculusCinema {

UITexture::UITexture() :
	Width( 0 ),
	Height( 0 ),
	Texture( 0 )

{
}

UITexture::~UITexture()
{
}

void UITexture::LoadTextureFromApplicationPackage( const char *assetPath )
{
    VTexture texture{VResource{assetPath}};
    Texture = texture.id();
    Width = texture.width();
    Height = texture.height();
}

} // namespace OculusCinema
