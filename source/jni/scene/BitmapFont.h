#pragma once

#include "vglobal.h"

#include "VMath.h"
#include "VString.h"
#include "Array.h"

NV_NAMESPACE_BEGIN

class BitmapFont;
class BitmapFontSurface;

enum HorizontalJustification {
	HORIZONTAL_LEFT, HORIZONTAL_CENTER, HORIZONTAL_RIGHT
};

enum VerticalJustification {
	VERTICAL_BASELINE, // align text by baseline of first row
	VERTICAL_CENTER,
	VERTICAL_CENTER_FIXEDHEIGHT, // ignores ascenders/descenders
	VERTICAL_TOP
};

// To get a black outline on fonts, AlphaCenter should be < ( ColorCenter = 0.5 )
// To get non-outlined fonts, ColorCenter should be < ( AlphaCenter = 0.5 )
struct fontParms_t {
	fontParms_t() :
			AlignHoriz(HORIZONTAL_LEFT), AlignVert(VERTICAL_BASELINE), Billboard(
					false), TrackRoll(false), AlphaCenter(0.425f), ColorCenter(
					0.50f) {
	}

	HorizontalJustification AlignHoriz; // horizontal justification around the specified x coordinate
	VerticalJustification AlignVert; // vertical justification around the specified y coordinate
	bool Billboard; // true to always face the camera
	bool TrackRoll; // when billboarding, track with camera roll
	float AlphaCenter; // below this distance, alpha is 0, above this alpha is 1
	float ColorCenter; // blow this distance, color is 0, above this color is 1
};

//==============================================================
// BitmapFont
class BitmapFont {
public:
	static BitmapFont * Create();
	static void Free(BitmapFont * & font);

    virtual bool Load(const VString &languagePackageFileName,
            const VString & fontInfoFileName) = 0;
	// Calculates the native (unscaled) width of the text string. Line endings are ignored.
    virtual float CalcTextWidth(const VString &text) const = 0;
	// Calculates the native (unscaled) width of the text string. Each '\n' will start a new line
	// and will increase the height by FontInfo.FontHeight. For multi-line strings, lineWidths will
	// contain the width of each individual line of text and width will be the width of the widest
	// line of text.
    virtual void CalcTextMetrics(const VString &text, size_t & len, float & width,
			float & height, float & ascent, float & descent, float & fontHeight,
			float * lineWidths, int const maxLines, int & numLines) const = 0;

	// Word wraps passed in text based on the passed in width in meters.
	// Turns any pre-existing escape characters into spaces.
	virtual void WordWrapText(VString & inOutText, const float widthMeters,
			const float fontScale = 1.0f) const = 0;
	// Another version of WordWrapText which doesn't break in between strings that are listed in wholeStrsList array
	// Ex : "Gear VR", we don't want to break in between "Gear" & "VR" so we need to pass "Gear VR" string in wholeStrsList
	virtual void WordWrapText(VString & inOutText, const float widthMeters,
            Array<VString> wholeStrsList,
			const float fontScale = 1.0f) const = 0;

protected:
	virtual ~BitmapFont() {
	}
};

//==============================================================
// BitmapFontSurface
class BitmapFontSurface {
public:
	static BitmapFontSurface * Create();
	static void Free(BitmapFontSurface * & fontSurface);

	virtual void Init(const int maxVertices) = 0;
	virtual void DrawText3D(BitmapFont const & font, const fontParms_t & flags,
			const Vector3f & pos, Vector3f const & normal, Vector3f const & up,
            float const scale, Vector4f const & color, const VString &text) = 0;
	virtual void DrawText3Df(BitmapFont const & font, const fontParms_t & flags,
			const Vector3f & pos, Vector3f const & normal, Vector3f const & up,
            float const scale, Vector4f const & color, const char *format,
			...) = 0;

	virtual void DrawTextBillboarded3D(BitmapFont const & font,
			fontParms_t const & flags, Vector3f const & pos, float const scale,
			Vector4f const & color, char const * text) = 0;
	virtual void DrawTextBillboarded3Df(BitmapFont const & font,
			fontParms_t const & flags, Vector3f const & pos, float const scale,
			Vector4f const & color, char const * fmt, ...) = 0;

	virtual void Finish(Matrix4f const & viewMatrix) = 0;

	virtual void Render3D(BitmapFont const & font,
			Matrix4f const & worldMVP) const = 0;

protected:
	virtual ~BitmapFontSurface() {
	}
};

NV_NAMESPACE_END
