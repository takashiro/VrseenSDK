
DEFINES += NV_NAMESPACE=NervGear

INCLUDEPATH += \
    $$PWD/jni/core \
    $PWD/jni/3rdparty/TinyXml \
    $$PWD/jni/3rdparty/minizip \
    $$PWD/jni

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

LIBS += -L"$$PWD/libs/" -lNervGear
ANDROID_EXTRA_LIBS += $$PWD/libs/libNervGear.so

include(cflags.pri)
