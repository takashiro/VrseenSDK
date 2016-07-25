TEMPLATE = app
CONFIG += console
CONFIG -= app_bundle
QT -= core
QT -= gui
TARGET = cinema

INCLUDEPATH += jni/

SOURCES += $$files(jni/*.cpp)
HEADERS += $$files(jni/*.h)

include(../../source/dynamicVrLib.pri)
