
TEMPLATE = lib
CONFIG += static
DESTDIR = $$absolute_path($$PWD/../)

INCLUDEPATH += $$PWD

SOURCES += \
    $$PWD/ioapi.c \
    $$PWD/miniunz.c \
    $$PWD/mztools.c \
    $$PWD/unzip.c \
    $$PWD/zip.c

HEADERS += \
    $$PWD/crypt.h \
    $$PWD/ioapi.h \
    $$PWD/iowin32.h \
    $$PWD/mztools.h \
    $$PWD/unzip.h \
    $$PWD/zip.h

QMAKE_CFLAGS += -Werror
QMAKE_CFLAGS += -Wall
QMAKE_CFLAGS += -Wextra
QMAKE_CFLAGS += -Wno-strict-aliasing
QMAKE_CFLAGS += -Wno-unused-parameter
QMAKE_CFLAGS += -Wno-missing-field-initializers
QMAKE_CFLAGS += -Wno-multichar

QMAKE_POST_LINK += $$QMAKE
