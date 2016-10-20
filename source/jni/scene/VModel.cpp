#include "VModel.h"
#include "VGlShader.h"
#include "VResource.h"
#include "VGlGeometry.h"
#include "VFile.h"
#include "VTexture.h"
#include "App.h"

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <assimp/port/AndroidJNI/AndroidJNIIOSystem.h>
#include <core/VEventLoop.h>

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

static pthread_t loadingThread = 0;
static VEglDriver      m_eglStatus;
static EGLint			m_eglClientVersion;
static EGLContext		m_eglShareContext;

VEventLoop* eventLoop = NULL;

struct ModelMesh
{
    VertexAttribs attribs;
    VArray<unsigned short> indices;
    unsigned int textureId;
};

struct VModel::Private
{
    Private()
    {
        loadModelProgram.initShader(glVertexShader,glFragmentShader);
    }

    VGlShader loadModelProgram;
    VArray<VGlGeometry> geos;
    std::function<void()> completeListener;

    static void* loadModelAsync(void* param);
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

    if(eventLoop!=NULL)
    {
        eventLoop->send("quit");

        void * data;
        pthread_join( loadingThread, &data );
        loadingThread = 0;

        delete eventLoop;
        eventLoop = NULL;
    }
}

bool VModel::loadAsync(VString& modelPath,std::function<void()> completeListener)
{
    if(!loadingThread)
    {
        m_eglStatus.updateDisplay();
        m_eglShareContext = eglGetCurrentContext();

        if ( m_eglShareContext == EGL_NO_CONTEXT )
        {
            vFatal( "EGL_NO_CONTEXT" );
        }
        EGLint configID;
        if ( !eglQueryContext( m_eglStatus.m_display, m_eglShareContext, EGL_CONFIG_ID, &configID ) )
        {
            vFatal( "eglQueryContext EGL_CONFIG_ID failed" );
        }
        m_eglStatus.m_config = m_eglStatus.eglConfigForConfigID( configID );
        if ( m_eglStatus.m_config == NULL )
        {
            vFatal( "EglConfigForConfigID failed" );
        }
        if ( !eglQueryContext( m_eglStatus.m_display, m_eglShareContext, EGL_CONTEXT_CLIENT_VERSION, (EGLint *)&m_eglClientVersion ) )
        {
            vFatal( "eglQueryContext EGL_CONTEXT_CLIENT_VERSION failed" );
        }

        EGLint SurfaceAttribs [ ] =
        {
            EGL_WIDTH, 1,
            EGL_HEIGHT, 1,
            EGL_NONE
        };
        m_eglStatus.m_pbufferSurface = eglCreatePbufferSurface( m_eglStatus.m_display, m_eglStatus.m_config, SurfaceAttribs );

        const int createErr = pthread_create( &loadingThread, NULL, &VModel::Private::loadModelAsync, NULL );
        if ( createErr != 0 )
        {
            vInfo("pthread_create returned" << createErr);
            return false;
        }
    }

    d->completeListener = completeListener;
    if(!eventLoop) eventLoop = new VEventLoop(10);
    VVariantArray args;
    args<<(void*)d<<modelPath;
    eventLoop->post("loadModel", args);

    return true;
}

void VModel::draw(int eye, const VMatrix4f & mvp )
{
    NV_UNUSED(eye);

    const VGlShader * shader = &d->loadModelProgram;
    //hack code,temporary for 158demo branch
    const VMatrix4f mvp2  = mvp * VMatrix4f::RotationX(-M_PI*0.5) * VMatrix4f::Scaling(10.0f,10.0f,10.0f);
    glUseProgram(shader->program);
    glUniformMatrix4fv(shader->uniformModelViewProMatrix, 1, GL_FALSE, mvp2.transposed().cell[0]);
    VEglDriver::glPushAttrib();

    glEnable(GL_DEPTH_TEST);
    glDepthFunc( GL_LEQUAL );

    for(unsigned int i = 0,len = d->geos.size();i<len;++i)
    {
        d->geos[i].drawElements();
    }

    VEglDriver::glPopAttrib();
}

void* VModel::Private::loadModelAsync(void* param)
{
    NV_UNUSED(param);
    pthread_setname_np( pthread_self(), "VModel::BackgroundLoad" );

    EGLint contextAttribs[] =
    {
        EGL_CONTEXT_CLIENT_VERSION, m_eglClientVersion,
        EGL_NONE, EGL_NONE,
        EGL_NONE
    };

    m_eglStatus.m_context = eglCreateContext( m_eglStatus.m_display, m_eglStatus.m_config, m_eglShareContext, contextAttribs );
    if ( m_eglStatus.m_context == EGL_NO_CONTEXT )
    {
        vFatal( "eglCreateContext failed");
    }

    if (eglMakeCurrent( m_eglStatus.m_display, m_eglStatus.m_pbufferSurface, m_eglStatus.m_pbufferSurface, m_eglStatus.m_context ) == EGL_FALSE )
    {
        vFatal("eglMakeCurrent failed:" << VEglDriver::getEglErrorString());
    }

    for(;;)
    {
        eventLoop->wait();
        VEvent event = eventLoop->next();

        if (event.name == "loadModel")
        {
            VModel::Private * d = static_cast<VModel::Private * >(event.data.at(0).toPointer());
            VString modelPath = event.data.at(1).toString();

//            VResource modelFile(modelPath);
//            if (!modelFile.exists()) {
//                vWarn("VModel::Load failed to read" << modelPath);
//                return false;
//            }
//
//            Assimp::Importer importer;
//            const aiScene* scene = importer.ReadFileFromMemory(modelFile.data().data(), modelFile.size(),aiProcessPreset_TargetRealtime_Quality);

            Assimp::Importer importer;
//            Assimp::AndroidJNIIOSystem* ioSystem = new Assimp::AndroidJNIIOSystem(apkAssetManager,apkInternalPath);
//            importer.SetIOHandler(ioSystem);
            const aiScene* scene = importer.ReadFile(modelPath.toStdString(), aiProcessPreset_TargetRealtime_Quality);

            if(!scene)
            {
                vWarn("VModel::Load " << importer.GetErrorString() );
            }
            else vInfo("VModel::Load Success" << modelPath);

            unsigned int meshCount = scene->mNumMeshes;
            d->geos.resize(meshCount);
            ModelMesh* modelMeshes = new ModelMesh[meshCount];

            for (unsigned int i = 0; i < meshCount; i++)
            {
                const aiMesh *mesh = scene->mMeshes[i];
                unsigned int vertexCount = mesh->mNumVertices;
                modelMeshes[i].attribs.position.resize( vertexCount );

                for (unsigned int j = 0; j < vertexCount; j++)
                {
                    const aiVector3D& vector = mesh->mVertices[j];
                    modelMeshes[i].attribs.position[j] .x = vector.x;
                    modelMeshes[i].attribs.position[j] .y = vector.y;
                    modelMeshes[i].attribs.position[j] .z = vector.z;
                }

                unsigned int faceCount = mesh->mNumFaces;
                unsigned int index = 0;
                modelMeshes[i].indices.resize( faceCount * 3 );

                for (unsigned int j = 0 ; j < faceCount ; j++)
                {
                    const aiFace& face = mesh->mFaces[j];
                    modelMeshes[i].indices[index + 0] = face.mIndices[0];
                    modelMeshes[i].indices[index + 1] = face.mIndices[1];
                    modelMeshes[i].indices[index + 2] = face.mIndices[2];
                    index += 3;
                }

                if (mesh->HasTextureCoords(0))
                {
                    modelMeshes[i].attribs.uvCoordinate0.resize(vertexCount);

                    for (unsigned int j = 0; j < vertexCount; j++)
                    {
                        modelMeshes[i]. attribs.uvCoordinate0[j].x = mesh->mTextureCoords[0][j].x;
                        modelMeshes[i].attribs.uvCoordinate0[j].y = mesh->mTextureCoords[0][j].y;
                    }

                    aiMaterial *mtl = scene->mMaterials[mesh->mMaterialIndex];
                    aiString textureFilename;

                    if (mtl && AI_SUCCESS == mtl->GetTexture(aiTextureType_DIFFUSE, 0, &textureFilename))
                    {
                        VString modelDirectoryName = VPath(modelPath).dirPath();
                        //VString modelDirectoryName = "assets";
                        VString textureFullPath = modelDirectoryName + "/" + textureFilename.data;
                        textureFullPath.replace('\\','/');

                        VTexture texture;
                        VFile textureFile(textureFullPath,VFile::ReadOnly);
                        texture.load(textureFile);
                        modelMeshes[i].textureId = texture.id();
                    }
                }
            }

            EGLSyncKHR GpuSync = VEglDriver::eglCreateSyncKHR( m_eglStatus.m_display, EGL_SYNC_FENCE_KHR, NULL );
            if ( GpuSync == EGL_NO_SYNC_KHR ) {
                vFatal("BackgroundGLLoadThread eglCreateSyncKHR_():EGL_NO_SYNC_KHR");
            }

            if ( EGL_FALSE == VEglDriver::eglClientWaitSyncKHR(m_eglStatus.m_display, GpuSync, EGL_SYNC_FLUSH_COMMANDS_BIT_KHR,
                                                               EGL_FOREVER_KHR ) )
            {
                vInfo("BackgroundGLLoadThread eglClientWaitSyncKHR returned EGL_FALSE");
            }

            VVariantArray args;
            args<<(void*)d<<(void*)modelMeshes<<meshCount;
            vApp->eventLoop().post("loadModelCompleted",args);
        }
        else if (event.name == "quit")
        {
            eventLoop->quit();
            break;
        }
    }

    if ( eglMakeCurrent( m_eglStatus.m_display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT ) == EGL_FALSE )
    {
        vFatal("eglMakeCurrent: shutdown failed");
    }

    if (eglDestroyContext( m_eglStatus.m_display, m_eglStatus.m_context ) == EGL_FALSE )
    {
        vFatal("eglDestroyContext: shutdown failed");
    }

    return NULL;
}

void VModel::command(const VEvent &event )
{
    if (event.name == "loadModelCompleted") {
        VModel::Private * d = static_cast<VModel::Private * >(event.data.at(0).toPointer());
        ModelMesh * modelMeshes = static_cast<ModelMesh * >(event.data.at(1).toPointer());
        int meshCount = event.data.at(2).toInt();

        if(meshCount<=0||modelMeshes==NULL) return;

        for(int i=0;i<meshCount;++i)
        {
            d->geos[i].createGlGeometry(modelMeshes[i].attribs,modelMeshes[i].indices);
            d->geos[i].textureId = modelMeshes[i].textureId;
        }

        delete modelMeshes;

        if (d->completeListener) {
            d->completeListener();
        }

        return;
    }
}

NV_NAMESPACE_END
