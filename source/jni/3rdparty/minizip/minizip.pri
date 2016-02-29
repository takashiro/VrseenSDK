
INCLUDEPATH += $$PWD

SOURCES += \
    jni/3rdparty/minizip/ioapi.c \
    jni/3rdparty/minizip/miniunz.c \
    jni/3rdparty/minizip/mztools.c \
    jni/3rdparty/minizip/unzip.c \
    jni/3rdparty/minizip/zip.c

HEADERS += \
    jni/3rdparty/minizip/crypt.h \
    jni/3rdparty/minizip/ioapi.h \
    jni/3rdparty/minizip/iowin32.h \
    jni/3rdparty/minizip/mztools.h \
    jni/3rdparty/minizip/unzip.h \
    jni/3rdparty/minizip/zip.h
