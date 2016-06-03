#include "UI/UITexture.h"
#include "CinemaApp.h"
#include "VZipFile.h"

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
	Texture = NervGear::LoadTextureFromApplicationPackage( assetPath, VTexture::Flags( VTexture::NoDefault ), Width, Height );
}

} // namespace OculusCinema
