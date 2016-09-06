#include "PanoCinema.h"
#include "SceneManager.h"
#include "SurfaceTexture.h"
#include "VAlgorithm.h"
#include <GazeCursor.h>

#include <VKernel.h>
#include <VEglDriver.h>
#include <VTimer.h>
#include <VLog.h>

NV_USING_NAMESPACE

SceneManager::SceneManager( PanoCinema &cinema ) :
	Cinema( cinema ),
	UseOverlay( true ),
	MovieTexture( NULL ),
	MovieTextureTimestamp( 0 ),
	FreeScreenActive( false ),
	FreeScreenScale( 1.0f ),
	FreeScreenDistance( 1.5f ),
	FreeScreenOrientation(),
	FreeScreenAngles(),
	ForceMono( false ),
	CurrentMovieWidth( 0 ),
	CurrentMovieHeight( 480 ),
	MovieTextureWidth( 0 ),
	MovieTextureHeight( 0 ),
	CurrentMovieFormat( VT_2D ),
	MovieRotation( 0 ),
	MovieDuration( 0 ),
	FrameUpdateNeeded( false ),
	ClearGhostsFrames( 0 ),
	UnitSquare(),
	CurrentMipMappedMovieTexture( 0 ),
	MipMappedMovieTextures(),
	MipMappedMovieFBOs(),
	ScreenVignetteTexture( 0 ),
	ScreenVignetteSbsTexture( 0 ),
	Scene(),
	SceneSeatPositions(),
	SceneSeatCount( 0 ),
	SeatPosition( 0 ),
	SceneScreenMatrix(),
//    SceneScreenBounds(0.026997, 0.427766, -3.253125, 5.973003 ,4.391771, -3.253125),
	SceneScreenBounds(20.605499, -3.161200, -16.684299,20.605499, 15.608700, 16.684299),
	AllowMove( false )

{
	MipMappedMovieTextures[0] = MipMappedMovieTextures[1] = MipMappedMovieTextures[2] = 0;

    Scene.YawOffset = 0.0f;
    Scene.Znear = 0.1f;
    Scene.Zfar = 2000.0f;

    // if no seats, create some at the position of the seats in home_theater
    for( int seatPos = -1; seatPos <= 2; seatPos++ )
    {
        SceneSeatPositions[ SceneSeatCount ].z = 3.6f;
        SceneSeatPositions[ SceneSeatCount ].x = 3.0f + seatPos * 0.9f - 0.45;
        SceneSeatPositions[ SceneSeatCount ].y = 0.0f;
        SceneSeatCount++;
    }

    for( int seatPos = -1; seatPos <= 1; seatPos++ )
    {
        SceneSeatPositions[ SceneSeatCount ].z = 1.8f;
        SceneSeatPositions[ SceneSeatCount ].x = 3.0f + seatPos * 0.9f;
        SceneSeatPositions[ SceneSeatCount ].y = -0.3f;
        SceneSeatCount++;
    }
    SetSeat( 1 );
}

void SceneManager::OneTimeInit(const VString &)
{
	vInfo("SceneManager::OneTimeInit");

    const double start = VTimer::Seconds();

    UnitSquare.createPlaneQuadGrid( 1, 1 );

	UseOverlay = true;

	ScreenVignetteTexture = BuildScreenVignetteTexture( 1 );
	ScreenVignetteSbsTexture = BuildScreenVignetteTexture( 2 );

    vInfo("SceneManager::OneTimeInit:" << (VTimer::Seconds() - start) << "seconds");
}

void SceneManager::OneTimeShutdown()
{
	vInfo("SceneManager::OneTimeShutdown");

	// Free GL resources

    UnitSquare.destroy();

	if ( ScreenVignetteTexture != 0 )
	{
		glDeleteTextures( 1, & ScreenVignetteTexture );
		ScreenVignetteTexture = 0;
	}

	if ( ScreenVignetteSbsTexture != 0 )
	{
		glDeleteTextures( 1, & ScreenVignetteSbsTexture );
		ScreenVignetteSbsTexture = 0;
	}
}

//=========================================================================================

static VVect3f MatrixUp( const VMatrix4f & m )
{
    return VVect3f( m.cell[1][0], m.cell[1][1], m.cell[1][2] );
}

static VVect3f MatrixForward( const VMatrix4f & m )
{
    return VVect3f( -m.cell[2][0], -m.cell[2][1], -m.cell[2][2] );
}

static VVect3f AnglesForMatrix( const VMatrix4f &m )
{
    const VVect3f viewForward = MatrixForward( m );
    const VVect3f viewUp = MatrixUp( m );

    VVect3f angles;

	angles.z = 0.0f;

	if ( viewForward.y > 0.7f )
	{
		angles.y = atan2( viewUp.x, viewUp.z );
	}
	else if ( viewForward.y < -0.7f )
	{
		angles.y = atan2( -viewUp.x, -viewUp.z );
	}
	else if ( viewUp.y < 0.0f )
	{
		angles.y = atan2( viewForward.x, viewForward.z );
	}
	else
	{
		angles.y = atan2( -viewForward.x, -viewForward.z );
	}

	angles.x = atan2( viewForward.y, viewUp.y );

	return angles;
}

VVect2f SceneManager::GetScreenSize() const
{
	if ( FreeScreenActive )
	{
        VVect3f size = GetFreeScreenScale();
        return VVect2f( size.x * 2.0f, size.y * 2.0f );
	}
	else
	{
        VVect3f size = SceneScreenBounds.size();
        return VVect2f( size.x, size.y );
	}
}

VVect3f SceneManager::GetFreeScreenScale() const
{
	// Scale is stored in a form that feels linear, raise to exponent to
	// get value to apply.
	const float applyScale = powf( 2.0f, FreeScreenScale );

	// adjust size based on aspect ratio
	float scaleX = 1.0f;
	float scaleY = (float)CurrentMovieHeight / CurrentMovieWidth;
	if ( scaleY > 0.6f )
	{
		scaleX *= 0.6f / scaleY;
		scaleY = 0.6f;
	}

    return VVect3f( applyScale * scaleX, applyScale * scaleY, applyScale );
}

VMatrix4f SceneManager::FreeScreenMatrix() const
{
    const VVect3f scale = GetFreeScreenScale();
	return FreeScreenOrientation *
            VMatrix4f::Translation( 0, 0, -FreeScreenDistance * scale.z ) *
            VMatrix4f::Scaling( scale );
}

// Aspect is width / height
VMatrix4f SceneManager::BoundsScreenMatrix(const VRect3f &bounds, const float movieAspect) const
{
    const VVect3f size = bounds.size();
    const VVect3f center = bounds.start + size * 0.5f;
	const float	screenHeight = size.y;
    const float screenWidth = std::max( size.x, size.z );
	float widthScale;
	float heightScale;
	float aspect = ( movieAspect == 0.0f ) ? 1.0f : movieAspect;
	if ( screenWidth / screenHeight > aspect )
	{	// screen is wider than movie, clamp size to height
		heightScale = screenHeight * 0.5f;
		widthScale = heightScale * aspect;
	}
	else
	{	// screen is taller than movie, clamp size to width
		widthScale = screenWidth * 0.5f;
		heightScale = widthScale / aspect;
	}

	float rotateAngle = ( size.x > size.z ) ? 0.0f : M_PI * 0.5f;
	if(center.x > 0) rotateAngle = -rotateAngle;

    return	VMatrix4f::Translation( center ) *
            VMatrix4f::RotationY( rotateAngle ) *
            VMatrix4f::Scaling( widthScale, heightScale, 1.0f );
}

VMatrix4f SceneManager::ScreenMatrix() const
{
	if ( FreeScreenActive )
	{
		return FreeScreenMatrix();
	}
	else
	{
		return BoundsScreenMatrix( SceneScreenBounds,
			( CurrentMovieHeight == 0 ) ? 1.0f : ( (float)CurrentMovieWidth / CurrentMovieHeight ) );
	}
}

bool SceneManager::GetUseOverlay() const
{
	// Don't enable the overlay when in throttled state
	//return ( UseOverlay && !ovr_GetPowerLevelStateThrottled() );

	// Quality is degraded too much when disabling the overlay.
	// Default behavior is 30Hz TW + no chromatic aberration
	return UseOverlay;
}

void SceneManager::ClearMovie()
{
	MovieTextureTimestamp = 0;
	FrameUpdateNeeded = true;
	CurrentMovieWidth = 0;
	MovieRotation = 0;
	MovieDuration = 0;

	delete MovieTexture;
	MovieTexture = NULL;
}

void SceneManager::SetFreeScreenAngles( const VVect3f &angles )
{
	FreeScreenAngles = angles;

    VMatrix4f rollPitchYaw = VMatrix4f::RotationY( FreeScreenAngles.y ) * VMatrix4f::RotationX( FreeScreenAngles.x );
    const VVect3f ForwardVector( 0.0f, 0.0f, -1.0f );
    const VVect3f UpVector( 0.0f, 1.0f, 0.0f );
    const VVect3f forward = rollPitchYaw.transform( ForwardVector );
    const VVect3f up = rollPitchYaw.transform( UpVector );

    VVect3f shiftedEyePos = Scene.CenterEyePos();
    VVect3f headModelOffset = Scene.HeadModelOffset( 0.0f, FreeScreenAngles.x, FreeScreenAngles.y,
			Scene.ViewParms.headModelDepth, Scene.ViewParms.headModelHeight );
	shiftedEyePos += headModelOffset;
    VMatrix4f result = VMatrix4f::LookAtRH( shiftedEyePos, shiftedEyePos + forward, up );

    FreeScreenOrientation = result.inverted();
}

void SceneManager::PutScreenInFront()
{
    FreeScreenOrientation = Scene.ViewMatrix.inverted();
    vApp->recenterYaw( false );
}

void SceneManager::ClampScreenToView()
{
	if ( !FreeScreenActive )
	{
		return;
	}

    VVect3f viewAngles = AnglesForMatrix( Scene.ViewMatrix );
    VVect3f deltaAngles = FreeScreenAngles - viewAngles;

	if ( deltaAngles.y > M_PI )
	{	// screen is a bit under PI and view is a bit above -PI
		deltaAngles.y -= 2 * M_PI;
	}
	else if ( deltaAngles.y < -M_PI )
	{	// screen is a bit above -PI and view is a bit below PI
		deltaAngles.y += 2 * M_PI;
	}
	deltaAngles.y = VAlgorithm::Clamp( deltaAngles.y, -( float )M_PI * 0.20f, ( float )M_PI * 0.20f );

	if ( deltaAngles.x > M_PI )
	{	// screen is a bit under PI and view is a bit above -PI
		deltaAngles.x -= 2 * M_PI;
	}
	else if ( deltaAngles.x < -M_PI )
	{	// screen is a bit above -PI and view is a bit below PI
		deltaAngles.x += 2 * M_PI;
	}
	deltaAngles.x = VAlgorithm::Clamp( deltaAngles.x, -( float )M_PI * 0.125f, ( float )M_PI * 0.125f );

	SetFreeScreenAngles( viewAngles + deltaAngles );
}

void SceneManager::ClearGazeCursorGhosts()
{
	// clear gaze cursor to avoid seeing it lerp
	ClearGhostsFrames = 3;
}


//============================================================================================

GLuint SceneManager::BuildScreenVignetteTexture( const int horizontalTile ) const
{

	// make it an even border at 16:9 aspect ratio, let it get a little squished at other aspects
	static const int scale = 6;
	static const int width = 16 * scale * horizontalTile;
	static const int height = 9 * scale;
	unsigned char buffer[width * height];
	memset( buffer, 255, sizeof( buffer ) );
	for ( int i = 0; i < width; i++ )
	{
		buffer[i] = 0;
		buffer[width*height - 1 - i] = 0;
	}
	for ( int i = 0; i < height; i++ )
	{
		buffer[i * width] = 0;
		buffer[i * width + width - 1] = 0;
		if ( horizontalTile == 2 )
		{
			buffer[i * width + width / 2 - 1] = 0;
			buffer[i * width + width / 2] = 0;
		}
	}
    GLuint texId;
	glGenTextures( 1, &texId );
    glBindTexture( GL_TEXTURE_2D, texId );
    glTexImage2D( GL_TEXTURE_2D, 0, GL_LUMINANCE, width, height, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, buffer );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
    glBindTexture( GL_TEXTURE_2D, 0 );

    VEglDriver::logErrorsEnum( "screenVignette" );
    return texId;
}

int SceneManager::BottomMipLevel( const int width, const int height ) const
{
	int bottomMipLevel = 0;
    int dimension = std::max( width, height );

	while( dimension > 1 )
	{
		bottomMipLevel++;
		dimension >>= 1;
	}

	return bottomMipLevel;
}

void SceneManager::SetSeat( int newSeat )
{
	SeatPosition = newSeat;
	Scene.FootPos = SceneSeatPositions[ SeatPosition ];
}

bool SceneManager::ChangeSeats( const VFrame & vrFrame )
{
	bool changed = false;
	if ( SceneSeatCount > 0 )
	{
        VVect3f direction( 0.0f );
		if ( vrFrame.input.buttonPressed & BUTTON_LSTICK_UP )
		{
			changed = true;
			direction.z += 1.0f;
		}
		if ( vrFrame.input.buttonPressed & BUTTON_LSTICK_DOWN )
		{
			changed = true;
			direction.z -= 1.0f;
		}
		if ( vrFrame.input.buttonPressed & BUTTON_LSTICK_RIGHT )
		{
			changed = true;
			direction.x += 1.0f;
		}
		if ( vrFrame.input.buttonPressed & BUTTON_LSTICK_LEFT )
		{
			changed = true;
			direction.x -= 1.0f;
		}

		if ( changed )
		{
			// Find the closest seat in the desired direction away from the current seat.
			direction.normalize();
			const float distance = direction.dotProduct( Scene.FootPos );
            float bestSeatDistance = VConstants<float>::MaxValue;
			int bestSeat = -1;
			for ( int i = 0; i < SceneSeatCount; i++ )
			{
				const float d = direction.dotProduct( SceneSeatPositions[i] ) - distance;
				if ( d > 0.01f && d < bestSeatDistance )
				{
					bestSeatDistance = d;
					bestSeat = i;
				}
			}
			if ( bestSeat != -1 )
			{
				SetSeat( bestSeat );
			}
		}
	}

	return changed;
}

/*
 * Command
 *
 * Actions that need to be performed on the render thread.
 */
bool SceneManager::Command(const VEvent &event)
{
	// Always include the space in MatchesHead to prevent problems
	// with commands with matching prefixes.
    vInfo("SceneManager::Command:" << event.name);

    if (event.name == "newVideo") {
		delete MovieTexture;
        MovieTexture = new SurfaceTexture( vApp->vrJni() );
        vInfo( "RC_NEW_VIDEO texId" << MovieTexture->textureId);

        VEventLoop *receiver = static_cast<VEventLoop *>(event.data.toPointer());
        receiver->post("surfaceTexture", MovieTexture->javaObject);

		// don't draw the screen until we have the new size
		CurrentMovieWidth = 0;
		return true;
	}

    if (event.name =="video") {
        int width = event.data.at(0).toInt();
        int height = event.data.at(1).toInt();
//        MovieRotation = event.data.at(2).toInt();
//        MovieDuration = event.data.at(3).toInt();

		const MovieDef *movie = Cinema.currentMovie();
        vAssert(movie);

		// always use 2d form lobby movies
		if ( ( movie == NULL ))
		{
			CurrentMovieFormat = VT_2D;
		}
		else
		{
			CurrentMovieFormat = movie->Format;

			// if movie format is not set, make some assumptions based on the width and if it's 3D
			if ( movie->Format == VT_UNKNOWN )
			{
				if ( movie->Is3D )
				{
					if ( width > height * 3 )
					{
						CurrentMovieFormat = VT_LEFT_RIGHT_3D_FULL;
					}
					else
					{
						CurrentMovieFormat = VT_LEFT_RIGHT_3D;
					}
				}
				else
				{
					CurrentMovieFormat = VT_2D;
				}
			}
		}

		MovieTextureWidth = width;
		MovieTextureHeight = height;

		// Disable overlay on larger movies to reduce judder
        longlong numberOfPixels = MovieTextureWidth * MovieTextureHeight;
        vInfo("Movie size:" << MovieTextureWidth << MovieTextureHeight << "=" << numberOfPixels << "pixels");

		// use the void theater on large movies
		if ( numberOfPixels > 1920 * 1080 )
		{
			vInfo("Oversized movie.  Switching to Void scene to reduce judder");
			UseOverlay = false;

			// downsize the screen resolution to fit into a 960x540 buffer
			float aspectRatio = ( float )width / ( float )height;
			if ( aspectRatio < 1.0f )
			{
				MovieTextureWidth = aspectRatio * 540.0f;
				MovieTextureHeight = 540;
			}
			else
			{
				MovieTextureWidth = 960;
				MovieTextureHeight = aspectRatio * 960.0f;
			}
		}

		switch( CurrentMovieFormat )
		{
			case VT_LEFT_RIGHT_3D_FULL:
				CurrentMovieWidth = width / 2;
				CurrentMovieHeight = height;
				break;

			case VT_TOP_BOTTOM_3D_FULL:
				CurrentMovieWidth = width;
				CurrentMovieHeight = height / 2;
				break;

			default:
				CurrentMovieWidth = width;
				CurrentMovieHeight = height;
				break;
		}

		// Create the texture that we will mip map from the external image
		for ( int i = 0 ; i < 3 ; i++ )
		{
			if ( MipMappedMovieFBOs[i] )
			{
				glDeleteFramebuffers( 1, &MipMappedMovieFBOs[i] );
			}
			if ( MipMappedMovieTextures[i] )
			{
				glDeleteTextures( 1, &MipMappedMovieTextures[i] );
			}
			glGenTextures( 1, &MipMappedMovieTextures[i] );
			glBindTexture( GL_TEXTURE_2D, MipMappedMovieTextures[i] );

            glTexImage2D( GL_TEXTURE_2D, 0, vApp->appInterface()->wantSrgbFramebuffer() ? GL_SRGB8_ALPHA8 :GL_RGBA,
					MovieTextureWidth, MovieTextureHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL );

			glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
			glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );
			glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR );
			glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );

			glGenFramebuffers( 1, &MipMappedMovieFBOs[i] );
			glBindFramebuffer( GL_FRAMEBUFFER, MipMappedMovieFBOs[i] );
			glFramebufferTexture2D( GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
					MipMappedMovieTextures[i], 0 );
			glBindFramebuffer( GL_FRAMEBUFFER, 0 );
		}
		glBindTexture( GL_TEXTURE_2D, 0 );

		return true;
	}

	return false;
}

/*
 * DrawEyeView
 */
VMatrix4f SceneManager::DrawEyeView( const int eye, const float fovDegrees )
{
	const bool drawScreen = (MovieTexture && ( CurrentMovieWidth > 0 ));

	// otherwise cracks would show overlay texture
	glClearColor( 0.0f, 0.0f, 0.0f, 1.0f );
	glClear( GL_COLOR_BUFFER_BIT );

	Scene.DrawEyeView( eye, fovDegrees );

    const VMatrix4f mvp = Scene.MvpForEye( eye, fovDegrees );

	// draw the screen on top
	if ( !drawScreen )
	{
		return mvp;
	}

    glDisable( GL_DEPTH_TEST );

	const VGlShader * prog = &Cinema.shaderMgr.MovieExternalUiProgram;
	glUseProgram( prog->program );
	glUniform4f( prog->uniformColor, 1, 1, 1, 0.0f );

	glVertexAttrib4f( 2, 1.0f, 1.0f, 1.0f, 1.0f );	// no color attributes on the surface verts, so force to 1.0

	//
	// draw the movie texture
	//
	if ( !GetUseOverlay())
	{
        // no overlay
		vApp->swapParms().WarpProgram = WP_CHROMATIC;
		vApp->swapParms().Images[eye][1].TexId = 0;

		glActiveTexture( GL_TEXTURE0 );
		glBindTexture( GL_TEXTURE_EXTERNAL_OES, MovieTexture->textureId );

		glActiveTexture( GL_TEXTURE1 );
		glBindTexture( GL_TEXTURE_2D, ScreenVignetteTexture );

		glUniformMatrix4fv( prog->uniformTexMatrix, 1, GL_FALSE, /* not transposed */
							getTexMatrix(eye).transposed().data());
		// The UI is always identity for now, but we may scale it later

        VMatrix4f identity;
		glUniformMatrix4fv( prog->uniformTexMatrix2, 1, GL_FALSE, /* not transposed */
                identity.transposed().data());

	    const VMatrix4f screenModel = ScreenMatrix();
        const VMatrix4f screenMvp = mvp * screenModel;
        glUniformMatrix4fv(prog->uniformModelViewProMatrix, 1, GL_FALSE, screenMvp.transposed().data());
        UnitSquare.drawElements();

		glBindTexture( GL_TEXTURE_EXTERNAL_OES, 0 );	// don't leave it bound
	}
	else
	{
		// use overlay
        const VMatrix4f screenModel = ScreenMatrix();
        const VMatrix4f mv = Scene.ViewMatrixForEye( eye ) * screenModel;

	    vApp->swapParms().WarpProgram = WP_CHROMATIC_MASKED_PLANE;
		vApp->swapParms().Images[eye][1].TexId = MipMappedMovieTextures[CurrentMipMappedMovieTexture];
		vApp->swapParms().Images[eye][1].Pose = vApp->sensorForNextWarp();
        vApp->swapParms().Images[eye][1].TexCoordsFromTanAngles = getTexMatrix(eye) * mv.tanAngleMatrixFromUnitSquare();

		// explicitly clear a hole in alpha
        const VMatrix4f screenMvp = mvp * screenModel;
        vApp->drawScreenMask( screenMvp, 0.0f, 0.0f );
	}

	// The framework will automatically draw the floating elements on top of us now.
	return mvp;
}

/*
 * Frame()
 *
 * App override
 */
VMatrix4f SceneManager::Frame( const VFrame & vrFrame )
{
	// disallow player movement
	VFrame vrFrameWithoutMove = vrFrame;
	if ( !AllowMove )
	{
		vrFrameWithoutMove.input.sticks[0][0] = 0.0f;
		vrFrameWithoutMove.input.sticks[0][1] = 0.0f;
	}
    Scene.Frame( vApp->viewSettings(), vrFrameWithoutMove, vApp->swapParms().ExternalVelocity);

	if ( ClearGhostsFrames > 0 )
	{
        vApp->gazeCursor().ClearGhosts();
		ClearGhostsFrames--;
	}

	// Check for new movie frames
	// latch the latest movie frame to the texture.
	if ( MovieTexture && CurrentMovieWidth )
	{
		glActiveTexture( GL_TEXTURE0 );
		MovieTexture->Update();
		glBindTexture( GL_TEXTURE_EXTERNAL_OES, 0 );
		if ( MovieTexture->nanoTimeStamp != MovieTextureTimestamp )
		{
			MovieTextureTimestamp = MovieTexture->nanoTimeStamp;
			FrameUpdateNeeded = true;
		}
	}

	// build the mip maps
	if ( FrameUpdateNeeded )
	{
		FrameUpdateNeeded = false;
		CurrentMipMappedMovieTexture = (CurrentMipMappedMovieTexture+1)%3;
		glActiveTexture( GL_TEXTURE1 );
		if ( CurrentMovieFormat == VT_LEFT_RIGHT_3D || CurrentMovieFormat == VT_LEFT_RIGHT_3D_FULL )
		{
			glBindTexture( GL_TEXTURE_2D, ScreenVignetteSbsTexture );
		}
		else
		{
			glBindTexture( GL_TEXTURE_2D, ScreenVignetteTexture );
		}
		glActiveTexture( GL_TEXTURE0 );
		glBindFramebuffer( GL_FRAMEBUFFER, MipMappedMovieFBOs[CurrentMipMappedMovieTexture] );
		glDisable( GL_DEPTH_TEST );
		glDisable( GL_SCISSOR_TEST );
        VEglDriver::glDisableFramebuffer( true, false );
		glViewport( 0, 0, MovieTextureWidth, MovieTextureHeight );
        if ( vApp->appInterface()->wantSrgbFramebuffer() )
		{	// we need this copied without sRGB conversion on the top level
            glDisable( VEglDriver::GL_FRAMEBUFFER_SRGB_EXT );
		}
		if ( CurrentMovieWidth > 0 )
		{
			glBindTexture( GL_TEXTURE_EXTERNAL_OES, MovieTexture->textureId );
			glUseProgram( Cinema.shaderMgr.CopyMovieProgram.program );
            UnitSquare.drawElements();
			glBindTexture( GL_TEXTURE_EXTERNAL_OES, 0 );
            if ( vApp->appInterface()->wantSrgbFramebuffer() )
			{	// we need this copied without sRGB conversion on the top level
                glEnable( VEglDriver::GL_FRAMEBUFFER_SRGB_EXT );
			}
		}
		else
		{
			// If the screen is going to be black because of a movie change, don't
			// leave the last dynamic color visible.
			glClearColor( 0.2f, 0.2f, 0.2f, 0.2f );
			glClear( GL_COLOR_BUFFER_BIT );
		}
		glBindFramebuffer( GL_FRAMEBUFFER, 0 );

		// texture 2 will hold the mip mapped screen
		glActiveTexture( GL_TEXTURE2 );
		glBindTexture( GL_TEXTURE_2D, MipMappedMovieTextures[CurrentMipMappedMovieTexture] );
		glGenerateMipmap( GL_TEXTURE_2D );
		glBindTexture( GL_TEXTURE_2D, 0 );

        VEglDriver::glFlush();
	}

	return Scene.CenterViewMatrix();
}

VMatrix4f  SceneManager::getTexMatrix(int eye)
{
	// allow stereo movies to also be played in mono
	const int stereoEye = ForceMono ? 0 : eye;

	const VMatrix4f stretchTop(
			1, 0, 0, 0,
			0, 0.5f, 0, 0,
			0, 0, 1, 0,
			0, 0, 0, 1 );
	const VMatrix4f stretchBottom(
			1, 0, 0, 0,
			0, 0.5, 0, 0.5f,
			0, 0, 1, 0,
			0, 0, 0, 1 );
	const VMatrix4f stretchRight(
			0.5f, 0, 0, 0.5f,
			0, 1, 0, 0,
			0, 0, 1, 0,
			0, 0, 0, 1 );
	const VMatrix4f stretchLeft(
			0.5f, 0, 0, 0,
			0, 1, 0, 0,
			0, 0, 1, 0,
			0, 0, 0, 1 );

	const VMatrix4f rotate90(
			0, 1, 0, 0,
			-1, 0, 0, 1,
			0, 0, 1, 0,
			0, 0, 0, 1 );

	const VMatrix4f rotate180(
			-1, 0, 0, 1,
			0, -1, 0, 1,
			0, 0, 1, 0,
			0, 0, 0, 1 );

	const VMatrix4f rotate270(
			0, -1, 0, 1,
			1, 0, 0, 0,
			0, 0, 1, 0,
			0, 0, 0, 1 );

	VMatrix4f texMatrix;

	switch ( CurrentMovieFormat )
	{
		case VT_LEFT_RIGHT_3D:
		case VT_LEFT_RIGHT_3D_FULL:
			texMatrix = ( stereoEye ? stretchRight : stretchLeft );
			break;
		case VT_TOP_BOTTOM_3D:
		case VT_TOP_BOTTOM_3D_FULL:
			texMatrix = ( stereoEye ? stretchBottom : stretchTop );
			break;
		default:
			switch( MovieRotation )
			{
				case 90 :
					texMatrix = rotate90;
					break;
				case 180 :
					texMatrix = rotate180;
					break;
				case 270 :
					texMatrix = rotate270;
					break;
			}
			break;
	}
	return  texMatrix;
}