TEMPLATE = app
CONFIG += console
CONFIG -= app_bundle
QT -= core
QT -= gui
TARGET = nervgearphoto

CONFIG(debug, debug|release): DEFINES += OVR_DEBUG=1 NDK_DEBUG=1

SOURCES += \
    jni/Oculus360Photos.cpp \
    jni/FileLoader.cpp \
    jni/PanoBrowser.cpp \
    jni/PanoMenu.cpp \
    jni/PhotosMetaData.cpp \
    jni/OVR_TurboJpeg.cpp

HEADERS += \
    jni/Oculus360Photos.h \
    jni/FileLoader.h \
    jni/PanoBrowser.h \
    jni/PanoMenu.h \
    jni/PhotosMetaData.h \
    jni/OVR_TurboJpeg.h

LIBS += -L"$$PWD/jni/" -ljpeg

ANDROID_APP_DIRS = ../../source $$PWD
include(../../source/makeApk.pri)
include(../../source/dynamicVrLib.pri)
