#include "VModel.h"
#include "VGlShader.h"
#include "VResource.h"
#include "VGlGeometry.h"

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <assimp/port/AndroidJNI/AndroidJNIIOSystem.h>

NV_NAMESPACE_BEGIN

static const char*  glVertexShader =
        "uniform highp mat4 Mvpm;\n"
        "attribute vec4 Position;\n"
        "attribute vec4 VertexColor;\n"
        "uniform mediump vec4 UniformColor;\n"
        "varying  lowp vec4 oColor;\n"
        "void main()\n"
        "{\n"
        "    gl_Position = Mvpm * Position;\n"
        "    oColor =  /* VertexColor */ UniformColor;\n"
        "}\n";

static const char*  glFragmentShader =
        "varying lowp vec4 oColor;\n"
        "void main()\n"
        "{\n"
        "    gl_FragColor = oColor;\n"
        "}\n";

struct VModel::Private
{
    Private()
    {
        loadModelProgram.initShader(glVertexShader,glFragmentShader);
    }

    VGlShader loadModelProgram;
    VArray<VGlGeometry> geos;
};


VModel::VModel():d(new Private)
{

}

VModel::~VModel()
{
    delete d;
}

bool VModel::load(VString& path)
{
    VResource modelFile(path);
    if (!modelFile.exists()) {
        vWarn("VModel::Load failed to read" << path);
        return false;
    }

    Assimp::Importer import;
    const aiScene* scene = import.ReadFileFromMemory(modelFile.data().data(), modelFile.size(),aiProcessPreset_TargetRealtime_Quality);

    if(!scene)
    {
        vWarn("VModel::Load " << import.GetErrorString() );
        return false;
    }

    unsigned int meshCount = scene->mNumMeshes;
    d->geos.resize(meshCount);

    for (unsigned int i = 0; i < meshCount; i++)
    {
        VertexAttribs attribs;
        VArray<unsigned short> indices;

       unsigned int vertexCount = scene->mMeshes[i]->mNumVertices;
       attribs.position.resize( vertexCount );

        for (unsigned int j = 0; j < vertexCount; j++)
        {
            const aiVector3D& vector = scene->mMeshes[i]->mVertices[j];
            attribs.position[j] .x = vector.x;
            attribs.position[j] .y = vector.y;
            attribs.position[j] .z = vector.z;
        }

        unsigned int faceCount = scene->mMeshes[i]->mNumFaces;
        unsigned int index = 0;
        indices.resize( faceCount * 3 );

        for (unsigned int j = 0 ; j < faceCount ; j++)
        {
            const aiFace& face = scene->mMeshes[i]->mFaces[j];
            indices[index + 0] = face.mIndices[0];
            indices[index + 1] = face.mIndices[1];
            indices[index + 2] = face.mIndices[2];
            index += 3;
        }

       d->geos[i].createGlGeometry(attribs,indices);
    }

    return true;
}

void VModel::draw(int eye, const VMatrix4f & mvp )
{
    NV_UNUSED(eye);

    const VGlShader * shader = &d->loadModelProgram;

    glUseProgram(shader->program);
    glUniform4f(shader->uniformColor, 1, 0, 0, 1);
    glUniformMatrix4fv(shader->uniformModelViewProMatrix, 1, GL_FALSE, mvp.transposed().cell[0]);

    for(unsigned int i = 0,len = d->geos.size();i<len;++i)
    {
        d->geos[i].drawElements();
    }
}

NV_NAMESPACE_END
