
CONFIG(debug, debug|release): DEFINES += NDK_DEBUG=1

INCLUDEPATH += \
    $$PWD/jni \
    $$PWD/jni/api \
    $$PWD/jni/core \
    $$PWD/jni/gui \
    $$PWD/jni/io \
    $$PWD/jni/media \
    $$PWD/jni/scene \
    $$PWD/jni/3rdparty/minizip

# OpenGL ES 3.0
LIBS += -lGLESv3
# GL platform interface
LIBS += -lEGL
# native multimedia
LIBS += -lOpenMAXAL
# logging
LIBS += -llog
# native windows
LIBS += -landroid
# For minizip
LIBS += -lz
# audio
LIBS += -lOpenSLES

LIBS += -L"$$PWD/libs/" -lvrseen
ANDROID_EXTRA_LIBS += $$PWD/libs/libvrseen.so

include(cflags.pri)
