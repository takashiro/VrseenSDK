#include "VModel.h"
#include "VGlShader.h"
#include "VResource.h"
#include "VGlGeometry.h"
#include "VTexture.h"

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <assimp/port/AndroidJNI/AndroidJNIIOSystem.h>

NV_NAMESPACE_BEGIN

static std::string apkInternalPath;
static AAssetManager *apkAssetManager = NULL;

extern "C"
{
void Java_com_vrseen_VrActivity_nativeInitLoadModel(JNIEnv *jni, jclass, jobject assetManager, jstring pathToInternalDir)
{
    apkAssetManager = AAssetManager_fromJava(jni, assetManager);

    const char *cPathToInternalDir;
    cPathToInternalDir = jni->GetStringUTFChars(pathToInternalDir, NULL ) ;
    apkInternalPath = std::string(cPathToInternalDir);
    jni->ReleaseStringUTFChars(pathToInternalDir, cPathToInternalDir);
}
}	// extern "C"


static const char*  glVertexShader =
        "uniform highp mat4 Mvpm;\n"
                "attribute vec4 Position;\n"
                "attribute vec2 TexCoord;\n"
                "varying  highp vec2 oTexCoord;\n"
                "void main()\n"
                "{\n"
                "    gl_Position = Mvpm * Position;\n"
                "    oTexCoord = vec2(TexCoord.x,1.0-TexCoord.y);\n"
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

bool VModel::load(VString& modelPath)
{
//    VResource modelFile(modelPath);
//    if (!modelFile.exists()) {
//        vWarn("VModel::Load failed to read" << path);
//        return false;
//    }

//    Assimp::Importer importer;
//    const aiScene* scene = importer.ReadFileFromMemory(modelFile.data().data(), modelFile.size(),aiProcessPreset_TargetRealtime_Quality);

    Assimp::Importer importer;
    Assimp::AndroidJNIIOSystem* ioSystem = new Assimp::AndroidJNIIOSystem(apkAssetManager,apkInternalPath);
    importer.SetIOHandler(ioSystem);
    const aiScene* scene = importer.ReadFile(modelPath.toStdString(), aiProcessPreset_TargetRealtime_Quality);

    if(!scene)
    {
        vWarn("VModel::Load " << importer.GetErrorString() );
        return false;
    }
    else vInfo("VModel::Load Success" << modelPath);

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
//                VString modelDirectoryName = VPath(modelPath).dirPath();
                VString modelDirectoryName = "assets";
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
    glUniformMatrix4fv(shader->uniformModelViewProMatrix, 1, GL_FALSE, mvp.transposed().cell[0]);
    VEglDriver::glPushAttrib();

    glEnable(GL_DEPTH_TEST);
    glDepthFunc( GL_LEQUAL );

    for(unsigned int i = 0,len = d->geos.size();i<len;++i)
    {
        d->geos[i].drawElements();
    }

    VEglDriver::glPopAttrib();
}

NV_NAMESPACE_END
