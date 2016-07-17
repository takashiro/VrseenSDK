#if !defined( Native_h )
#define Native_h

NV_USING_NAMESPACE

namespace OculusCinema {

class Native {
public:
	static void			OneTimeInit( App *app, jclass mainActivityClass );
	static void			OneTimeShutdown();

    static VString		GetExternalCacheDirectory( App *app );  	// returns path to app specific writable directory
    static bool 		CreateVideoThumbnail(const VString &videoFilePath, const VString &outputFilePath, const int width, const int height );
	static bool			CheckForMovieResume(const VString &movieName );

	static bool			IsPlaying( App *app );
	static bool 		IsPlaybackFinished( App *app );
	static bool 		HadPlaybackError( App *app );

	static int 			GetPosition( App *app );
	static int 			GetDuration( App *app );
	static void 		SetPosition( App *app, int positionMS );
	static void 		SeekDelta( App *app, int deltaMS );

	static void 		StartMovie(const VString &movieName, bool resumePlayback, bool isEncrypted, bool loop );
	static void 		PauseMovie( App *app );
	static void 		ResumeMovie( App *app );
	static void 		StopMovie( App *app );
	static bool			TogglePlaying( App *app );
};

} // namespace OculusCinema

#endif // Native_h
