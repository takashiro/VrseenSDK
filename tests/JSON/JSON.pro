TEMPLATE = app
CONFIG += console c++11
CONFIG -= app_bundle
CONFIG -= qt

SOURCES += main.cpp \
    ../../VRLib/jni/corelib/JSON.cpp

HEADERS += \
    ../../VRLib/jni/corelib/JSON.h \
    ../../VRLib/jni/corelib/SharedPointer.h

INCLUDEPATH += ../../VRLib/jni/corelib
