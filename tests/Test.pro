TEMPLATE = app

TEMPLATE = app
CONFIG += console c++11
CONFIG -= app_bundle
CONFIG -= qt

NV_ROOT = $PWD/../../source/jni

INCLUDEPATH += \
    jni \
    $$NV_ROOT \
    $$NV_ROOT/core \
    $$NV_ROOT/io \
    $$NV_ROOT/media

HEADERS += jni/test.h

SOURCES += \
    $$files(jni/core/*.cpp) \
    $$files(jni/io/*.cpp) \
    $$files(jni/media/*.cpp) \
    jni/main.cpp
