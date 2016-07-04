TEMPLATE = app
CONFIG += console
CONFIG -= app_bundle
QT -= core
QT -= gui
TARGET = panophoto

SOURCES += \
    jni/PanoPhoto.cpp \
    jni/FileLoader.cpp

HEADERS += \
    jni/PanoPhoto.h \
    jni/FileLoader.h

ANDROID_APP_DIR = $$PWD
include(../../source/makeApk.pri)

include(../../source/dynamicVrLib.pri)
