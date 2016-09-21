/************************************************************************************

 Filename    :   BitmapFont.cpp
 Content     :   Monospaced bitmap font rendering intended for debugging only.
 Created     :   March 11, 2014
 Authors     :   Jonathan E. Wright

 Copyright   :   Copyright 2014 Oculus VR, LLC. All Rights reserved.

 *************************************************************************************/

// TODO:
// - add support for multiple fonts per surface using texture arrays (store texture in 3rd texture coord)
// - in-world text really should sort with all other transparent surfaces
//
#include "BitmapFont.h"
#include "VAlgorithm.h"

#include <errno.h>
#include <math.h>
#include <pthread.h>
#include <sched.h>
#include <unistd.h>			// for usleep
#include <android/sensor.h>

#include <sys/time.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <sstream>

#include "VPath.h"
#include "VJson.h"
#include "VZipFile.h"
#include "VLog.h"
#include "App.h"

#include "android/JniUtils.h"
#include "api/VEglDriver.h"

#include "../api/VGlShader.h"
#include "VTexture.h"
#include "../api/VGlGeometry.h"
#include "VString.h"

NV_NAMESPACE_BEGIN

char const* FontSingleTextureVertexShaderSrc = "uniform mat4 Mvpm[NUM_VIEWS];\n"
		"uniform lowp vec4 UniformColor;\n"
		"attribute vec4 Position;\n"
		"attribute vec2 TexCoord;\n"
		"attribute vec4 VertexColor;\n"
		"attribute vec4 FontParms;\n"
		"varying highp vec2 oTexCoord;\n"
		"varying lowp vec4 oColor;\n"
		"varying vec4 oFontParms;\n"
		"void main()\n"
		"{\n"
		"    gl_Position = Mvpm[VIEW_ID] * Position;\n"
		"    oTexCoord = TexCoord;\n"
		"    oColor = UniformColor * VertexColor;\n"
		"    oFontParms = FontParms;\n"
		"}\n";

// Use derivatives to make the faded color and alpha boundaries a
// consistent thickness regardless of font scale.
char const* SDFFontFragmentShaderSrc =
		"#extension GL_OES_standard_derivatives : require\n"
				"uniform sampler2D Texture0;\n"
				"varying highp vec2 oTexCoord;\n"
				"varying lowp vec4 oColor;\n"
				"varying mediump vec4 oFontParms;\n"
				"void main()\n"
				"{\n"
				"    mediump float distance = texture( Texture0, oTexCoord ).r;\n"
				"    mediump float ds = oFontParms.z * 255.0;\n"
				"	 mediump float dd = fwidth( oTexCoord.x ) * 8.0 * ds;\n"
				"    mediump float ALPHA_MIN = oFontParms.x - dd;\n"
				"    mediump float ALPHA_MAX = oFontParms.x + dd;\n"
				"    mediump float COLOR_MIN = oFontParms.y - dd;\n"
				"    mediump float COLOR_MAX = oFontParms.y + dd;\n"
				"	gl_FragColor.xyz = ( oColor * ( clamp( distance, COLOR_MIN, COLOR_MAX ) - COLOR_MIN ) / ( COLOR_MAX - COLOR_MIN ) ).xyz;\n"
				"	gl_FragColor.w = oColor.w * ( clamp( distance, ALPHA_MIN, ALPHA_MAX ) - ALPHA_MIN ) / ( ALPHA_MAX - ALPHA_MIN );\n"
				"}\n";

class FontGlyphType {
public:
	FontGlyphType() :
			CharCode(0), X(0.0f), Y(0.0f), Width(0.0f), Height(0.0f), AdvanceX(
					0.0f), AdvanceY(0.0f), BearingX(0.0f), BearingY(0.0f) {
	}

	int32_t CharCode;
	float X;
	float Y;
	float Width;
	float Height;
	float AdvanceX;
	float AdvanceY;
	float BearingX;
	float BearingY;
};

class FontInfoType {
public:
	static int FNT_FILE_VERSION;

	// This is used to scale the UVs to world units that work with the current scale values used throughout
	// the native code. Unfortunately the original code didn't account for the image size before factoring
	// in the user scale, so this keeps everything the same.
	static const float DEFAULT_SCALE_FACTOR;

	FontInfoType() :
			NaturalWidth(0.0f), NaturalHeight(0.0f), HorizontalPad(0), VerticalPad(
					0), FontHeight(0), ScaleFactorX(1.0f), ScaleFactorY(1.0f), TweakScale(
					1.0f), CenterOffset(0.0f), MaxAscent(0.0f), MaxDescent(0.0f) {
	}

    bool Load(const VZipFile &languagePackageFile, const VString &fileName);
	FontGlyphType const & GlyphForCharCode(uint32_t const charCode) const;

	std::string FontName; // name of the font (not necessarily the file name)
	std::string CommandLine; // command line used to generate this font
	std::string ImageFileName; // the file name of the font image
	float NaturalWidth; // width of the font image before downsampling to SDF
	float NaturalHeight; // height of the font image before downsampling to SDF
	float HorizontalPad; // horizontal padding for all glyphs
	float VerticalPad; // vertical padding for all glyphs
	float FontHeight; // vertical distance between two baselines (i.e. two lines of text)
	float ScaleFactorX; // x-axis scale factor
	float ScaleFactorY; // y-axis scale factor
	float TweakScale; // additional scale factor used to tweak the size of other-language fonts
	float CenterOffset; // +/- value applied to "center" distance in the signed distance field. Range [-1,1]. A negative offset will make the font appear bolder.
	float MaxAscent; // maximum ascent of any character
	float MaxDescent; // maximum descent of any character
    VArray<FontGlyphType> Glyphs; // info about each glyph in the font
    VArray<int32_t> CharCodeMap; // index by character code to get the index of a glyph for the character

private:
    bool LoadFromPackage(const VZipFile &packageFile, const VString &fileName);
	bool LoadFromBuffer(void const * buffer, size_t const bufferSize);
};

int FontInfoType::FNT_FILE_VERSION = 1; // initial version storing pixel locations and scaling post/load to fix some precision loss
// for now, we're not going to increment this so that we're less likely to have dependency issues with loading the font from Home
// int FontInfoType::FNT_FILE_VERSION = 2;		// added TweakScale for manual adjustment of other-language fonts
const float FontInfoType::DEFAULT_SCALE_FACTOR = 512.0f;

class BitmapFontLocal: public BitmapFont {
public:
	BitmapFontLocal() :
			Texture(0), ImageWidth(0), ImageHeight(0) {
	}
	~BitmapFontLocal() {
		if (Texture != 0) {
			glDeleteTextures(1, &Texture);
		}
		Texture = 0;
	}

    bool Load(const VString &languagePackageFileName, const VString &fontInfoFileName) override;

	// Calculates the native (unscaled) width of the text string. Line endings are ignored.
    float CalcTextWidth(const VString &text) const override;

	// Calculates the native (unscaled) width of the text string. Each '\n' will start a new line
	// and will increase the height by FontInfo.FontHeight. For multi-line strings, lineWidths will
	// contain the width of each individual line of text and width will be the width of the widest
	// line of text.
    void CalcTextMetrics(const VString &text, size_t & len, float & width,
			float & height, float & ascent, float & descent, float & fontHeight,
            float * lineWidths, int const maxLines, int & numLines) const override;

	void WordWrapText(VString & inOutText, const float widthMeters,
			const float fontScale = 1.0f) const;
	void WordWrapText(VString & inOutText, const float widthMeters,
            VArray<VString> wholeStrsList,
			const float fontScale = 1.0f) const;

	FontGlyphType const & GlyphForCharCode(uint32_t const charCode) const {
		return FontInfo.GlyphForCharCode(charCode);
	}
	FontInfoType const & GetFontInfo() const {
		return FontInfo;
	}
	const VGlShader & GetFontProgram() const {
		return FontProgram;
	}
	int GetImageWidth() const {
		return ImageWidth;
	}
	int GetImageHeight() const {
		return ImageHeight;
	}
	GLuint GetTexture() const {
		return Texture;
	}

private:
	FontInfoType FontInfo;
	GLuint Texture;
	int ImageWidth;
	int ImageHeight;

	VGlShader FontProgram;

private:
    bool LoadImage(const VZipFile &languagePackageFile,
            const VString &imageName);
    bool LoadImageFromBuffer(const VString &imageName,
            const uchar *buffer, size_t const bufferSize,
            bool const isASTC);
	bool LoadFontInfo(char const * glyphFileName);
	bool LoadFontInfoFromBuffer(unsigned char const * buffer,
			size_t const bufferSize);
};

//==================================================================================================
// BitmapFontSurfaceLocal
//
class BitmapFontSurfaceLocal: public BitmapFontSurface {
public:
	struct fontVertex_t {
		fontVertex_t() :
				xyz(0.0f), s(0.0f), t(0.0f), rgba(), fontParms() {
		}

        VVect3f xyz;
		float s;
		float t;
		uchar rgba[4];
		uchar fontParms[4];
	};

	typedef unsigned short fontIndex_t;

	BitmapFontSurfaceLocal();
	virtual ~BitmapFontSurfaceLocal();

	virtual void Init(const int maxVertices);
	void Free();

	// add text to the VBO that will render in a 2D pass.
    void DrawText3D(BitmapFont const & font, const fontParms_t & flags,
            const VVect3f & pos, VVect3f const & normal, VVect3f const & up,
            float const scale, VVect4f const & color, const VString &text) override;
    void DrawText3Df(BitmapFont const & font, const fontParms_t & flags,
            const VVect3f & pos, VVect3f const & normal, VVect3f const & up,
            float const scale, VVect4f const & color, const char *text, ...) override;

    virtual void DrawTextBillboarded3D(BitmapFont const & font,
            fontParms_t const & flags, VVect3f const & pos, float const scale,
            VVect4f const & color, const VString &text);

	// transform the billboarded font strings
    virtual void Finish(VMatrix4f const & viewMatrix);

	// render the VBO
	virtual void Render3D(BitmapFont const & font,
			const int eye) const;

private:
	VGlGeometry Geo; // font glyphs
	fontVertex_t * Vertices; // vertices that are written to the VBO
	int MaxVertices;
	int MaxIndices;
	int CurVertex; // reset every Render()
	int CurIndex; // reset every Render()

	struct vertexBlockFlags_t {
		bool FaceCamera; // true to always face the camera
	};

	// The vertices in a vertex block are in local space and pre-scaled.  They are transformed into
	// world space and stuffed into the VBO before rendering (once the current MVP is known).
	// The vertices can be pivoted around the Pivot point to face the camera, then an additional
	// rotation applied.
	class VertexBlockType {
	public:
		VertexBlockType() :
				Font(NULL), Verts(NULL), NumVerts(0), Pivot(0.0f), Rotation(), Billboard(
						true), TrackRoll(false) {
		}

		VertexBlockType(VertexBlockType const & other) :
				Font(NULL), Verts(NULL), NumVerts(0), Pivot(0.0f), Rotation(), Billboard(
						true), TrackRoll(false) {
			Copy(other);
		}

		VertexBlockType & operator=(VertexBlockType const & other) {
			Copy(other);
			return *this;
		}

		void Copy(VertexBlockType const & other) {
			if (&other == this) {
				return;
			}
			delete[] Verts;
			Font = other.Font;
			Verts = other.Verts;
			NumVerts = other.NumVerts;
			Pivot = other.Pivot;
			Rotation = other.Rotation;
			Billboard = other.Billboard;
			TrackRoll = other.TrackRoll;

			other.Font = NULL;
			other.Verts = NULL;
			other.NumVerts = 0;
		}

		VertexBlockType(BitmapFont const & font, int const numVerts,
                VVect3f const & pivot, VQuatf const & rot, bool const billboard,
				bool const trackRoll) :
				Font(&font), NumVerts(numVerts), Pivot(pivot), Rotation(rot), Billboard(
						billboard), TrackRoll(trackRoll) {
			Verts = new fontVertex_t[numVerts];
		}

		~VertexBlockType() {
			Free();
		}

		void Free() {
			Font = NULL;
			delete[] Verts;
			Verts = NULL;
			NumVerts = 0;
		}

		mutable BitmapFont const * Font; // the font used to render text into this vertex block
		mutable fontVertex_t * Verts; // the vertices
		mutable int NumVerts; // the number of vertices in the block
        VVect3f Pivot; // postion this vertex block can be rotated around
        VQuatf Rotation; // additional rotation to apply
		bool Billboard; // true to always face the camera
		bool TrackRoll; // if true, when billboarded, roll with the camera
	};

	VArray<VertexBlockType> VertexBlocks; // each pointer in the array points to an allocated block ov

	// We cast BitmapFont to BitmapFontLocal internally so that we do not have to expose
	// a lot of BitmapFontLocal methods in the BitmapFont interface just so BitmapFontSurfaceLocal
	// can use them. This problem comes up because BitmapFontSurface specifies the interface as
	// taking BitmapFont as a parameter, not BitmapFontLocal. This is safe right now because
	// we know that BitmapFont cannot be instantiated, nor is there any class derived from it other
	// than BitmapFontLocal.
	static BitmapFontLocal const & AsLocal(BitmapFont const & font) {
		return *static_cast<BitmapFontLocal const*>(&font);
	}
};

//==============================
// ftoi
#if defined( OVR_CPU_X86_64 )
inline int ftoi( float const f )
{
	return _mm_cvtt_ss2si( _mm_set_ss( f ) );
}
#elif defined( OVR_CPU_x86 )
inline int ftoi( float const f )
{
	int i;
	__asm
	{
		fld f
		fistp i
	}
	return i;
}
#else
inline int ftoi(float const f) {
	return (int) f;
}
#endif

//==============================
// FileSize
static size_t FileSize(FILE * f) {
	if (f == NULL) {
		return 0;
	}
#if defined( WIN32 )
	struct _stat64 stats;
	__int64 r = _fstati64( f->_file, &stats );
#else
	struct stat stats;
	int r = fstat(f->_file, &stats);
#endif
	if (r < 0) {
		return 0;
	}
	return static_cast<size_t>(stats.st_size); // why st_size is signed I have no idea... negative file lengths?
}

//==================================================================================================
// FontInfoType
//==================================================================================================

//==============================
// FontInfoType::LoadFromPackage
bool FontInfoType::LoadFromPackage(const VZipFile &packageFile, const VString &fileName) {
    uint length = 0;
    void *packageBuffer = NULL;

    vInfo("fileName is" << fileName);
    packageFile.read(fileName, packageBuffer, length);
    if (packageBuffer == nullptr) {
		return false;
	}

	size_t fsize;

	unsigned char * buffer = NULL;
	// copy to a zero-terminated buffer for JSON parser
	fsize = length + 1;
	buffer = new unsigned char[fsize];
	memcpy(buffer, packageBuffer, length);
	buffer[length] = '\0';
	free(packageBuffer);

	// try to load from the buffer -- this may fail due to an invalid version
	bool r = LoadFromBuffer(buffer, fsize);
	delete[] buffer;
	return r;
}

//==============================
// FontInfoType::Load
bool FontInfoType::Load(const VZipFile &languagePackageFile, const VString &fileName) {
    if (languagePackageFile.isOpen() && LoadFromPackage(languagePackageFile, fileName)) {
		return true;
	}

	// if it wasn't loaded from the language package, try again from the app package
    const VZipFile &apk = vApp->apkFile();
    return LoadFromPackage(apk, fileName);
}

//==============================
// FontInfoType::LoadFromBuffer
bool FontInfoType::LoadFromBuffer(void const * buffer,
		size_t const bufferSize) {
    NV_UNUSED(bufferSize);
	std::stringstream s;
	s << reinterpret_cast<char const *>(buffer);
	VJson jsonRoot;
	s >> jsonRoot;
    if (jsonRoot.isNull()) {
		vWarn("JSON Error");
		return false;
	}

	int32_t maxCharCode = -1;
	// currently we're only supporting the first unicode plane up to 65k. If we were to support other planes
	// we could conceivably end up with a very sparse 1,114,111 byte type for mapping character codes to
	// glyphs and if that's the case we may just want to use a hash, or use a combination of tables for
	// the first 65K and hashes for the other, less-frequently-used characters.
	static const int MAX_GLYPHS = 0xffff;

	// load the glyphs
	if (jsonRoot.type() != VJson::Object)
		return false;

	int Version = jsonRoot.value("Version").toInt();
	if (Version != FNT_FILE_VERSION) {
		return false;
	}

    FontName = jsonRoot.value("FontName").toStdString();
    CommandLine = jsonRoot.value("CommandLine").toStdString();
    ImageFileName = jsonRoot.value("ImageFileName").toStdString();
	const int numGlyphs = jsonRoot.value("NumGlyphs").toInt();
	if (numGlyphs < 0 || numGlyphs > MAX_GLYPHS) {
		vAssert( numGlyphs > 0 && numGlyphs <= MAX_GLYPHS);
		return false;
	}

	NaturalWidth = jsonRoot.value("NaturalWidth").toDouble();
	NaturalHeight = jsonRoot.value("NaturalHeight").toDouble();

	// we scale everything after loading integer values from the JSON file because the OVR JSON writer loses precision on floats
	double nwScale = 1.0f / NaturalWidth;
	double nhScale = 1.0f / NaturalHeight;

	HorizontalPad = jsonRoot.value("HorizontalPad").toDouble() * nwScale;
	VerticalPad = jsonRoot.value("VerticalPad").toDouble() * nhScale;
	FontHeight = jsonRoot.value("FontHeight").toDouble() * nhScale;
	CenterOffset = jsonRoot.value("CenterOffset").toDouble();
	TweakScale =
			jsonRoot.contains("TweakScale") ?
					jsonRoot.value("TweakScale").toDouble() : 1.0f;

    vInfo("FontName = " << FontName);
    vInfo("CommandLine = " << CommandLine);
    vInfo("HorizontalPad = " << HorizontalPad);
    vInfo("VerticalPad = " << VerticalPad);
    vInfo("FontHeight = " << FontHeight);
    vInfo("CenterOffset = " << CenterOffset);
    vInfo("TweakScale = " << TweakScale);
    vInfo("ImageFileName = " << ImageFileName);
    vInfo("Loading " << numGlyphs << " glyphs.");

/// HACK: this is hard-coded until we do not have a dependcy on reading the font from Home
	if (FontName == "korean.fnt") {
		TweakScale = 0.75f;
		CenterOffset = -0.02f;
	}
/// HACK: end hack

	Glyphs.resize(numGlyphs);

	const VJson jsonGlyphArray = jsonRoot.value("Glyphs");

	double oWidth = 0.0;
	double oHeight = 0.0;

	if (jsonGlyphArray.type() == VJson::Array) {
		const VJsonArray &elements = jsonGlyphArray.toArray();

		int i = 0;
		for (const VJson &jsonGlyph : elements) {
			if (jsonGlyph.type() == VJson::Object) {
				FontGlyphType & g = Glyphs[i];
				g.CharCode = jsonGlyph.value("CharCode").toInt();
				g.X = jsonGlyph.value("X").toDouble();
				g.Y = jsonGlyph.value("Y").toDouble();
				g.Width = jsonGlyph.value("Width").toDouble();
				g.Height = jsonGlyph.value("Height").toDouble();
				g.AdvanceX = jsonGlyph.value("AdvanceX").toDouble();
				g.AdvanceY = jsonGlyph.value("AdvanceY").toDouble();
				g.BearingX = jsonGlyph.value("BearingX").toDouble();
				g.BearingY = jsonGlyph.value("BearingY").toDouble();

				if (g.CharCode == 'O') {
					oWidth = g.Width;
					oHeight = g.Height;
				}

				g.X *= nwScale;
				g.Y *= nhScale;
				g.Width *= nwScale;
				g.Height *= nhScale;
				g.AdvanceX *= nwScale;
				g.AdvanceY *= nhScale;
				g.BearingX *= nwScale;
				g.BearingY *= nhScale;

				float const ascent = g.BearingY;
				float const descent = g.Height - g.BearingY;
				if (ascent > MaxAscent) {
					MaxAscent = ascent;
				}
				if (descent > MaxDescent) {
					MaxDescent = descent;
				}

				maxCharCode = std::max(maxCharCode, g.CharCode);
			}
			i++;
		}
	}

	float const DEFAULT_TEXT_SCALE = 0.0025f;

	double const NATURAL_WIDTH_SCALE = NaturalWidth / 4096.0;
	double const NATURAL_HEIGHT_SCALE = NaturalHeight / 3820.0;
	double const DEFAULT_O_WIDTH = 325.0;
	double const DEFAULT_O_HEIGHT = 322.0;
	double const OLD_WIDTH_FACTOR = 1.04240608;
	float const widthScaleFactor = static_cast<float>(DEFAULT_O_WIDTH / oWidth
			* OLD_WIDTH_FACTOR * NATURAL_WIDTH_SCALE);
	float const heightScaleFactor = static_cast<float>(DEFAULT_O_HEIGHT
			/ oHeight * OLD_WIDTH_FACTOR * NATURAL_HEIGHT_SCALE);

	ScaleFactorX = DEFAULT_SCALE_FACTOR * DEFAULT_TEXT_SCALE * widthScaleFactor
			* TweakScale;
	ScaleFactorY = DEFAULT_SCALE_FACTOR * DEFAULT_TEXT_SCALE * heightScaleFactor
			* TweakScale;

	// This is not intended for wide or ucf character sets -- depending on the size range of
	// character codes lookups may need to be changed to use a hash.
	if (maxCharCode >= MAX_GLYPHS) {
		vAssert( maxCharCode <= MAX_GLYPHS);
		maxCharCode = MAX_GLYPHS;
	}

	// resize the array to the maximum glyph value
	CharCodeMap.resize(maxCharCode + 1);

	// init to empty value
	for (int i = 0; i < CharCodeMap.length(); ++i) {
		CharCodeMap[i] = -1;
	}

	for (int i = 0; i < Glyphs.length(); ++i) {
		FontGlyphType const & g = Glyphs[i];
		CharCodeMap[g.CharCode] = i;
	}

	return true;
}

//==============================
// FontInfoType::GlyphForCharCode
FontGlyphType const & FontInfoType::GlyphForCharCode(
		uint32_t const charCode) const {
	if (charCode >= CharCodeMap.size()) {
		static FontGlyphType emptyGlyph;
		return emptyGlyph;
	}
	const int glyphIndex = CharCodeMap[charCode];

	if (glyphIndex < 0 || glyphIndex >= Glyphs.length()) {
		vWarn("FontInfoType::GlyphForCharCode FAILED TO FIND GLYPH FOR CHARACTER!");
		vWarn("FontInfoType::GlyphForCharCode: charCode " << charCode << " yielding " << glyphIndex);
		vWarn("FontInfoType::GlyphForCharCode: CharCodeMap size " << CharCodeMap.size() << " Glyphs size " << Glyphs.length());

		return Glyphs['*'];
	}

	vAssert( glyphIndex >= 0 && glyphIndex < Glyphs.length());
	return Glyphs[glyphIndex];
}

//==================================================================================================
// BitmapFontLocal
//==================================================================================================

#if defined( NV_OS_WIN )
#define PATH_SEPARATOR '\\'
#define PATH_SEPARATOR_STR "\\"
#define PATH_SEPARATOR_NON_CANONICAL '/'
#else
#define PATH_SEPARATOR '/'
#define PATH_SEPARATOR_STR "/"
#define PATH_SEPARATOR_NON_CANONICAL '\\'
#endif

// TODO: we really need a decent set of functions for path manipulation. OVR_String_PathUtil has
// some bugs and doesn't have functionality for cross-platform path conversion.
static void MakePathCanonical(VString path) {

    int n = path.length();
	for (int i = 0; i < n; ++i) {
		if (path[i] == PATH_SEPARATOR_NON_CANONICAL) {
			path[i] = PATH_SEPARATOR;
		}
	}
}

#if 0	// unused?
static size_t MakePathCanonical( char const * inPath, char * outPath, size_t outSize )
{
	size_t i = 0;
	for (; outPath[i] != '\0' && i < outSize; ++i )
	{
		if ( inPath[i] == PATH_SEPARATOR_NON_CANONICAL )
		{
			outPath[i] = PATH_SEPARATOR;
		}
		else
		{
			outPath[i] = inPath[i];
		}
	}
	if ( i == outSize )
	{
		outPath[outSize - 1 ] = '\0';
	}
	else
	{
		outPath[i] = '\0';
	}
	return i;
}
#endif

static void AppendPath(VString& path, const VString& append) {
    VString appendCanonical;
    appendCanonical = append;
	MakePathCanonical(path);
    int n = path.length();
    if (n > 0&& path[n - 1] != PATH_SEPARATOR && appendCanonical[0] != PATH_SEPARATOR) {
        path += PATH_SEPARATOR_STR;
    }
    path += appendCanonical;
}

static void StripPath(const VString& path, VString& outName) {
	if (path[0] == '\0') {
		outName[0] = '\0';
		return;
	}
    size_t n = path.length();
    VString fnameStart;
	for (int i = n - 1; i >= 0; --i) {
		if (path[i] == PATH_SEPARATOR) {
            fnameStart = path.substr(i + 1);
			break;
		}
	}
    if (fnameStart.length() != 0) {
		// this will copy 0 characters if the path separator was the last character
        outName = fnameStart;
    } else {
        outName = path;
	}
}

static void StripFileName(const VString& path, VString& outPath) {
    size_t n = path.length();
    VString fnameStart;
	for (int i = n - 1; i >= 0; --i) {
		if (path[i] == PATH_SEPARATOR) {
            fnameStart = path.substr(0, i + 1);
			break;
		}
	}
    if (fnameStart.length() != 0) {
        outPath = fnameStart;
	} else {
        outPath = path;
	}
}

//==============================
// BitmapFontLocal::Load
bool BitmapFontLocal::Load(const VString &languagePackageName, const VString &fontInfoFileName) {
    VZipFile languagePackageFile(languagePackageName);
	if (!FontInfo.Load(languagePackageFile, fontInfoFileName)) {
		return false;
	}

	// strip any path from the image file name path and prepend the path from the .fnt file -- i.e. always
	// require them to be loaded from the same directory.
    VString baseName = VPath(FontInfo.ImageFileName).fileName();
    vInfo("fontInfoFileName = " << fontInfoFileName);
	vInfo("image baseName = " << baseName);

    VString imagePath;
    StripFileName(fontInfoFileName, imagePath);
    vInfo("imagePath = " << imagePath);

    VString imageFileName;
    StripPath(fontInfoFileName, imageFileName);
    vInfo("imageFileName = " << imageFileName);

    AppendPath(imagePath, baseName);
    vInfo("imagePath = " << imagePath);
    if (!LoadImage(languagePackageFile, imagePath)) {
		return false;
	}

	// create the shaders for font rendering if not already created
	if (FontProgram.vertexShader == 0 || FontProgram.fragmentShader == 0) {
		FontProgram.useMultiview = true;
		FontProgram.initShader(FontSingleTextureVertexShaderSrc,
				SDFFontFragmentShaderSrc); //SingleTextureFragmentShaderSrc );
	}

	return true;
}

//==============================
// BitmapFontLocal::LoadImage
bool BitmapFontLocal::LoadImage(const VZipFile &languagePackageFile, const VString &imageName)
{
	// try to open the language pack apk
    uint length = 0;
    void *packageBuffer = nullptr;
    if (languagePackageFile.isOpen()) {
        languagePackageFile.read(imageName, packageBuffer, length);
	}

	// one of the following conditions should be true here:
	// - we opened the language apk and read the texture file without error
	// - we opened the language apk and failed to open the texture file
	// - we failed to open the language apk
    if (packageBuffer == nullptr) {
        const VZipFile &apk = vApp->apkFile();
        apk.read(imageName, packageBuffer, length);
	}

	bool result = false;
	if (packageBuffer != NULL) {
        result = LoadImageFromBuffer(imageName,
				(unsigned char const*) packageBuffer, length,
                imageName.endsWith(".astc", false));
		free(packageBuffer);
	} else {
        //TODO Replace the block with VFile
        FILE * f = fopen(imageName.toUtf8().data(), "rb");
		if (f != NULL) {
			size_t fsize = FileSize(f);

			unsigned char * buffer = new unsigned char[fsize];

			size_t countRead = fread(buffer, fsize, 1, f);
			fclose(f);
			f = NULL;
			if (countRead == 1) {
                result = LoadImageFromBuffer(imageName, buffer, fsize,
                        imageName.endsWith(".astc", false));
			}
			delete[] buffer;
		}
	}

	if (!result) {
		vWarn("BitmapFontLocal::LoadImage: failed to load image '" << imageName << "'");
	}
	return result;
}

//==============================
// BitmapFontLocal::LoadImageFromBuffer
bool BitmapFontLocal::LoadImageFromBuffer(const VString &imageName, const uchar * buffer, size_t bufferSize, bool const isASTC)
{
	if (Texture != 0) {
		glDeleteTextures(1, &Texture);
		Texture = 0;
	}

	if (isASTC) {
        VTexture texture;
        texture.loadAstc(buffer, bufferSize, 1);
        Texture = texture.id();
	} else {
        VTexture texture(imageName, VByteArray(reinterpret_cast<const char *>(buffer), bufferSize));
        Texture = texture.id();
        ImageWidth = texture.width();
        ImageHeight = texture.height();
	}
	if (Texture == 0) {
		vWarn("BitmapFontLocal::Load: failed to load '" << imageName << "'");
		return false;
	}

	vInfo("BitmapFontLocal::LoadImageFromBuffer: success");
	return true;
}

//==============================
// BitmapFontLocal::WordWrapText
void BitmapFontLocal::WordWrapText(VString & inOutText, const float widthMeters,
		const float fontScale) const {
    WordWrapText(inOutText, widthMeters, VArray<VString>(), fontScale);
}

//==============================
// BitmapFontLocal::WordWrapText
void BitmapFontLocal::WordWrapText(VString & inOutText, const float widthMeters,
        VArray<VString> wholeStrsList, const float fontScale) const {
	float const xScale = FontInfo.ScaleFactorX * fontScale;
    const int32_t totalLength = (int) inOutText.length();
	int32_t lastWhitespaceIndex = -1;
	double lineWidthAtLastWhitespace = 0.0f;
	double lineWidth = 0.0f;
	int dontSplitUntilIdx = -1;
	for (int32_t pos = 0; pos < totalLength; ++pos) {
        uint32_t charCode = inOutText.at(pos);

		// Replace any existing character escapes with space as we recompute where to insert line breaks
		if (charCode == '\r' || charCode == '\n' || charCode == '\t') {
            inOutText[pos] = ' ';
			charCode = ' ';
		}

		FontGlyphType const & g = GlyphForCharCode(charCode);
		lineWidth += g.AdvanceX * xScale;

		for (int i = 0; i < wholeStrsList.length(); ++i) {
            int curWholeStrLen = (int) wholeStrsList[i].length();
			int endPos = pos + curWholeStrLen;

			if (endPos < totalLength) {
                VString subInStr = inOutText.range(pos, endPos);
				if (subInStr == wholeStrsList[i]) {
					dontSplitUntilIdx = std::max(dontSplitUntilIdx, endPos);
				}
			}
		}

		if (pos >= dontSplitUntilIdx) {
			if (charCode == ' ') {
				lastWhitespaceIndex = pos;
				lineWidthAtLastWhitespace = lineWidth;
			}

			// always check the line width and as soon as we exceed it, wrap the text at
			// the last whitespace. This ensure's the text always fits within the width.
			if (lineWidth >= widthMeters && lastWhitespaceIndex >= 0) {
				dontSplitUntilIdx = -1;
                inOutText[lastWhitespaceIndex] = '\n';
				// subtract the width after the last whitespace so that we don't lose any
				// of the accumulated width since then.
				lineWidth -= lineWidthAtLastWhitespace;
			}
		}
	}
}

//==============================
// BitmapFontLocal::CalcTextWidth
float BitmapFontLocal::CalcTextWidth(const VString &text) const
{
	float width = 0.0f;

    std::u32string ucs4 = text.toUcs4();
    for (char32_t ch : ucs4) {
        if (ch == '\r' || ch == '\n') {
			continue; // skip line endings
		}
        FontGlyphType const & g = GlyphForCharCode(ch);
		width += g.AdvanceX * FontInfo.ScaleFactorX;
	}
	return width;
}

//==============================
// BitmapFontLocal::CalcTextMetrics
void BitmapFontLocal::CalcTextMetrics(const VString &text, size_t & len,
        float & width, float & height, float & firstAscent, float & lastDescent,
        float & fontHeight, float * lineWidths, int const maxLines,
        int & numLines) const {
	len = 0;
	numLines = 0;
	width = 0.0f;
	height = 0.0f;

	if (lineWidths == NULL || maxLines <= 0) {
		return;
	}
    if (text.isEmpty()) {
		return;
	}

	float maxLineAscent = 0.0f;
	float maxLineDescent = 0.0f;
	firstAscent = 0.0f;
	lastDescent = 0.0f;
	fontHeight = FontInfo.FontHeight * FontInfo.ScaleFactorY;
	numLines = 0;
	int charsOnLine = 0;
	lineWidths[0] = 0.0f;

    std::u32string ucs4 = text.toUcs4();
    std::u32string::iterator p = ucs4.begin();
    for (;; len++) {
        uint charCode = *p;
        p++;
		if (charCode == '\r') {
			continue; // skip carriage returns
		}
		if (charCode == '\n' || charCode == '\0') {
			// keep track of the widest line, which will be the width of the entire text block
			if (lineWidths[numLines] > width) {
				width = lineWidths[numLines];
			}

			firstAscent = (numLines == 0) ? maxLineAscent : firstAscent;
			lastDescent = (charsOnLine > 0) ? maxLineDescent : lastDescent;
			charsOnLine = 0;

			if (numLines < maxLines - 1) {
				// if we're not out of array space, advance and zero the width
				numLines++;
				lineWidths[numLines] = 0.0f;
				maxLineAscent = 0.0f;
				maxLineDescent = 0.0f;
			}
			if (charCode == '\0') {
				break;
			}
			continue;
		}

		charsOnLine++;

		FontGlyphType const & g = GlyphForCharCode(charCode);
		lineWidths[numLines] += g.AdvanceX * FontInfo.ScaleFactorX;

		if (numLines == 0) {
			if (g.BearingY > maxLineAscent) {
				maxLineAscent = g.BearingY;
			}
		} else {
			// all lines after the first line are full height
			maxLineAscent = FontInfo.FontHeight;
		}
		float descent = g.Height - g.BearingY;
		if (descent > maxLineDescent) {
			maxLineDescent = descent;
		}
	}

	vAssert( numLines >= 1);

	firstAscent *= FontInfo.ScaleFactorY;
	lastDescent *= FontInfo.ScaleFactorY;
	height = firstAscent;
	height += (numLines - 1) * FontInfo.FontHeight * FontInfo.ScaleFactorY;
	height += lastDescent;

	vAssert( numLines <= maxLines);
}

//==================================================================================================
// BitmapFontSurfaceLocal
//==================================================================================================

//==============================
// BitmapFontSurfaceLocal::BitmapFontSurface
BitmapFontSurfaceLocal::BitmapFontSurfaceLocal() :
		Vertices(NULL), MaxVertices(0), MaxIndices(0), CurVertex(0), CurIndex(0) {
}

//==============================
// BitmapFontSurfaceLocal::~BitmapFontSurfaceLocal
BitmapFontSurfaceLocal::~BitmapFontSurfaceLocal() {
	Geo.destroy();
	delete[] Vertices;
	Vertices = NULL;
}

//==============================
// BitmapFontSurfaceLocal::Init
// Initializes the surface VBO
void BitmapFontSurfaceLocal::Init(const int maxVertices) {
	assert(
			Geo.vertexBuffer == 0 && Geo.indexBuffer == 0 && Geo.vertexArrayObject == 0);
	assert( Vertices == NULL);
	if (Vertices != NULL) {
		delete[] Vertices;
		Vertices = NULL;
	}
	assert( maxVertices % 4 == 0);

	MaxVertices = maxVertices;
	MaxIndices = (maxVertices / 4) * 6;

	Vertices = new fontVertex_t[maxVertices];
	const int vertexByteCount = maxVertices * sizeof(fontVertex_t);

	// font VAO
    VEglDriver::glGenVertexArraysOES(1, &Geo.vertexArrayObject);
    VEglDriver::glBindVertexArrayOES(Geo.vertexArrayObject);

	// vertex buffer
	glGenBuffers(1, &Geo.vertexBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, Geo.vertexBuffer);
	glBufferData(GL_ARRAY_BUFFER, vertexByteCount, (void*) Vertices,
			GL_DYNAMIC_DRAW);


	glEnableVertexAttribArray(VERTEX_POSITION); // x, y and z
	glVertexAttribPointer(VERTEX_POSITION, 3, GL_FLOAT,
			GL_FALSE, sizeof(fontVertex_t), (void*) 0);

	glEnableVertexAttribArray(VERTEX_UVC0); // s and t
	glVertexAttribPointer(VERTEX_UVC0, 2, GL_FLOAT, GL_FALSE,
			sizeof(fontVertex_t), (void*) offsetof( fontVertex_t, s ));

	glEnableVertexAttribArray(VERTEX_COLOR); // color
	glVertexAttribPointer(VERTEX_COLOR, 4, GL_UNSIGNED_BYTE,
			GL_TRUE, sizeof(fontVertex_t),
			(void*) offsetof( fontVertex_t, rgba ));

	glDisableVertexAttribArray(VERTEX_UVC1);

	glEnableVertexAttribArray(FONT_PARMS); // outline parms
	glVertexAttribPointer(FONT_PARMS, 4,
			GL_UNSIGNED_BYTE, GL_TRUE, sizeof(fontVertex_t),
			(void*) offsetof( fontVertex_t, fontParms ));

	fontIndex_t * indices = new fontIndex_t[MaxIndices];
	const int indexByteCount = MaxIndices * sizeof(fontIndex_t);

	// indices never change
	int numQuads = MaxIndices / 6;
	int v = 0;
	for (int i = 0; i < numQuads; i++) {
		indices[i * 6 + 0] = v + 2;
		indices[i * 6 + 1] = v + 1;
		indices[i * 6 + 2] = v + 0;
		indices[i * 6 + 3] = v + 3;
		indices[i * 6 + 4] = v + 2;
		indices[i * 6 + 5] = v + 0;
		v += 4;
	}

	glGenBuffers(1, &Geo.indexBuffer);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, Geo.indexBuffer);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, indexByteCount, (void*) indices,
			GL_STATIC_DRAW);

	Geo.indexCount = 0; // if there's anything to render this will be modified

    VEglDriver::glBindVertexArrayOES(0);

	delete[] indices;

	CurVertex = 0;
	CurIndex = 0;

	vInfo("BitmapFontSurfaceLocal::Init: success");
}

//==============================
// ColorToABGR
int32_t ColorToABGR(VVect4f const & color) {
	// format is ABGR
	return (ftoi(color.w * 255.0f) << 24) | (ftoi(color.z * 255.0f) << 16)
			| (ftoi(color.y * 255.0f) << 8) | ftoi(color.x * 255.0f);
}

//==============================
// BitmapFontSurfaceLocal::DrawText3D
void BitmapFontSurfaceLocal::DrawText3D(BitmapFont const & font,
        fontParms_t const & parms, VVect3f const & pos,
        VVect3f const & normal, VVect3f const & up, float scale,
        VVect4f const & color, const VString &text) {
    if (text.isEmpty()) {
		return; // nothing to do here, move along
	}

	// TODO: multiple line support -- we would need to calculate the horizontal width
	// for each string ending in \n
	size_t len;
	float width;
	float height;
	float ascent;
	float descent;
	float fontHeight;
	int const MAX_LINES = 128;
	float lineWidths[MAX_LINES];
	int numLines;
	AsLocal(font).CalcTextMetrics(text, len, width, height, ascent, descent,
			fontHeight, lineWidths, MAX_LINES, numLines);
//	LOG( "BitmapFontSurfaceLocal::DrawText3D( \"%s\" %s %s ) : width = %.2f, height = %.2f, numLines = %i, fh = %.2f",
//			text, ( parms.AlignVert == VERTICAL_CENTER ) ? "cv" : ( ( parms.AlignVert == VERTICAL_BOTTOM ) ? "bv" : "tv" ),
// 			( parms.AlignHoriz == HORIZONTAL_CENTER ) ? "ch" : ( ( parms.AlignVert == HORIZONTAL_LEFT ) ? "lh" : "rh" ),
//			width, height, numLines, AsLocal( font ).GetFontInfo().FontHeight );
	if (len == 0) {
		return;
	}

    vAssert(normal.isNormalized());
    vAssert(up.isNormalized());

	const FontInfoType & fontInfo = AsLocal(font).GetFontInfo();

	float imageWidth = (float) AsLocal(font).GetImageWidth();
	float const xScale = AsLocal(font).GetFontInfo().ScaleFactorX * scale;
	float const yScale = AsLocal(font).GetFontInfo().ScaleFactorY * scale;

	// allocate a vertex block
	size_t numVerts = 4 * len;
    VertexBlockType vb(font, numVerts, pos, VQuatf(), parms.Billboard,
			parms.TrackRoll);

    VVect3f const right = up.crossProduct(normal);
    VVect3f const r = (parms.Billboard) ? VVect3f(1.0f, 0.0f, 0.0f) : right;
    VVect3f const u = (parms.Billboard) ? VVect3f(0.0f, 1.0f, 0.0f) : up;

    VVect3f curPos(0.0f);
	switch (parms.AlignVert) {
	case VERTICAL_BASELINE:
		break;

	case VERTICAL_CENTER: {
		float const vofs = (height * 0.5f) - ascent;
		curPos += u * (vofs * scale);
		break;
	}
	case VERTICAL_CENTER_FIXEDHEIGHT: {
		// for fixed height, we must adjust single-line text by the max ascent because fonts
		// are rendered according to their baseline. For multiline text, the first line
		// contributes max ascent only while the other lines are adjusted by font height.
		float const ma = AsLocal(font).GetFontInfo().MaxAscent;
		float const md = AsLocal(font).GetFontInfo().MaxDescent;
		float const fh = AsLocal(font).GetFontInfo().FontHeight;
		float const adjust = (ma - md) * 0.5f;
		float const vofs = (fh * (numLines - 1) * 0.5f) - adjust;
		curPos += u * (vofs * yScale);
		break;
	}
	case VERTICAL_TOP: {
		float const vofs = height - ascent;
		curPos += u * (vofs * scale);
		break;
	}
	}

    VVect3f basePos = curPos;
	switch (parms.AlignHoriz) {
	case HORIZONTAL_LEFT:
		break;

	case HORIZONTAL_CENTER: {
		curPos -= r * (lineWidths[0] * 0.5f * scale);
		break;
	}
	case HORIZONTAL_RIGHT: {
		curPos -= r * (lineWidths[0] * scale);
		break;
	}
	}

    VVect3f lineInc = u * (fontInfo.FontHeight * yScale);
	float const distanceScale = imageWidth / FontInfoType::DEFAULT_SCALE_FACTOR;
	const uint8_t fontParms[4] =
			{
                    (uint8_t) (VAlgorithm::Clamp(
							parms.AlphaCenter + fontInfo.CenterOffset, 0.0f,
                            1.0f) * 255), (uint8_t) (VAlgorithm::Clamp(
							parms.ColorCenter + fontInfo.CenterOffset, 0.0f,
                            1.0f) * 255), (uint8_t) (VAlgorithm::Clamp(
							distanceScale, 1.0f, 255.0f)), 0 };

	int iColor = ColorToABGR(color);

	int curLine = 0;
	fontVertex_t * v = vb.Verts;
    std::u32string ucs4Text = text.toUcs4();
    std::u32string::iterator p = ucs4Text.begin();
	size_t i = 0;
    uint32_t charCode = *p;
    for (; charCode != '\0'; i++, charCode = *(++p)) {
		vAssert( i < len);
		if (charCode == '\n' && curLine < numLines && curLine < MAX_LINES) {
			// move to next line
			curLine++;
			basePos -= lineInc;
			curPos = basePos;
			switch (parms.AlignHoriz) {
			case HORIZONTAL_LEFT:
				break;

			case HORIZONTAL_CENTER: {
				curPos -= r * (lineWidths[curLine] * 0.5f * scale);
				break;
			}
			case HORIZONTAL_RIGHT: {
				curPos -= r * (lineWidths[curLine] * scale);
				break;
			}
			}
		}

		FontGlyphType const & g = AsLocal(font).GlyphForCharCode(charCode);

		float s0 = g.X;
		float t0 = g.Y;
		float s1 = (g.X + g.Width);
		float t1 = (g.Y + g.Height);

		float bearingX = g.BearingX * xScale;
		float bearingY = g.BearingY * yScale;

		float rw = (g.Width + g.BearingX) * xScale;
		float rh = (g.Height - g.BearingY) * yScale;

		// lower left
		v[i * 4 + 0].xyz = curPos + (r * bearingX) - (u * rh);
		v[i * 4 + 0].s = s0;
		v[i * 4 + 0].t = t1;
		*(vuint32*) (&v[i * 4 + 0].rgba[0]) = iColor;
		*(vuint32*) (&v[i * 4 + 0].fontParms[0]) = *(vuint32*) (&fontParms[0]);
		// upper left
		v[i * 4 + 1].xyz = curPos + (r * bearingX) + (u * bearingY);
		v[i * 4 + 1].s = s0;
		v[i * 4 + 1].t = t0;
		*(vuint32*) (&v[i * 4 + 1].rgba[0]) = iColor;
		*(vuint32*) (&v[i * 4 + 1].fontParms[0]) = *(vuint32*) (&fontParms[0]);
		// upper right
		v[i * 4 + 2].xyz = curPos + (r * rw) + (u * bearingY);
		v[i * 4 + 2].s = s1;
		v[i * 4 + 2].t = t0;
		*(vuint32*) (&v[i * 4 + 2].rgba[0]) = iColor;
		*(vuint32*) (&v[i * 4 + 2].fontParms[0]) = *(vuint32*) (&fontParms[0]);
		// lower right
		v[i * 4 + 3].xyz = curPos + (r * rw) - (u * rh);
		v[i * 4 + 3].s = s1;
		v[i * 4 + 3].t = t1;
		*(vuint32*) (&v[i * 4 + 3].rgba[0]) = iColor;
		*(vuint32*) (&v[i * 4 + 3].fontParms[0]) = *(vuint32*) (&fontParms[0]);
		// advance to start of next char
		curPos += r * (g.AdvanceX * xScale);
	}
	// add the new vertex block to the array of vertex blocks
	VertexBlocks.append(vb);
}

//==============================
// BitmapFontSurfaceLocal::DrawText3Df
void BitmapFontSurfaceLocal::DrawText3Df(BitmapFont const & font,
        fontParms_t const & parms, VVect3f const & pos,
        VVect3f const & normal, VVect3f const & up, float const scale,
        VVect4f const & color, const char *fmt, ...) {
	char buffer[256];
	va_list args;
	va_start( args, fmt);
	vsnprintf(buffer, sizeof(buffer), fmt, args);
	va_end( args);
	DrawText3D(font, parms, pos, normal, up, scale, color, buffer);
}

//==============================
// BitmapFontSurfaceLocal::DrawTextBillboarded3D
void BitmapFontSurfaceLocal::DrawTextBillboarded3D(BitmapFont const & font,
        fontParms_t const & parms, VVect3f const & pos, float const scale,
        VVect4f const & color, const VString &text) {
	fontParms_t billboardParms = parms;
	billboardParms.Billboard = true;
    DrawText3D(font, billboardParms, pos, VVect3f(1.0f, 0.0f, 0.0f),
            VVect3f(0.0f, -1.0f, 0.0f), scale, color, text);
}

//==============================================================
// vbSort_t
// small structure that is used to sort vertex blocks by their distance to the camera
//==============================================================
struct vbSort_t {
	int VertexBlockIndex;
	float DistanceSquared;
};

//==============================
// VertexBlockSortFn
// sort function for vertex blocks
int VertexBlockSortFn(void const * a, void const * b) {
	return ftoi(
			((vbSort_t const*) a)->DistanceSquared
					- ((vbSort_t const*) b)->DistanceSquared);
}

//==============================
// BitmapFontSurfaceLocal::Finish
// transform all vertex blocks into the vertices array so they're ready to be uploaded to the VBO
// We don't have to do this for each eye because the billboarded surfaces are sorted / aligned
// based on their distance from / direction to the camera view position and not the camera direction.
void BitmapFontSurfaceLocal::Finish(VMatrix4f const & viewMatrix) {
    vAssert(this != NULL);

	//SPAM( "BitmapFontSurfaceLocal::Finish" );

    VMatrix4f invViewMatrix = viewMatrix.inverted(); // if the view is never scaled or sheared we could use Transposed() here instead
    VVect3f viewPos = invViewMatrix.translation();

	// sort vertex blocks indices based on distance to pivot
	int const MAX_VERTEX_BLOCKS = 256;
	vbSort_t vbSort[MAX_VERTEX_BLOCKS];
	int const n = VertexBlocks.length();
	for (int i = 0; i < n; ++i) {
		vbSort[i].VertexBlockIndex = i;
		VertexBlockType & vb = VertexBlocks[i];
		vbSort[i].DistanceSquared = (vb.Pivot - viewPos).lengthSquared();
	}

	qsort(vbSort, n, sizeof(vbSort[0]), VertexBlockSortFn);

	// transform the vertex blocks into the vertices array
	CurIndex = 0;
	CurVertex = 0;

	// TODO:
	// To add multiple-font-per-surface support, we need to add a 3rd component to s and t,
	// then get the font for each vertex block, and set the texture index on each vertex in
	// the third texture coordinate.
	for (int i = 0; i < VertexBlocks.length(); ++i) {
		VertexBlockType & vb = VertexBlocks[vbSort[i].VertexBlockIndex];
        VMatrix4f transform;
		if (vb.Billboard) {
			if (vb.TrackRoll) {
				transform = invViewMatrix;
			} else {
                VVect3f textNormal = viewPos - vb.Pivot;
				float const len = textNormal.length();
                if (len < VConstantsf::SmallestNonDenormal) {
					vb.Free();
					continue;
				}
				textNormal *= 1.0f / len;
                transform = VMatrix4f::CreateFromBasisVectors(textNormal,
                        VVect3f(0.0f, 1.0f, 0.0f));
			}
			transform.setTranslation(vb.Pivot);
        } else {
			transform.setTranslation(vb.Pivot);
		}

		for (int j = 0; j < vb.NumVerts; j++) {
			fontVertex_t const & v = vb.Verts[j];
            Vertices[CurVertex].xyz = transform.transform(v.xyz);
			Vertices[CurVertex].s = v.s;
			Vertices[CurVertex].t = v.t;
			*(vuint32*) (&Vertices[CurVertex].rgba[0]) = *(vuint32*) (&v.rgba[0]);
			*(vuint32*) (&Vertices[CurVertex].fontParms[0]) =
					*(vuint32*) (&v.fontParms[0]);
			CurVertex++;
		}
		CurIndex += (vb.NumVerts / 2) * 3;
		// free this vertex block
		vb.Free();
	}
	// remove all elements from the vertex block (but don't free the memory since it's likely to be
	// needed on the next frame.
	VertexBlocks.clear();


    VEglDriver::glBindVertexArrayOES(Geo.vertexArrayObject);
	glBindBuffer(GL_ARRAY_BUFFER, Geo.vertexBuffer);
	glBufferSubData(GL_ARRAY_BUFFER, 0, CurVertex * sizeof(fontVertex_t),
			(void *) Vertices);
    VEglDriver::glBindVertexArrayOES(0);

	Geo.indexCount = CurIndex;
}

//==============================
// BitmapFontSurfaceLocal::Render3D
// render the font surface by transforming each vertex block and copying it into the VBO
// TODO: once we add support for multiple fonts per surface, this should not take a BitmapFont for input.
void BitmapFontSurfaceLocal::Render3D(BitmapFont const & font,const int eye) const {

	VEglDriver::logErrorsEnum("BitmapFontSurfaceLocal::Render3D - pre");

	//SPAM( "BitmapFontSurfaceLocal::Render3D" );

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glDepthMask(GL_FALSE);
	glDisable(GL_CULL_FACE);

	// Draw the text glyphs
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, AsLocal(font).GetTexture());

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	glUseProgram(AsLocal(font).GetFontProgram().program);

	if (vApp->eyeSettings().useMultiview)
	{
		VMatrix4f modelViewProMatrix[2];
		modelViewProMatrix[0] = vApp->getModelViewProMatrix(0).transposed();
		modelViewProMatrix[1] = vApp->getModelViewProMatrix(1).transposed();
		glUniformMatrix4fv(AsLocal(font).GetFontProgram().uniformModelViewProMatrix, 2, GL_FALSE,
						   modelViewProMatrix[0].data());
	}
	else
	{
		glUniformMatrix4fv(AsLocal(font).GetFontProgram().uniformModelViewProMatrix, 1, GL_FALSE,
						   vApp->getModelViewProMatrix(eye).transposed().data());
	}

	float textColor[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
	glUniform4fv(AsLocal(font).GetFontProgram().uniformColor, 1, textColor);

	// draw all font vertices
    VEglDriver::glBindVertexArrayOES(Geo.vertexArrayObject);
	glDrawElements(GL_TRIANGLES, Geo.indexCount, GL_UNSIGNED_SHORT, NULL);
    VEglDriver::glBindVertexArrayOES(0);

	glEnable(GL_CULL_FACE);

	glDisable(GL_BLEND);
	glDepthMask(GL_FALSE);

    VEglDriver::logErrorsEnum("BitmapFontSurfaceLocal::Render3D - post");
}

//==============================
// BitmapFont::Create
BitmapFont * BitmapFont::Create() {
	return new BitmapFontLocal;
}
//==============================
// BitmapFont::Free
void BitmapFont::Free(BitmapFont * & font) {
	if (font != NULL) {
		delete font;
		font = NULL;
	}
}

//==============================
// BitmapFontSurface::Create
BitmapFontSurface * BitmapFontSurface::Create() {
	return new BitmapFontSurfaceLocal();
}

//==============================
// BitmapFontSurface::Free
void BitmapFontSurface::Free(BitmapFontSurface * & fontSurface) {
	if (fontSurface != NULL) {
		delete fontSurface;
		fontSurface = NULL;
	}
}

NV_NAMESPACE_END
