TEMPLATE = lib
TARGET = vrseen
CONFIG += console
CONFIG -= app_bundle
CONFIG -= qt

CONFIG(debug, debug|release): DEFINES += NDK_DEBUG=1

INCLUDEPATH += \
    jni \
    jni/api \
    jni/core \
    jni/gui \
    jni/io \
    jni/media \
    jni/scene

SOURCES += \
    $$files(jni/core/*.cpp) \
    $$files(jni/core/android/*.cpp) \
    $$files(jni/api/*.cpp) \
    $$files(jni/gui/*.cpp) \
    $$files(jni/io/*.cpp) \
    $$files(jni/media/*.cpp) \
    $$files(jni/scene/*.cpp) \
    jni/App.cpp

HEADERS += \
    $$files(jni/core/*.h) \
    $$files(jni/core/android/*.h) \
    $$files(jni/core/*.h) \
    $$files(jni/api/*.h) \
    $$files(jni/gui/*.h) \
    $$files(jni/io/*.h) \
    $$files(jni/media/*.h) \
    $$files(jni/scene/*.h) \
    jni/App.h \
    jni/vglobal.h

include(jni/3rdparty/minizip/minizip.pri)
include(jni/3rdparty/stb/stb.pri)

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

linux {
    CONFIG(staticlib) {
        QMAKE_POST_LINK = $$QMAKE_COPY $$system_path($$OUT_PWD/lib$${TARGET}.a) $$system_path($$PWD/libs)
    } else {
        QMAKE_POST_LINK = $$QMAKE_COPY $$system_path($$OUT_PWD/lib$${TARGET}.so) $$system_path($$PWD/libs)
    }
}

include(cflags.pri)
include(deployment.pri)
qtcAddDeployment()
