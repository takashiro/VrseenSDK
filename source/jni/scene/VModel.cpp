#include "VModel.h"
#include "VGlShader.h"
#include "VResource.h"
#include "VGlGeometry.h"
#include "VMap.h"
#include "VTexture.h"

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <assimp/port/AndroidJNI/AndroidJNIIOSystem.h>

NV_NAMESPACE_BEGIN

static const char*  glVertexShader =
        "uniform highp mat4 Mvpm;\n"
        "attribute vec4 Position;\n"
        "attribute vec2 TexCoord;\n"
        "varying  highp vec2 oTexCoord;\n"
        "void main()\n"
        "{\n"
        "    gl_Position = Mvpm * Position;\n"
        "    oTexCoord = TexCoord;\n"
        "}\n";

static const char*  glFragmentShader =
        "uniform sampler2D Texture0;\n"
        "varying highp vec2 oTexCoord;\n"
        "void main()\n"
        "{\n"
        "    gl_FragColor = texture2D( Texture0, oTexCoord );\n"
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
    for(unsigned int i = 0,len = d->geos.size();i<len;++i)
    {
        d->geos[i].destroy();
    }
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

       const aiMesh *mesh = scene->mMeshes[i];
       unsigned int vertexCount = mesh->mNumVertices;
       attribs.position.resize( vertexCount );

        for (unsigned int j = 0; j < vertexCount; j++)
        {
            const aiVector3D& vector = mesh->mVertices[j];
            attribs.position[j] .x = vector.x;
            attribs.position[j] .y = vector.y;
            attribs.position[j] .z = vector.z;
        }

        unsigned int faceCount = mesh->mNumFaces;
        unsigned int index = 0;
        indices.resize( faceCount * 3 );

        for (unsigned int j = 0 ; j < faceCount ; j++)
        {
            const aiFace& face = mesh->mFaces[j];
            indices[index + 0] = face.mIndices[0];
            indices[index + 1] = face.mIndices[1];
            indices[index + 2] = face.mIndices[2];
            index += 3;
        }

        if (mesh->HasTextureCoords(0))
        {
            attribs.uvCoordinate0.resize(vertexCount);

            for (unsigned int j = 0; j < vertexCount; j++)
            {
                attribs.uvCoordinate0[j].x = mesh->mTextureCoords[0][j].x;
                attribs.uvCoordinate0[j].y = mesh->mTextureCoords[0][j].y;
            }

            aiMaterial *mtl = scene->mMaterials[mesh->mMaterialIndex];
            aiString textureFilename;

            if (mtl && AI_SUCCESS == mtl->GetTexture(aiTextureType_DIFFUSE, 0, &textureFilename))
            {
                VString modelDirectoryName = VPath(path).dirPath();
                VString textureFullPath = modelDirectoryName + "/" + textureFilename.data;

                VTexture texture;
                texture.load(VResource(textureFullPath));
                d->geos[i].textureId = texture.id();
            }
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
