TEMPLATE = app
CONFIG += console
CONFIG -= app_bundle
QT -= core
QT -= gui
TARGET = ovrapp

CONFIG(debug, debug|release): DEFINES += OVR_DEBUG=1 NDK_DEBUG=1

SOURCES += jni/OvrApp.cpp

HEADERS += jni/OvrApp.h

ANDROID_APP_DIRS = ../../source $$PWD
include(../../source/makeApk.pri)

include(../../source/dynamicVrLib.pri)
