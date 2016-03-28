#if !defined( Native_h )
#define Native_h

namespace OculusCinema {

class Native {
public:
	static void			OneTimeInit( App *app, jclass mainActivityClass );
	static void			OneTimeShutdown();

    static VString		GetExternalCacheDirectory( App *app );  	// returns path to app specific writable directory
    static bool 		CreateVideoThumbnail(const NervGear::VString &videoFilePath, const NervGear::VString &outputFilePath, const int width, const int height );
	static bool			CheckForMovieResume( App *app, const char * movieName );

	static bool			IsPlaying( App *app );
	static bool 		IsPlaybackFinished( App *app );
	static bool 		HadPlaybackError( App *app );

	static int 			GetPosition( App *app );
	static int 			GetDuration( App *app );
	static void 		SetPosition( App *app, int positionMS );
	static void 		SeekDelta( App *app, int deltaMS );

	static void 		StartMovie( App *app, const char * movieName, bool resumePlayback, bool isEncrypted, bool loop );
	static void 		PauseMovie( App *app );
	static void 		ResumeMovie( App *app );
	static void 		StopMovie( App *app );
	static bool			TogglePlaying( App *app );
};

} // namespace OculusCinema

#endif // Native_h
