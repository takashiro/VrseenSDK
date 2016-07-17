#if !defined( CinemaStrings_h )
#define CinemaStrings_h

#include <VString.h>

using namespace NervGear;

namespace OculusCinema {

class CinemaApp;

class CinemaStrings {
public:
	static void		OneTimeInit( CinemaApp &cinema );

	static VString	LoadingMenu_Title;

	static VString	Category_Trailers;
	static VString	Category_MyVideos;

	static VString	MovieSelection_Resume;
	static VString	MovieSelection_Next;

	static VString	ResumeMenu_Title;
	static VString	ResumeMenu_Resume;
	static VString	ResumeMenu_Restart;

	static VString	TheaterSelection_Title;

	static VString	Error_NoVideosOnPhone;
	static VString	Error_NoVideosInMyVideos;
	static VString	Error_UnableToPlayMovie;

	static VString	MoviePlayer_Reorient;
};

} // namespace OculusCinema

#endif // CinemaStrings_h
