#include "VResource.h"
#include "VGuiText.h"
#include "VPainter.h"
#include "VGlGeometry.h"
#include "VGlShader.h"

#include <ft2build.h>
#include FT_FREETYPE_H

NV_NAMESPACE_BEGIN

static const char *VertexShaderSource =
    "uniform highp mat4 Mvpm;\n"
    "attribute vec4 Position;\n"
    "attribute vec2 TexCoord;\n"
    "varying highp vec2 oTexCoord;\n"
    "void main()\n"
    "{\n"
    "    gl_Position = Mvpm * Position;\n"
    "    oTexCoord = TexCoord;\n"
    "}\n";

static const char *FragmentShaderSource =
    "uniform sampler2D Texture0;\n"
    "varying highp vec2 oTexCoord;\n"
    "uniform lowp vec4 UniformColor;\n"
    "void main()\n"
    "{\n"
    "    vec4 sampled = vec4(1.0, 1.0, 1.0, texture2D(Texture0, oTexCoord).r);\n"
    "    gl_FragColor = UniformColor * sampled;\n"
    "}\n";

struct VGuiText::Private
{
    VGlShader shader;
    VGlGeometry geometry;
    VColor color;
    VString text;
    float scale;

    FT_Library ft;
    FT_Face face;

    Private()
    :scale(1.0f)
    {
        shader.initShader(VertexShaderSource, FragmentShaderSource);
        geometry.createPlaneQuadGrid(1, 1);
    }

    VTexture generateCharTex(char c)
    {
        VTexture texture;
        if (FT_Load_Char(face, c, FT_LOAD_RENDER)) {
            return texture;
        }
        FT_GlyphSlot glyph = face->glyph;
        texture.loadRed(glyph->bitmap.buffer, glyph->bitmap.width, glyph->bitmap.rows);
        return texture;
    }
};

VGuiText::VGuiText(VGraphicsItem* parent)
    : VGraphicsItem(parent)
    ,d(new Private())
{
    setBoundingRect(VRect3f(-0.2f, -0.2f, -3.0f, 0.2f, 0.2f, -3.0f));
}


VGuiText::~VGuiText()
{
    // Destroy FreeType once we're finished
    FT_Done_Face(d->face);
    FT_Done_FreeType(d->ft);
    delete d;
}

VColor VGuiText::textColor() const
{
    return d->color;
}

void VGuiText::setTextColor(const VColor &color)
{
    d->color = color;
}

VString VGuiText::textValue() const
{
    return d->text;
}

void VGuiText::setTextValue(const VString &text)
{
    d->text = text;
}

void VGuiText::paint(VPainter *painter) {
    if (FT_Init_FreeType(&(d->ft))) {
        return;
    }

    VResource re("res/raw/times.ttf");
    if (FT_New_Memory_Face(d->ft, reinterpret_cast<const FT_Byte *>(re.data().data()), re.size(), 0,
                           &d->face)) {
        return;
    }
    // Set size to load glyphs as
    if (FT_Set_Pixel_Sizes(d->face, 0, 50)) {
        return;
    }


    // Disable byte-alignment restriction
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    glUseProgram(d->shader.program);

    VMatrix4f offset = VMatrix4f::Translation(0.0f, 0.0f, 0.0f);
    const VMatrix4f &mvp = painter->viewMatrix();
    const VMatrix4f &rect = transform();

    for (char16_t c: d->text) {
        VTexture texture = d->generateCharTex(c);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texture.id());

        VMatrix4f screenMvp = mvp * offset * rect;
        offset = VMatrix4f::Translation(0.5f, 0.0f, 0.0f) * offset;

        glUniformMatrix4fv(d->shader.uniformModelViewProMatrix, 1, GL_FALSE, screenMvp.transposed().data());
        glUniform4f(d->shader.uniformColor, d->color.red / 255.0f, d->color.green / 255.0f,
                    d->color.blue / 255.0f, d->color.alpha / 255.0f);
        d->geometry.drawElements();
    }
}
NV_NAMESPACE_END

