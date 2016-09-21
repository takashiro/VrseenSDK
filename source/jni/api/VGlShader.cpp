#include "VGlShader.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <scene/VEyeItem.h>

#include "api/VEglDriver.h"
#include "../core/VLog.h"
#define STRINGIZE( x )			#x
NV_NAMESPACE_BEGIN




const char * VGlShader::getFadedPanoVertexShaderSource()
{
const char * fadedPanoVertexShaderSource =
"uniform highp mat4 Mvpm;\n"
"uniform highp mat4 Texm;\n"
"attribute vec4 Position;\n"
"attribute vec2 TexCoord;\n"
"varying  highp vec2 oTexCoord;\n"
"void main()\n"
"{\n"
"   gl_Position = Mvpm * Position;\n"
"   oTexCoord = vec2( Texm * vec4( TexCoord, 0, 1 ) );\n"
"}\n";
    return fadedPanoVertexShaderSource;
    }
const char * VGlShader::getFadedPanoProgramShaderSource()
{
    const char * fadedPanoProgramShaderSource =
"#extension GL_OES_EGL_image_external : require\n"
"uniform samplerExternalOES Texture0;\n"
"uniform sampler2D Texture1;\n"
"uniform lowp vec4 UniformColor;\n"
"varying highp vec2 oTexCoord;\n"
"void main()\n"
"{\n"
"	lowp vec4 staticColor = texture2D( Texture1, oTexCoord );\n"
"	lowp vec4 movieColor = texture2D( Texture0, oTexCoord );\n"
"	gl_FragColor = UniformColor * mix( movieColor, staticColor, staticColor.w );\n"
"}\n";
return fadedPanoProgramShaderSource;

}






const char * VGlShader::getUniformSingleTextureProgramShaderSource()
{
    const char * uniformSingleTextureProgramShaderSource=
    "uniform sampler2D Texture0;\n"
    "uniform lowp vec4 UniformColor;\n"
    "varying highp vec2 oTexCoord;\n"
    "void main()\n"
    "{\n"
    "   gl_FragColor = UniformColor * texture2D( Texture0, oTexCoord );\n"
    "}\n";

   return uniformSingleTextureProgramShaderSource;
}
const char * VGlShader::getUniformTextureProgramShaderSource()
{
    const char * uniformTextureProgramShaderSource=
            "uniform sampler2D Texture0;\n"
            "varying highp vec2 oTexCoord;\n"
            "varying lowp vec4	oColor;\n"
            "void main()\n"
            "{\n"
            "	gl_FragColor = oColor * texture( Texture0, oTexCoord );\n"
            "}\n";

   return uniformTextureProgramShaderSource;
}
const char * VGlShader::getPanoProgramShaderSource()
{
             const char * panoProgramSource =
                     "#extension GL_OES_EGL_image_external : enable\n"
                     "uniform samplerExternalOES Texture0;\n"
                     "uniform lowp vec4 UniformColor;\n"
                     "uniform lowp vec4 ColorBias;\n"
                     "varying highp vec2 oTexCoord;\n"
                     "void main()\n"
                     "{\n"
                     "	gl_FragColor = ColorBias + UniformColor * texture2D( Texture0, oTexCoord );\n"
                     "}\n";
   return panoProgramSource;
}





const char * VGlShader::getPanoVertexShaderSource()
{
             const char * panoVertexShaderSource =
                     "uniform highp mat4 Mvpm[NUM_VIEWS];\n"
                     "uniform highp mat4 Texm[NUM_VIEWS];\n"
                     "attribute vec4 Position;\n"
                     "attribute vec2 TexCoord;\n"
                     "varying  highp vec2 oTexCoord;\n"
                     "void main()\n"
                     "{\n"
                     "   gl_Position = Mvpm[VIEW_ID] * Position;\n"
                     "   oTexCoord = vec2( Texm[VIEW_ID] * vec4( TexCoord, 0, 1 ) );\n"
                     "}\n";
   return panoVertexShaderSource;
}



const char * VGlShader::getCubeMapPanoVertexShaderSource()
{
             const char * cubeMapPanoVertexSource =
"uniform mat4 Mvpm[NUM_VIEWS];\n"
"attribute vec4 Position;\n"
"uniform mediump vec4 UniformColor;\n"
"varying  lowp vec4 oColor;\n"
"varying highp vec3 oTexCoord;\n"
"void main()\n"
"{\n"
"   gl_Position = Mvpm[VIEW_ID] * Position;\n"
"	oTexCoord = Position.xyz;\n"
"   oColor = UniformColor;\n"
"}\n";
  return cubeMapPanoVertexSource;
}




const char * VGlShader::getCubeMapPanoProgramShaderSource()
{
             const char * cubeMapPanoProgramSource =
            "uniform samplerCube Texture0;\n"
            "varying highp vec3 oTexCoord;\n"
            "varying lowp vec4	oColor;\n"
            "void main()\n"
            "{\n"
            "	gl_FragColor = oColor * texture( Texture0, oTexCoord );\n"
            "}\n";
   return cubeMapPanoProgramSource;
}
const char * VGlShader::getTexturedMvpVertexShaderSource()
{
    const char * texturedMvpVertexShaderSource =
        "uniform mat4 Mvpm[NUM_VIEWS];\n"
        "uniform highp mat4 Texm[NUM_VIEWS];\n"
        "attribute vec4 Position;\n"
        "attribute vec4 VertexColor;\n"
        "attribute vec2 TexCoord;\n"
        "uniform mediump vec4 UniformColor;\n"
        "varying  lowp vec4 oColor;\n"
        "varying highp vec2 oTexCoord;\n"
        "void main()\n"
        "{\n"
        "   gl_Position = Mvpm[VIEW_ID] * Position;\n"
        "	oTexCoord = vec2( Texm[VIEW_ID] * vec4(TexCoord,1,1) );\n"
        "   oColor = /* VertexColor * */ UniformColor;\n"
        "}\n";
    return texturedMvpVertexShaderSource;
}
const char * VGlShader::getHighlightColorVertexShaderSource()
{
  const char * highlightColorVertexShaderSource =
"uniform mat4 Mvpm;\n"
"uniform vec4 UniformColor;\n"
"attribute vec4 Position;\n"
"attribute vec2 TexCoord;\n"
"varying  highp vec2 oTexCoord;\n"
"varying  lowp vec4 oColor;\n"
"void main()\n"
"{\n"
"   gl_Position = Mvpm * Position;\n"
"   oTexCoord = TexCoord;\n"
"	oColor = UniformColor;\n"
"}\n";
  return highlightColorVertexShaderSource;
}
const char * VGlShader::getHighlightPragmentShaderSource()
{

  const char *  HighlightPragmentShaderSource =
"uniform sampler2D Texture1;\n"
"varying highp vec2 oTexCoord;\n"
"varying lowp vec4 oColor;\n"
"void main()\n"
"{\n"
"	gl_FragColor = oColor;\n"
"	gl_FragColor.w = texture2D(Texture1, oTexCoord).x;\n"
"}\n";
  return HighlightPragmentShaderSource;

}
const char * VGlShader::getDoubletextureTransparentColorVertexShaderSource()
{
const char* getDoubletextureTransparentColorVertexShaderSource =
"uniform sampler2D Texture0;\n"
"uniform sampler2D Texture1;\n"
"varying highp vec2 oTexCoord;\n"
"void main()\n"
"{\n"
"	gl_FragColor.xyz = texture2D(Texture0, oTexCoord).xyz;\n"
"	gl_FragColor.w = texture2D(Texture1, oTexCoord).x;\n"
"}\n";
   return getDoubletextureTransparentColorVertexShaderSource;

}
const char * VGlShader::getUntextureInverseColorVertexShaderSource()
{
 const char * untextureInverseColorVertexShaderSource =
"uniform mat4 Mvpm[NUM_VIEWS];\n"
"attribute vec4 VertexColor;\n"
"attribute vec4 Position;\n"
"varying  lowp vec4 oColor;\n"
"void main()\n"
"{\n"
"   gl_Position = Mvpm[VIEW_ID] * Position;\n"
"   oColor = vec4(1.0, 1.0, 1.0, 1.0 - VertexColor.x);\n"
"}\n";

 return untextureInverseColorVertexShaderSource;

}

const char * VGlShader::getUntextureMvpVertexShaderSource()
{

 const char * untextureMvpVertexShaderSource =
    "uniform mat4 Mvpm;\n"
"attribute vec4 Position;\n"
"uniform mediump vec4 UniformColor;\n"
"varying  lowp vec4 oColor;\n"
"void main()\n"
"{\n"
    "   gl_Position = Mvpm * Position;\n"
    "   oColor = UniformColor;\n"
"}\n";
 return untextureMvpVertexShaderSource;

}
const char * VGlShader::getSingleTextureVertexShaderSource()

{
    const char * singleTextureVertexShaderSrc =
            "uniform highp mat4 Mvpm;\n"
            "attribute highp vec4 Position;\n"
            "attribute highp vec2 TexCoord;\n"
            "varying highp vec2 oTexCoord;\n"
            "void main()\n"
            "{\n"
            "   gl_Position = Mvpm * Position;\n"
            "   oTexCoord = TexCoord;\n"
            "}\n";

  return singleTextureVertexShaderSrc;

}
const char * VGlShader::getSingleTextureSkVertexShaderSource()
{

    const char * singleTextureSkinned1VertexShaderSrc =
            "uniform highp mat4 Mvpm;\n"
            "uniform highp mat4 Joints[" STRINGIZE( 16 ) "];\n"
            "attribute highp vec4 Position;\n"
            "attribute highp vec2 TexCoord;\n"
            "attribute highp vec4 JointWeights;\n"
            "attribute highp vec4 JointIndices;\n"
            "varying highp vec2 oTexCoord;\n"
            "void main()\n"
            "{\n"
            "   gl_Position = Mvpm * ( Joints[int(JointIndices.x)] * Position );\n"
            "   oTexCoord = TexCoord;\n"
            "}\n";
  return  singleTextureSkinned1VertexShaderSrc;

}
 const char * VGlShader::getSingleTextureFragmentShaderSource()
{
    const char * singleTextureFragmentShaderSrc =
            "uniform sampler2D Texture0;\n"
            "varying highp vec2 oTexCoord;\n"
            "void main()\n"
            "{\n"
            "   gl_FragColor = texture2D( Texture0, oTexCoord );\n"
            "}\n";
    return singleTextureFragmentShaderSrc;


}

 const char * VGlShader::getLightMappedVertexShaderSource()
{
    const char * lightMappedVertexShaderSrc =
            "uniform highp mat4 Mvpm;\n"
            "attribute highp vec4 Position;\n"
            "attribute highp vec2 TexCoord;\n"
            "attribute highp vec2 TexCoord1;\n"
            "varying highp vec2 oTexCoord;\n"
            "varying highp vec2 oTexCoord1;\n"
            "void main()\n"
            "{\n"
            "   gl_Position = Mvpm * Position;\n"
            "   oTexCoord = TexCoord;\n"
            "   oTexCoord1 = TexCoord1;\n"
            "}\n";
    return lightMappedVertexShaderSrc;


}
 const char * VGlShader::getLightMappedSkVertexShaderSource()
{
    const char * lightMappedSkinned1VertexShaderSrc =
            "uniform highp mat4 Mvpm;\n"
            "uniform highp mat4 Joints[" STRINGIZE( 16 ) "];\n"
            "attribute highp vec4 Position;\n"
            "attribute highp vec2 TexCoord;\n"
            "attribute highp vec2 TexCoord1;\n"
            "attribute highp vec4 JointWeights;\n"
            "attribute highp vec4 JointIndices;\n"
            "varying highp vec2 oTexCoord;\n"
            "varying highp vec2 oTexCoord1;\n"
            "void main()\n"
            "{\n"
            "   gl_Position = Mvpm * ( Joints[int(JointIndices.x)] * Position );\n"
            "   oTexCoord = TexCoord;\n"
            "   oTexCoord1 = TexCoord1;\n"
            "}\n";
    return lightMappedSkinned1VertexShaderSrc;


}
 const char * VGlShader::getLightMappedFragmentShaderSource()
{
    const char * lightMappedFragmentShaderSrc =
            "uniform sampler2D Texture0;\n"
            "uniform sampler2D Texture1;\n"
            "varying highp vec2 oTexCoord;\n"
            "varying highp vec2 oTexCoord1;\n"
            "void main()\n"
            "{\n"
            "   lowp vec4 diffuse = texture2D( Texture0, oTexCoord );\n"
            "   lowp vec4 emissive = texture2D( Texture1, oTexCoord1 );\n"
            "   gl_FragColor.xyz = diffuse.xyz * emissive.xyz * 1.5;\n"
            "   gl_FragColor.w = diffuse.w;\n"
            "}\n";
    return lightMappedFragmentShaderSrc;

}


const char * VGlShader::getReflectionMappedVertexShaderSource()
{
    const char * reflectionMappedVertexShaderSrc =
            "uniform highp mat4 Mvpm;\n"
            "uniform highp mat4 Modelm;\n"
            "uniform highp mat4 Viewm;\n"
            "attribute highp vec4 Position;\n"
            "attribute highp vec3 Normal;\n"
            "attribute highp vec3 Tangent;\n"
            "attribute highp vec3 Binormal;\n"
            "attribute highp vec2 TexCoord;\n"
            "attribute highp vec2 TexCoord1;\n"
            "varying highp vec3 oEye;\n"
            "varying highp vec3 oNormal;\n"
            "varying highp vec3 oTangent;\n"
            "varying highp vec3 oBinormal;\n"
            "varying highp vec2 oTexCoord;\n"
            "varying highp vec2 oTexCoord1;\n"
            "vec3 multiply( mat4 m, vec3 v )\n"
            "{\n"
            "   return vec3(\n"
            "      m[0].x * v.x + m[1].x * v.y + m[2].x * v.z,\n"
            "      m[0].y * v.x + m[1].y * v.y + m[2].y * v.z,\n"
            "      m[0].z * v.x + m[1].z * v.y + m[2].z * v.z );\n"
            "}\n"
            "vec3 transposeMultiply( mat4 m, vec3 v )\n"
            "{\n"
            "   return vec3(\n"
            "      m[0].x * v.x + m[0].y * v.y + m[0].z * v.z,\n"
            "      m[1].x * v.x + m[1].y * v.y + m[1].z * v.z,\n"
            "      m[2].x * v.x + m[2].y * v.y + m[2].z * v.z );\n"
            "}\n"
            "void main()\n"
            "{\n"
            "   gl_Position = Mvpm * Position;\n"
            "   vec3 eye = transposeMultiply( Viewm, -vec3( Viewm[3] ) );\n"
            "   oEye = eye - vec3( Modelm * Position );\n"
            "   oNormal = multiply( Modelm, Normal );\n"
            "   oTangent = multiply( Modelm, Tangent );\n"
            "   oBinormal = multiply( Modelm, Binormal );\n"
            "   oTexCoord = TexCoord;\n"
            "   oTexCoord1 = TexCoord1;\n"
            "}\n";

    return reflectionMappedVertexShaderSrc;


}
const char * VGlShader::getReflectionMappedSkVertexShaderSource()
{

    const char * reflectionMappedSkinned1VertexShaderSrc =
            "uniform highp mat4 Mvpm;\n"
            "uniform highp mat4 Modelm;\n"
            "uniform highp mat4 Viewm;\n"
            "uniform highp mat4 Joints[" STRINGIZE( 16 ) "];\n"
            "attribute highp vec4 Position;\n"
            "attribute highp vec3 Normal;\n"
            "attribute highp vec3 Tangent;\n"
            "attribute highp vec3 Binormal;\n"
            "attribute highp vec2 TexCoord;\n"
            "attribute highp vec2 TexCoord1;\n"
            "attribute highp vec4 JointWeights;\n"
            "attribute highp vec4 JointIndices;\n"
            "varying highp vec3 oEye;\n"
            "varying highp vec3 oNormal;\n"
            "varying highp vec3 oTangent;\n"
            "varying highp vec3 oBinormal;\n"
            "varying highp vec2 oTexCoord;\n"
            "varying highp vec2 oTexCoord1;\n"
            "vec3 multiply( mat4 m, vec3 v )\n"
            "{\n"
            "   return vec3(\n"
            "      m[0].x * v.x + m[1].x * v.y + m[2].x * v.z,\n"
            "      m[0].y * v.x + m[1].y * v.y + m[2].y * v.z,\n"
            "      m[0].z * v.x + m[1].z * v.y + m[2].z * v.z );\n"
            "}\n"
            "vec3 transposeMultiply( mat4 m, vec3 v )\n"
            "{\n"
            "   return vec3(\n"
            "      m[0].x * v.x + m[0].y * v.y + m[0].z * v.z,\n"
            "      m[1].x * v.x + m[1].y * v.y + m[1].z * v.z,\n"
            "      m[2].x * v.x + m[2].y * v.y + m[2].z * v.z );\n"
            "}\n"
            "void main()\n"
            "{\n"
            "   gl_Position = Mvpm * ( Joints[int(JointIndices.x)] * Position );\n"
            "   vec3 eye = transposeMultiply( Viewm, -vec3( Viewm[3] ) );\n"
            "   oEye = eye - vec3( Modelm * ( Joints[int(JointIndices.x)] * Position ) );\n"
            "   oNormal = multiply( Modelm, multiply( Joints[int(JointIndices.x)], Normal ) );\n"
            "   oTangent = multiply( Modelm, multiply( Joints[int(JointIndices.x)], Tangent ) );\n"
            "   oBinormal = multiply( Modelm, multiply( Joints[int(JointIndices.x)], Binormal ) );\n"
            "   oTexCoord = TexCoord;\n"
            "   oTexCoord1 = TexCoord1;\n"
            "}\n";

    return reflectionMappedSkinned1VertexShaderSrc;
}
const char * VGlShader::getReflectionMappedFragmentShaderSource()
{
    const char * reflectionMappedFragmentShaderSrc =
            "uniform sampler2D Texture0;\n"
            "uniform sampler2D Texture1;\n"
            "uniform sampler2D Texture2;\n"
            "uniform sampler2D Texture3;\n"
            "uniform samplerCube Texture4;\n"
            "varying highp vec3 oEye;\n"
            "varying highp vec3 oNormal;\n"
            "varying highp vec3 oTangent;\n"
            "varying highp vec3 oBinormal;\n"
            "varying highp vec2 oTexCoord;\n"
            "varying highp vec2 oTexCoord1;\n"
            "void main()\n"
            "{\n"
            "   mediump vec3 normal = texture2D( Texture2, oTexCoord ).xyz * 2.0 - 1.0;\n"
            "   mediump vec3 surfaceNormal = normal.x * oTangent + normal.y * oBinormal + normal.z * oNormal;\n"
            "   mediump vec3 eyeDir = normalize( oEye.xyz );\n"
            "   mediump vec3 reflectionDir = dot( eyeDir, surfaceNormal ) * 2.0 * surfaceNormal - eyeDir;\n"
            "   lowp vec3 specular = texture2D( Texture3, oTexCoord ).xyz * textureCube( Texture4, reflectionDir ).xyz;\n"
            "   lowp vec4 diffuse = texture2D( Texture0, oTexCoord );\n"
            "   lowp vec4 emissive = texture2D( Texture1, oTexCoord1 );\n"
            "	gl_FragColor.xyz = diffuse.xyz * emissive.xyz * 1.5 + specular;\n"
            "   gl_FragColor.w = diffuse.w;\n"
            "}\n";
    return reflectionMappedFragmentShaderSrc;
}
const char * VGlShader::getUntexturedFragmentShaderSource()
{
    const char * untexturedFragmentShaderSource =
            "varying lowp vec4 oColor;\n"
            "void main()\n"
            "{\n"
            "	gl_FragColor = oColor;\n"
            "}\n";
    return untexturedFragmentShaderSource;

}
const char * VGlShader::getUniformColorVertexShaderSource()
{
    const char * uniformColorVertexShaderSource =
            "attribute vec4 Position;\n"
            "attribute vec4 VertexColor;\n"
            "attribute vec2 TexCoord;\n"
            "uniform mediump vec4 UniformColor;\n"
            "varying highp vec2 oTexCoord;\n"
            "varying lowp vec4 oColor;\n"
            "void main()\n"
            "{\n"
            "   gl_Position = Position;\n"
            "	oTexCoord = TexCoord;\n"
            "   oColor = VertexColor * UniformColor;\n"
            "}\n";
    return uniformColorVertexShaderSource;
}

const char * VGlShader::getVertexColorVertexShaderSource()
{
    const char * vertexColorVertexShaderSrc =
            "uniform highp mat4 Mvpm;\n"
            "attribute highp vec4 Position;\n"
            "attribute lowp vec4 VertexColor;\n"
            "varying lowp vec4 oColor;\n"
            "void main()\n"
            "{\n"
            "   gl_Position = Mvpm * Position;\n"
            "   oColor = VertexColor;\n"
            "}\n";
    return vertexColorVertexShaderSrc;


}


const char * VGlShader::getVertexColorSkVertexShaderSource()
{
    const char * vertexColorSkinned1VertexShaderSrc =
            "uniform highp mat4 Mvpm;\n"
            "uniform highp mat4 Joints[" STRINGIZE( 16 ) "];\n"
            "attribute highp vec4 Position;\n"
            "attribute lowp vec4 VertexColor;\n"
            "attribute highp vec4 JointWeights;\n"
            "attribute highp vec4 JointIndices;\n"
            "varying lowp vec4 oColor;\n"
            "void main()\n"
            "{\n"
            "   gl_Position = Mvpm * ( Joints[int(JointIndices.x)] * Position );\n"
            "   oColor = VertexColor;\n"
            "}\n";
return vertexColorSkinned1VertexShaderSrc;
}
const char * VGlShader::getVertexColorFragmentShaderSource()
{

    const char * vertexColorFragmentShaderSrc =
            "varying lowp vec4 oColor;\n"
            "void main()\n"
            "{\n"
            "   gl_FragColor = oColor;\n"
            "}\n";
return  vertexColorFragmentShaderSrc;

}

 const char * VGlShader::getAdditionalVertexShaderSource()

 {

     const char* additionalVertexShaderSource =
             "uniform mat4 Mvpm[NUM_VIEWS];\n"
             "attribute vec4 Position;\n"
             "attribute vec4 VertexColor;\n"
             "attribute vec2 TexCoord;\n"
             "uniform mediump vec4 UniformColor;\n"
             "varying  highp vec2 oTexCoord;\n"
             "varying  lowp vec4 oColor;\n"
             "void main()\n"
             "{\n"
             "   gl_Position = Mvpm[VIEW_ID] * Position;\n"
             "   oTexCoord = TexCoord;\n"
             "   oColor = VertexColor * UniformColor;\n"
             "}\n";
     return additionalVertexShaderSource;
 }

const char * VGlShader::getAdditionalFragmentShaderSource()
{

    const char * externalFragmentShaderSource =
            "#extension GL_OES_EGL_image_external : require\n"
            "uniform samplerExternalOES Texture0;\n"
            "uniform lowp vec4 ColorBias;\n"
            "varying highp vec2 oTexCoord;\n"
            "varying lowp vec4 oColor;\n"
            "void main()\n"
            "{\n"
            "	gl_FragColor = ColorBias + oColor * texture2D( Texture0, oTexCoord );\n"
            "}\n";
    return externalFragmentShaderSource;

}

static const char version[] = "#version 300 es\n";
static const char EGL_IMAGE_EXT_ADRENO [] = "#extension GL_OES_EGL_image_external_essl3 : require\n";

static const char * FindShaderVersionEnd(const char *src)
{
    if ( src == NULL || strncmp( src, "#version ", 9 ) != 0 )
    {
        return src;
    }
    while ( *src != 0 && *src != '\n' )
    {
        src++;
    }
    if ( *src == '\n' )
    {
        src++;
    }
    return src;
}

struct VGlShader::Private
{

};

VGlShader::~VGlShader()
{

}

GLuint VGlShader::createShader(GLuint shaderType, const char *src)
{
    GLuint shader = glCreateShader( shaderType );

    if(useMultiview)
    {
        const char * sources[5];
        int len = 0;

        const char * postVersion = FindShaderVersionEnd( src );
        sources[len++] = version;

        if(strstr(src, "GL_OES_EGL_image_external") != NULL)
        {
            const char* vendor = (const char*) glGetString(GL_VENDOR);
            if(strcmp(vendor, "Qualcomm") == 0 && VEglDriver::glIsExtensionString("GL_OES_EGL_image_external_essl3")) sources[len++] = EGL_IMAGE_EXT_ADRENO;
            else VEyeItem::settings.useMultiview = false;
        }

        if(shaderType == GL_VERTEX_SHADER)
        {

            sources[len++] = { ( VEyeItem::settings.useMultiview ) ?
                               "#extension GL_OVR_multiview2 : enable\n"
                                       "#define NUM_VIEWS 2\n"
                                       "layout(num_views=NUM_VIEWS) in;\n"
                                       "#define VIEW_ID gl_ViewID_OVR\n"
                                       "#define varying out\n"
                                       "#define attribute in\n"
                              :  "#define NUM_VIEWS 1\n"
                                       "#define VIEW_ID 0\n"
                                       "#define varying out\n"
                                       "#define attribute in\n"
            };
        }
        else
        {
            sources[len++] = {( VEyeItem::settings.useMultiview ) ?
                              "#extension GL_OVR_multiview2 : enable\n"
                                      "#define varying in\n"
                                      "#define attribute out\n"
                                      "#define VIEW_ID gl_ViewID_OVR\n"
                                      "#ifdef varying\n"
                                      "    out vec4 gl_FragColor;\n"
                                      "#endif\n"
                                      "#define texture2D texture\n"
                                      "#define textureCube texture\n"
                                      "#define texture2DProj textureProj\n"
                              :  "#define varying in\n"
                                      "#define attribute out\n"
                                      "#ifdef varying\n"
                                      "    out vec4 gl_FragColor;\n"
                                      "#endif\n"
                                      "#define texture2D texture\n"
                                      "#define textureCube texture\n"
                                      "#define texture2DProj textureProj\n"
            };
        }
        sources[len++] = postVersion;
        sources[len] = "\0";

        glShaderSource(shader, len, sources, 0 );
    }
    else glShaderSource( shader, 1, &src, 0 );

    glCompileShader( shader );

    GLint r;
    glGetShaderiv( shader, GL_COMPILE_STATUS, &r );
    if ( r == GL_FALSE )
    {
        vInfo( "Compiling shader: "<<src<<"****** failed ******\n" );
        GLchar msg[4096];
        glGetShaderInfoLog( shader, sizeof( msg ), 0, msg );
        vInfo( msg );
        vFatal( "Failed to compile shader,type:"<<shaderType);
    }

    return  shader;
}

GLuint VGlShader::createProgram(GLuint vertexShader, GLuint fragmentShader)
{
    program = glCreateProgram();
    glAttachShader( program, vertexShader );
    glAttachShader( program, fragmentShader );

    // set attributes before linking

    glBindAttribLocation( program, VERTEX_POSITION,		"Position" );
    glBindAttribLocation( program, VERTEX_NORMAL,			"Normal" );
    glBindAttribLocation( program, VERTEX_TANGENT,			"Tangent" );
    glBindAttribLocation( program, VERTEX_BINORMAL,		"Binormal" );
    glBindAttribLocation( program, VERTEX_COLOR,			"VertexColor" );
    glBindAttribLocation( program, VERTEX_UVC0,				"TexCoord" );
    glBindAttribLocation( program, VERTEX_UVC1,				"TexCoord1" );
    glBindAttribLocation( program, JOINT_WEIGHTS,	"JointWeights" );
    glBindAttribLocation( program, JOINT_INDICES,	"JointIndices" );
    glBindAttribLocation( program, FONT_PARMS,		"FontParms" );



    // link and error check
    glLinkProgram( program );
    GLint r;
    glGetProgramiv( program, GL_LINK_STATUS, &r );
    if ( r == GL_FALSE )
    {
        GLchar msg[1024];
        glGetProgramInfoLog( program, sizeof( msg ), 0, msg );
        vFatal( "Linking program failed: "<<msg );
    }
    uniformModelViewProMatrix = glGetUniformLocation( program, "Mvpm" );
    uniformModelMatrix = glGetUniformLocation( program, "Modelm" );
    uniformViewMatrix = glGetUniformLocation( program, "Viewm" );
    uniformProjectionMatrix = glGetUniformLocation( program, "Projectionm" );
    uniformColor = glGetUniformLocation( program, "UniformColor" );
    uniformTexMatrix = glGetUniformLocation( program, "Texm" );
    uniformTexMatrix2 = glGetUniformLocation( program, "Texm2" );
    uniformTexMatrix3 = glGetUniformLocation( program, "Texm3" );
    uniformTexMatrix4 = glGetUniformLocation( program, "Texm4" );
    uniformTexMatrix5 = glGetUniformLocation( program, "Texm5" );
    uniformTexClamp = glGetUniformLocation( program, "TexClamp" );
    uniformRotateScale = glGetUniformLocation( program, "RotateScale" );
    uniformJoints = glGetUniformLocation(program, "Joints" );
    uniformColorTableOffset = glGetUniformLocation( program, "ColorTableOffset" );
    uniformFadeDirection = glGetUniformLocation(program, "UniformFadeDirection" );

    glUseProgram( program );

    // texture and image_external bindings
    for ( int i = 0; i < 8; i++ )
    {
        char name[32];
        sprintf( name, "Texture%i", i );
        const GLint uTex = glGetUniformLocation( program, name );
        if ( uTex != -1 )
        {
            glUniform1i( uTex, i );
        }
    }

    return  program;
}

GLuint VGlShader::initShader(const char *vertexSrc, const char *fragmentSrc)
{
    vertexShader = createShader( GL_VERTEX_SHADER ,vertexSrc);
    fragmentShader = createShader( GL_FRAGMENT_SHADER ,fragmentSrc);
    program = createProgram(vertexShader,fragmentShader);

    glUseProgram( 0 );

    return  program;
}

void  VGlShader::destroy() 
{
    if ( program != 0 )
    {
        glDeleteProgram( program );
    }
    if ( vertexShader != 0 )
    {
        glDeleteShader( vertexShader );
    }
    if ( fragmentShader != 0 )
    {
        glDeleteShader( fragmentShader );
    }
    program = 0;
    vertexShader = 0;
    fragmentShader = 0;
}

NV_NAMESPACE_END
