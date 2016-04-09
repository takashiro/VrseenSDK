TEMPLATE = app

TEMPLATE = app
CONFIG += console c++11
CONFIG -= app_bundle
CONFIG -= qt

DEFINES += NV_NAMESPACE=NervGear

NV_ROOT = $PWD/../../source/jni

INCLUDEPATH += \
    jni \
    $$NV_ROOT \
    $$NV_ROOT/core \
    $$NV_ROOT/io

HEADERS += jni/test.h
SOURCES += jni/main.cpp

include(jni/core/core.pri)
include(jni/io/io.pri)
