TEMPLATE = app
CONFIG += console
CONFIG -= app_bundle
CONFIG -= qt
TARGET = nervgearvideo

SOURCES += jni/Oculus360Videos.cpp
HEADERS += jni/Oculus360Videos.h

ANDROID_APP_DIR = $$PWD
include(../../source/makeApk.pri)

include(../../source/dynamicVrLib.pri)
