#include "ShaderManager.h"
#include "PanoCinema.h"
#include "core/VTimer.h"


NV_USING_NAMESPACE

//=======================================================================================

static const char* copyMovieVertexShaderSrc =
	"uniform highp mat4 Mvpm;\n"
	"uniform highp mat4 Texm;\n"
	"attribute vec4 Position;\n"
	"attribute vec2 TexCoord;\n"
	"varying  highp vec2 oTexCoord;\n"
	"void main()\n"
	"{\n"
	"   gl_Position = Position;\n"
	"   oTexCoord = vec2( TexCoord.x, 1.0 - TexCoord.y );\n"	// need to flip Y
	"}\n";

static const char* copyMovieFragmentShaderSource =
	"#extension GL_OES_EGL_image_external : require\n"
	"uniform samplerExternalOES Texture0;\n"
	"uniform sampler2D Texture1;\n"				// edge vignette
	"varying highp vec2 oTexCoord;\n"
	"void main()\n"
	"{\n"
	"	gl_FragColor = texture2D( Texture0, oTexCoord ) *  texture2D( Texture1, oTexCoord );\n"
	"}\n";

static const char* movieUiVertexShaderSrc =
	"uniform highp mat4 Mvpm[NUM_VIEWS];\n"
	"uniform highp mat4 Texm[NUM_VIEWS];\n"
	"attribute vec4 Position;\n"
	"attribute vec2 TexCoord;\n"
	"uniform lowp vec4 UniformColor;\n"
	"varying  highp vec2 oTexCoord;\n"
	"varying  lowp vec4 oColor;\n"
	"void main()\n"
	"{\n"
	"   gl_Position = Mvpm[VIEW_ID] * Position;\n"
	"   oTexCoord = vec2( Texm[VIEW_ID] * vec4(TexCoord,1,1) );\n"
	"   oColor = UniformColor;\n"
	"}\n";


const char* movieExternalUiFragmentShaderSource =
	"#extension GL_OES_EGL_image_external : require\n"
	"uniform samplerExternalOES Texture0;\n"
	"uniform sampler2D Texture1;\n"	// fade / clamp texture
	"uniform lowp vec4 ColorBias;\n"
	"varying highp vec2 oTexCoord;\n"
	"varying lowp vec4	oColor;\n"
	"void main()\n"
	"{\n"
	"	lowp vec4 movieColor = texture2D( Texture0, oTexCoord ) * texture2D( Texture1, oTexCoord );\n"
	"	gl_FragColor = ColorBias + oColor * movieColor;\n"
	"}\n";


//=======================================================================================

ShaderManager::ShaderManager( PanoCinema &cinema ) :
	Cinema( cinema )
{
}

void ShaderManager::OneTimeInit(const VString &)
{
	vInfo("ShaderManager::OneTimeInit");

    const double start = VTimer::Seconds();

	MovieExternalUiProgram.useMultiview = true;
	MovieExternalUiProgram 		.initShader( movieUiVertexShaderSrc, movieExternalUiFragmentShaderSource );
	CopyMovieProgram 			.initShader( copyMovieVertexShaderSrc, copyMovieFragmentShaderSource );
	overlayScreenFadeMaskProgram.useMultiview = true;
	overlayScreenFadeMaskProgram.initShader(VGlShader::getUntextureInverseColorVertexShaderSource(),VGlShader::getUntexturedFragmentShaderSource() );

    vInfo("ShaderManager::OneTimeInit:" << (VTimer::Seconds() - start) << "seconds");
}

void ShaderManager::OneTimeShutdown()
{
	vInfo("ShaderManager::OneTimeShutdown");

	 MovieExternalUiProgram .destroy();
	 CopyMovieProgram .destroy();
	 overlayScreenFadeMaskProgram.destroy();
}
