#include "CinemaStrings.h"
#include "VrLocale.h"
#include "CinemaApp.h"
#include "BitmapFont.h"

namespace OculusCinema
{

VString CinemaStrings::LoadingMenu_Title;

VString CinemaStrings::Category_Trailers;
VString CinemaStrings::Category_MyVideos;

VString CinemaStrings::MovieSelection_Resume;
VString CinemaStrings::MovieSelection_Next;

VString CinemaStrings::ResumeMenu_Title;
VString CinemaStrings::ResumeMenu_Resume;
VString CinemaStrings::ResumeMenu_Restart;

VString CinemaStrings::TheaterSelection_Title;

VString CinemaStrings::Error_NoVideosOnPhone;
VString CinemaStrings::Error_NoVideosInMyVideos;
VString CinemaStrings::Error_UnableToPlayMovie;

VString CinemaStrings::MoviePlayer_Reorient;

void CinemaStrings::OneTimeInit( CinemaApp &cinema )
{
	vInfo("CinemaStrings::OneTimeInit");

    VrLocale::GetString( vApp->vrJni(), vApp->javaObject(), "@string/LoadingMenu_Title", 		"@string/LoadingMenu_Title", 		LoadingMenu_Title );
    VrLocale::GetString( vApp->vrJni(), vApp->javaObject(), "@string/Category_Trailers", 		"@string/Category_Trailers", 		Category_Trailers );
    VrLocale::GetString( vApp->vrJni(), vApp->javaObject(), "@string/Category_MyVideos", 		"@string/Category_MyVideos", 		Category_MyVideos );
    VrLocale::GetString( vApp->vrJni(), vApp->javaObject(), "@string/MovieSelection_Resume",	"@string/MovieSelection_Resume",	MovieSelection_Resume );
    VrLocale::GetString( vApp->vrJni(), vApp->javaObject(), "@string/MovieSelection_Next", 		"@string/MovieSelection_Next", 		MovieSelection_Next );
    VrLocale::GetString( vApp->vrJni(), vApp->javaObject(), "@string/ResumeMenu_Title", 		"@string/ResumeMenu_Title", 		ResumeMenu_Title );
    VrLocale::GetString( vApp->vrJni(), vApp->javaObject(), "@string/ResumeMenu_Resume", 		"@string/ResumeMenu_Resume", 		ResumeMenu_Resume );
    VrLocale::GetString( vApp->vrJni(), vApp->javaObject(), "@string/ResumeMenu_Restart", 		"@string/ResumeMenu_Restart", 		ResumeMenu_Restart );
    VrLocale::GetString( vApp->vrJni(), vApp->javaObject(), "@string/TheaterSelection_Title", 	"@string/TheaterSelection_Title", 	TheaterSelection_Title );

    VrLocale::GetString( vApp->vrJni(), vApp->javaObject(), "@string/Error_NoVideosOnPhone", 	"@string/Error_NoVideosOnPhone", 	Error_NoVideosOnPhone );

    VrLocale::GetString( vApp->vrJni(), vApp->javaObject(), "@string/Error_NoVideosInMyVideos", "@string/Error_NoVideosInMyVideos", Error_NoVideosInMyVideos );

    VrLocale::GetString( vApp->vrJni(), vApp->javaObject(), "@string/Error_UnableToPlayMovie", 	"@string/Error_UnableToPlayMovie",	Error_UnableToPlayMovie );

    VrLocale::GetString( vApp->vrJni(), vApp->javaObject(), "@string/MoviePlayer_Reorient", 	"@string/MoviePlayer_Reorient", 	MoviePlayer_Reorient );
}

} // namespace OculusCinema
