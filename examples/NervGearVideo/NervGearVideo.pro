TEMPLATE = app
CONFIG += console
CONFIG -= app_bundle
CONFIG -= qt
TARGET = nervgearvideo

SOURCES += \
    jni/Oculus360Videos.cpp \
    jni/VideoBrowser.cpp \
    jni/VideoMenu.cpp \
    jni/VideosMetaData.cpp \
    jni/OVR_TurboJpeg.cpp

HEADERS += \
    jni/Oculus360Videos.h \
    jni/VideoBrowser.h \
    jni/VideoMenu.h \
    jni/VideosMetaData.h \
    jni/OVR_TurboJpeg.h

LIBS += -L"$$PWD/jni/" -ljpeg

ANDROID_APP_DIRS = $$absolute_path(../../source) $$PWD
include(../../source/makeApk.pri)

include(../../source/dynamicVrLib.pri)
