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
	LOG( "CinemaStrings::OneTimeInit" );

	App *app = cinema.app;

	VrLocale::GetString( app->vrJni(), app->javaObject(), "@string/LoadingMenu_Title", 		"@string/LoadingMenu_Title", 		LoadingMenu_Title );
	VrLocale::GetString( app->vrJni(), app->javaObject(), "@string/Category_Trailers", 		"@string/Category_Trailers", 		Category_Trailers );
	VrLocale::GetString( app->vrJni(), app->javaObject(), "@string/Category_MyVideos", 		"@string/Category_MyVideos", 		Category_MyVideos );
	VrLocale::GetString( app->vrJni(), app->javaObject(), "@string/MovieSelection_Resume",	"@string/MovieSelection_Resume",	MovieSelection_Resume );
	VrLocale::GetString( app->vrJni(), app->javaObject(), "@string/MovieSelection_Next", 		"@string/MovieSelection_Next", 		MovieSelection_Next );
	VrLocale::GetString( app->vrJni(), app->javaObject(), "@string/ResumeMenu_Title", 		"@string/ResumeMenu_Title", 		ResumeMenu_Title );
	VrLocale::GetString( app->vrJni(), app->javaObject(), "@string/ResumeMenu_Resume", 		"@string/ResumeMenu_Resume", 		ResumeMenu_Resume );
	VrLocale::GetString( app->vrJni(), app->javaObject(), "@string/ResumeMenu_Restart", 		"@string/ResumeMenu_Restart", 		ResumeMenu_Restart );
	VrLocale::GetString( app->vrJni(), app->javaObject(), "@string/TheaterSelection_Title", 	"@string/TheaterSelection_Title", 	TheaterSelection_Title );

	VrLocale::GetString( app->vrJni(), app->javaObject(), "@string/Error_NoVideosOnPhone", 	"@string/Error_NoVideosOnPhone", 	Error_NoVideosOnPhone );

	VrLocale::GetString( app->vrJni(), app->javaObject(), "@string/Error_NoVideosInMyVideos", "@string/Error_NoVideosInMyVideos", Error_NoVideosInMyVideos );

	VrLocale::GetString( app->vrJni(), app->javaObject(), "@string/Error_UnableToPlayMovie", 	"@string/Error_UnableToPlayMovie",	Error_UnableToPlayMovie );

	VrLocale::GetString( app->vrJni(), app->javaObject(), "@string/MoviePlayer_Reorient", 	"@string/MoviePlayer_Reorient", 	MoviePlayer_Reorient );
}

} // namespace OculusCinema
