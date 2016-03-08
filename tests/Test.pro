TEMPLATE = app

TEMPLATE = app
CONFIG += console c++11
CONFIG -= app_bundle
CONFIG -= qt

DEFINES += NV_NAMESPACE=NervGear

NV_ROOT = $PWD/../../source/jni

INCLUDEPATH += \
    $$NV_ROOT/ \
    $$NV_ROOT/core

HEADERS += test.h
SOURCES += main.cpp

include(core/core.pri)
