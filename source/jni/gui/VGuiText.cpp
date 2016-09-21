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
                "void main()\n"
                "{\n"
                "    gl_FragColor = texture2D(Texture0, oTexCoord);\n"
                "}\n";

struct VGuiText::Private
{
    VGlShader shader;
    VGlGeometry geometry;
    VColor color;
    VString text;
    VVect2f pos;
    float scale;

    FT_Library ft;
    FT_Face face;
    VertexAttribs vertexs;
    VArray< ushort > indices;

    Private()
    {
        shader.initShader(VertexShaderSource, FragmentShaderSource);

        for (int i = 0; i < 4; ++i) {
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
        indices.push_back(3);


        FT_Init_FreeType(&ft);
        FT_New_Face(ft, "res/raw/verdana.ttf", 0, &face);
        geometry.createGlGeometry(vertexs, indices);
    }

};

VGuiText::VGuiText(VGraphicsItem* parent)
    : VGraphicsItem(parent)
    ,d(new Private())
{
    // Set size to load glyphs as
    FT_Set_Pixel_Sizes(d->face, 0, 48);

    // Disable byte-alignment restriction
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
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
    FT_Load_Char(d->face, c, FT_LOAD_RENDER);

    GLuint texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexImage2D(
            GL_TEXTURE_2D,
            0,
            GL_RED,
            d->face->glyph->bitmap.width,
            d->face->glyph->bitmap.rows,
            0,
            GL_RED,
            GL_UNSIGNED_BYTE,
            d->face->glyph->bitmap.buffer
    );

    // Set texture options
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    return texture;
}

void VGuiText::updateVertexAttribs(float xpos, float ypos, float w, float h)
{
    d->vertexs.position.clear();

    d->vertexs.position.push_back(VVect3f(xpos, ypos + h, 0.0));
    d->vertexs.position.push_back(VVect3f(xpos, ypos, 0.0));
    d->vertexs.position.push_back(VVect3f( xpos + w, ypos, 0.0));

    d->vertexs.position.push_back(VVect3f(xpos, ypos + h, 0.0));
    d->vertexs.position.push_back(VVect3f(xpos + w, ypos, 0.0));
    d->vertexs.position.push_back(VVect3f(xpos + w, ypos + h, 0.0));

}

void VGuiText::paint(VPainter *painter)
{
    VGraphicsItem::paint(painter);
    int x = d->pos.x;
    int y = d->pos.y;
    VEglDriver::glPushAttrib();
    glEnable( GL_BLEND );
    glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glUseProgram(d->shader.program);
    const VMatrix4f screenMvp = transform();
    glUniformMatrix4fv(d->shader.uniformModelViewProMatrix, 1, GL_FALSE, screenMvp.transposed().data());

    for (auto c: d->text) {
        uint texId = generateCharTex(c);



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

        updateVertexAttribs(xpos, ypos, w, h);
        d->geometry.updateGlGeometry(d->vertexs);
        d->geometry.textureId = texId;
        d->geometry.drawElements();
    }

    VEglDriver::glPopAttrib();
}
NV_NAMESPACE_END

