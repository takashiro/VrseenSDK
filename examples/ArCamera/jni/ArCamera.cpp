#include "ArCamera.h"

#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <jni.h>
#include <android/keycodes.h>

#include <VPath.h>
#include <VZipFile.h>
#include <VFile.h>
#include <VResource.h>
#include <VEyeItem.h>
#include <VTimer.h>
#include <VColor.h>
#include <VArray.h>
#include <VString.h>
#include <VEglDriver.h>

#include <android/JniUtils.h>

#include "SurfaceTexture.h"
#include "VTexture.h"
#include "BitmapFont.h"
#include "GazeCursor.h"
#include "App.h"

NV_NAMESPACE_BEGIN
static const char* cameraVertexShaderSrc =
        "attribute vec4 Position;\n"
        "attribute vec2 TexCoord;\n"
        "varying  highp vec2 oTexCoord;\n"
        "void main()\n"
        "{\n"
        "   gl_Position = Position;\n"
        "   oTexCoord = TexCoord;\n"
        "}\n";
static const char* cameraFragmentShaderSrc =
        "#extension GL_OES_EGL_image_external : require\n"
        "uniform samplerExternalOES cam_tex;\n"
        "varying highp vec2 oTexCoord;\n"
        "void main()\n"
        "{\n"
				"gl_FragColor = texture2D( cam_tex, oTexCoord );\n"
        "}\n";

extern "C" {
static GLuint createShader(const char *src,GLuint shaderType){
	GLuint shader = glCreateShader( shaderType );
	glShaderSource( shader, 1, &src, 0 );
    glCompileShader( shader );

    GLint r;
    glGetShaderiv( shader, GL_COMPILE_STATUS, &r );
    if ( r == GL_FALSE )
    {
        vInfo( "Compiling shader: "<<src<<"****** failed ******\n" );
        GLchar msg[4096];
        glGetShaderInfoLog( shader, sizeof( msg ), 0, msg );
        vInfo( msg );
        return 0;
    }
	return shader;
}
static GLuint createProgram(const char *ver, const char * fag){

	GLuint vshader = createShader(ver,GL_VERTEX_SHADER);
	GLuint fshader = createShader(fag,GL_FRAGMENT_SHADER);
	GLuint p = glCreateProgram();
    glAttachShader( p, vshader );
    glAttachShader( p, fshader );
	glLinkProgram( p );
    GLint r;
    glGetProgramiv( p, GL_LINK_STATUS, &r );
    if ( r == GL_FALSE )
    {
        GLchar msg[1024];
        glGetProgramInfoLog( p, sizeof( msg ), 0, msg );
        vFatal( "Linking program failed: "<<msg );
    }
	return p;
}
static GLuint createRect(GLuint p){
    VGlGeometry geometry;
    struct vertices_t
    {
        float positions[4][3];
        float uvs[4][2];
    }
            vertices =
            {
                    { { -0.99, -0.99, 0 }, { -0.99, 0.99, 0 }, { 0.99, -0.99, 0 }, { 0.99, 0.99, 0 } },
                    { { 0, 1 }, { 0, 0 }, { 1, 1 }, { 1, 0 } }
            };
    unsigned short indices[6] = { 0, 1, 2, 2, 1, 3 };

    geometry.vertexCount = 4;
    geometry.indexCount = 6;

    VEglDriver::glGenVertexArraysOES( 1, &geometry.vertexArrayObject );
    VEglDriver::glBindVertexArrayOES( geometry.vertexArrayObject );

    glGenBuffers( 1, &geometry.vertexBuffer );
    glBindBuffer( GL_ARRAY_BUFFER, geometry.vertexBuffer );
    glBufferData( GL_ARRAY_BUFFER, sizeof( vertices ), &vertices, GL_STATIC_DRAW );

    glGenBuffers( 1, &geometry.indexBuffer );
    glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, geometry.indexBuffer );
    glBufferData( GL_ELEMENT_ARRAY_BUFFER, sizeof( indices ), indices, GL_STATIC_DRAW );


	GLuint ind = glGetAttribLocation(p,"Position");
    glEnableVertexAttribArray( ind );
    glVertexAttribPointer( ind, 3, GL_FLOAT, false,
                           sizeof( vertices.positions[0] ), (const GLvoid *)offsetof( vertices_t, positions ) );

	ind = glGetAttribLocation(p,"TexCoord");
    glEnableVertexAttribArray( ind );
    glVertexAttribPointer( ind, 2, GL_FLOAT, false,
                           sizeof( vertices.uvs[0] ), (const GLvoid *)offsetof( vertices_t, uvs ) );
    VEglDriver::glBindVertexArrayOES( 0 );
	return geometry.vertexArrayObject;
}
/*jobject Java_com_vrseen_arcamera_ArCamera_nativeGetCameraSurfaceTexture( JNIEnv *jni, jclass clazz,
        jlong appPtr )
{
    LOG( "getCameraSurfaceTexture: %i",
            (vApp->GetCameraTexture()->textureId ));
    return vApp->GetCameraTexture()->javaObject;
}

void Java_com_vrseen_arcamera_ArCamera_nativeSetCameraFov( JNIEnv *jni, jclass clazz,
        jlong appPtr, jfloat fovHorizontal, jfloat fovVertical )
{
    LOG( "nativeSetCameraFov %.2f %.2f", fovHorizontal, fovVertical );
    VVariantArray args;
    args << fovHorizontal << fovVertical;
    vApp->eventLoop().post("video", std::move(args));
}*/

void Java_com_vrseen_arcamera_ArCamera_construct(JNIEnv *jni, jclass, jobject activity)
{
    (new ArCamera(jni, jni->GetObjectClass(activity), activity))->onCreate(nullptr, nullptr, nullptr);
}

jobject Java_com_vrseen_arcamera_ArCamera_createCameraTexture(JNIEnv *, jclass)
{
	// set up a message queue to get the return message
	// TODO: make a class that encapsulates this work
    VEventLoop result(1);
    vApp->eventLoop().post("newTexture", &result);
	result.wait();
    VEvent event = result.next();
    if (event.name == "surfaceTexture") {
        return static_cast<jobject>(event.data.toPointer());
    }
	vWarn("Failed to create camera texture");
    return NULL;
}

} // extern "C"



ArCamera::ArCamera(JNIEnv *jni, jclass activityClass, jobject activityObject)
    : VMainActivity(jni, activityClass, activityObject)
    , m_isPause(false)
    , m_cameraTexture(nullptr)
{
}

ArCamera::~ArCamera()
{
}

void ArCamera::init(const VString &, const VString &, const VString &)
{
    program = createProgram(cameraVertexShaderSrc,cameraFragmentShaderSrc);
	vao = createRect(program);
}

void ArCamera::shutdown()
{
    if (m_cameraTexture != NULL) {
        delete m_cameraTexture;
        m_cameraTexture = NULL;
	}
}

void ArCamera::configureVrMode(VKernel* kernel)
{
	kernel->msaa = 1;
}

bool ArCamera::onKeyEvent( const int keyCode, const KeyState::eKeyEventType eventType )
{
    NV_UNUSED(keyCode, eventType);
	return false;
}

void ArCamera::command(const VEvent &event )
{

    if (event.name == "newTexture") {
        delete m_cameraTexture;
        m_cameraTexture = new SurfaceTexture( vApp->vrJni() );
        VEventLoop *receiver = static_cast<VEventLoop *>(event.data.toPointer());
        receiver->post("surfaceTexture", m_cameraTexture->javaObject);
        return;
    }
    if (event.name == "pause") {
        m_isPause = true;
        return;
    }
    if (event.name == "resume") {
        m_isPause = false;
    }
}

VMatrix4f ArCamera::drawEyeView( const int eye, const float fovDegrees )
{
	NV_UNUSED(eye, fovDegrees);
	VMatrix4f mvp;
	if (m_cameraTexture && !m_isPause) {
		glActiveTexture(GL_TEXTURE0);
		m_cameraTexture->Update();
		glBindTexture(GL_TEXTURE_EXTERNAL_OES, 0);
	}
	glUseProgram( program );
	glUniform1i(glGetUniformLocation(program,"cam_tex"),1);
	glClearColor(0, 0, 0, 1);
	glActiveTexture( GL_TEXTURE1 );
	glBindTexture( GL_TEXTURE_EXTERNAL_OES, m_cameraTexture->textureId );
	glDisable( GL_DEPTH_TEST );
	glDisable( GL_CULL_FACE );
	VEglDriver::glBindVertexArrayOES( vao );
	glDrawElements( GL_TRIANGLES, 6, GL_UNSIGNED_SHORT , NULL );
	glActiveTexture(GL_TEXTURE1);
	glBindTexture( GL_TEXTURE_EXTERNAL_OES, 0 );
	glUseProgram(0);
	return mvp;
}

VMatrix4f ArCamera::onNewFrame(VFrame vrFrame ) {
    NV_UNUSED(vrFrame);
    return VMatrix4f();
}

void ArCamera::onResume()
{
	m_isPause = false;
}

void ArCamera::onPause()
{
	m_isPause = true;
}

NV_NAMESPACE_END
