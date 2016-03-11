#include "UI/UITexture.h"
#include "CinemaApp.h"
#include "VApkFile.h"

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
	Texture = NervGear::LoadTextureFromApplicationPackage( assetPath, TextureFlags_t( TEXTUREFLAG_NO_DEFAULT ), Width, Height );
}

} // namespace OculusCinema
