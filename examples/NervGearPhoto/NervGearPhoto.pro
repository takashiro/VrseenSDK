TEMPLATE = app
CONFIG += console
CONFIG -= app_bundle
QT -= core
QT -= gui
TARGET = nervgearphoto

CONFIG(debug, debug|release): DEFINES += OVR_DEBUG=1 NDK_DEBUG=1

SOURCES += \
    jni/Oculus360Photos.cpp \
    jni/FileLoader.cpp \
    jni/PanoBrowser.cpp \
    jni/PanoMenu.cpp \
    jni/PhotosMetaData.cpp \
    jni/OVR_TurboJpeg.cpp

HEADERS += \
    jni/Oculus360Photos.h \
    jni/FileLoader.h \
    jni/PanoBrowser.h \
    jni/PanoMenu.h \
    jni/PhotosMetaData.h \
    jni/OVR_TurboJpeg.h

LIBS += -L"$$PWD/jni/" -ljpeg

ANDROID_PACKAGE_SOURCE_DIR = $$PWD/java
system(mkdir $$system_path($$ANDROID_PACKAGE_SOURCE_DIR))

ANDROID_APP_DIRS = ../../source $$PWD
ANDROID_SOURCE_DIRS = res src assets
for(dir, ANDROID_APP_DIRS) {
    for(file, ANDROID_SOURCE_DIRS) {
        system(mkdir $$system_path($$ANDROID_PACKAGE_SOURCE_DIR/$$file))
        system(xcopy /y /e $$system_path($$dir/$$file) $$system_path($$ANDROID_PACKAGE_SOURCE_DIR/$$file))
    }
}
system($$QMAKE_COPY $$system_path($$PWD/AndroidManifest.xml) $$system_path($$ANDROID_PACKAGE_SOURCE_DIR/AndroidManifest.xml))

include(../../source/dynamicVrLib.pri)
