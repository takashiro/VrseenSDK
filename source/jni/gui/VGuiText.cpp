#include <io/VResource.h>
#include "VGuiText.h"
#include "VPainter.h"
#include "VGlGeometry.h"
#include "VGlShader.h"


#include "ft2build.h"
#include FT_FREETYPE_H

NV_NAMESPACE_BEGIN

static const char* VertexShaderSource =
    "uniform highp mat4 Mvpm;\n"
    "attribute vec4 Position;\n"
    "attribute vec2 TexCoord;\n"
    "varying highp vec2 oTexCoord;\n"
    "void main()\n"
    "{\n"
    "    gl_Position = Mvpm * Position;\n"
    "    oTexCoord = TexCoord;\n"
    "}\n";

static const char * FragmentShaderSource =
    "uniform sampler2D Texture0;\n"
    "varying highp vec2 oTexCoord;\n"
    "uniform lowp vec4 UniformColor;\n"
    "void main()\n"
    "{\n"
    "    gl_FragColor = vec4(1.0, 1.0, 1.0, texture2D(Texture0, oTexCoord).r + 0.5);\n"
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
    VertexAttribs vertexs;
    VArray< ushort > indices;

    Private()
    :scale(1.0f)
    {
        shader.initShader(VertexShaderSource, FragmentShaderSource);

        /*for (int i = 0; i < 4; ++i) {
            VVect3f pos(0, 0, 0);
            VVect4f col(0, 0, 0, 0);
            vertexs.position.push_back(pos);
            vertexs.color.push_back(col);
        }

        vertexs.uvCoordinate0.push_back(VVect2f(0, 0));
        vertexs.uvCoordinate0.push_back(VVect2f(0, 1));
        vertexs.uvCoordinate0.push_back(VVect2f(1, 1));
        vertexs.uvCoordinate0.push_back(VVect2f(1, 0));

        indices.push_back(0);
        indices.push_back(1);
        indices.push_back(2);
        indices.push_back(0);
        indices.push_back(2);
        indices.push_back(3);*/

        geometry.createPlaneQuadGrid(1, 1);
    }

};

VGuiText::VGuiText(VGraphicsItem* parent)
    : VGraphicsItem(parent)
    ,d(new Private())
{
//    // Set size to load glyphs as
//    FT_Set_Pixel_Sizes(d->face, 0, 48);
//    // Disable byte-alignment restriction
//    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    setBoundingRect(VRect3f(-1.0f, -1.0f, -3.0f, 1.0f, 1.0f, 3.0f));
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


uint VGuiText::generateCharTex(char c)
{
    if (FT_Load_Char(d->face, c, FT_LOAD_RENDER)) {
        return -1;
    }

    FT_GlyphSlot glyph = d->face->glyph;
    VTexture texture;
    texture.loadRed(glyph->bitmap.buffer, glyph->bitmap.width, glyph->bitmap.rows);
    return texture.id();
}

void VGuiText::updateVertexAttribs(float xpos, float ypos, float w, float h)
{
    d->vertexs.position.clear();

    d->vertexs.position.push_back(VVect3f(xpos, ypos + h, -3.0));
    d->vertexs.position.push_back(VVect3f(xpos, ypos, -3.0));
    d->vertexs.position.push_back(VVect3f( xpos + w, ypos, -3.0));
    d->vertexs.position.push_back(VVect3f(xpos + w, ypos + h, -3.0));

}

void VGuiText::paint(VPainter *painter)
{
    if (FT_Init_FreeType(&(d->ft))) {
        return;
    }

    VResource re("res/raw/times.ttf");
    if (FT_New_Memory_Face(d->ft, reinterpret_cast<const FT_Byte*>(re.data().data()), re.size(), 0, &d->face)){
        return ;
    }
    // Set size to load glyphs as
    if (FT_Set_Pixel_Sizes(d->face, 0, 48)) {
        return ;
    }


    // Disable byte-alignment restriction
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

//    float x = 100;
//    float y = 100;
    uint texture = generateCharTex('a');
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture);

    glUseProgram(d->shader.program);
    const VMatrix4f screenMvp = painter->viewMatrix() * transform();
    glUniformMatrix4fv(d->shader.uniformModelViewProMatrix, 1, GL_FALSE, screenMvp.transposed().data());
    glUniform4f(d->shader.uniformColor, d->color.red / 255.0f, d->color.green / 255.0f, d->color.blue / 255.0f, d->color.alpha / 255.0f);
    d->geometry.drawElements();

    /*GLuint texId;
    for (auto c: d->text) {
        texId = generateCharTex(c);



//        Character character = {
//               Texture: texture,
//               Size:  glm::ivec2(face->glyph->bitmap.width, face->glyph->bitmap.rows),
//               Bearing: glm::ivec2(face->glyph->bitmap_left, face->glyph->bitmap_top),
//               Advance: face->glyph->advance.x
//        };

        float xpos = x + d->face->glyph->bitmap_left * d->scale;
        float ypos = y - (d->face->glyph->bitmap.rows -  d->face->glyph->bitmap_top) * d->scale;
        float w = d->face->glyph->bitmap.width * d->scale;
        float h = d->face->glyph->bitmap.rows * d->scale;

        //updateVertexAttribs(xpos, ypos, w, h);
        //d->geometry.updateGlGeometry(d->vertexs);
        d->geometry.textureId = texId;
        d->geometry.drawElements();
    }*/
}
NV_NAMESPACE_END

