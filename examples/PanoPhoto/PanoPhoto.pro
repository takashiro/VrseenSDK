TEMPLATE = app
CONFIG += console
CONFIG -= app_bundle
QT -= core
QT -= gui
TARGET = panophoto

SOURCES += \
    jni/PanoPhoto.cpp \
    jni/FileLoader.cpp \
    jni/PanoBrowser.cpp \
    jni/PanoMenu.cpp \
    jni/PhotosMetaData.cpp

HEADERS += \
    jni/PanoPhoto.h \
    jni/FileLoader.h \
    jni/PanoBrowser.h \
    jni/PanoMenu.h \
    jni/PhotosMetaData.h

ANDROID_APP_DIR = $$PWD
include(../../source/makeApk.pri)

include(../../source/dynamicVrLib.pri)
