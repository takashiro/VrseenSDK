TEMPLATE = app
CONFIG += console
CONFIG -= app_bundle
QT -= core
QT -= gui
TARGET = vrscene

CONFIG(debug, debug|release): DEFINES += OVR_DEBUG=1 NDK_DEBUG=1

SOURCES += jni/VrScene.cpp

HEADERS += jni/VrScene.h

ANDROID_APP_DIRS = $$absolute_path(../../source) $$PWD
include(../../source/makeApk.pri)

include(../../source/dynamicVrLib.pri)
