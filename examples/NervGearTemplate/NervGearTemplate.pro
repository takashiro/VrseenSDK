TEMPLATE = app
CONFIG += console
CONFIG -= app_bundle
QT -= core
QT -= gui
TARGET = ovrapp

SOURCES += jni/OvrApp.cpp

HEADERS += jni/OvrApp.h

ANDROID_APP_DIRS = $$absolute_path(../../source) $$PWD
include(../../source/makeApk.pri)

include(../../source/dynamicVrLib.pri)
