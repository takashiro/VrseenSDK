include(../modules.pri)
include(../../source/jni/3rdparty/stb/stb.pri)
include(../../source/jni/3rdparty/minizip/minizip.pri)

TARGET = nvio

SOURCES += $$NV_ROOT/io/*.cpp
