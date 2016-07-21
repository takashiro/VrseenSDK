
TEMPLATE = lib
CONFIG += static
DESTDIR = $$absolute_path($$PWD/../)

SOURCES += \
    $$PWD/stb_image.c \
    $$PWD/stb_image_write.c

HEADERS += \
    $$PWD/stb_image.h \
    $$PWD/stb_image_write.h

QMAKE_CFLAGS += -Werror
QMAKE_CFLAGS += -Wall
QMAKE_CFLAGS += -Wextra
QMAKE_CFLAGS += -Wno-strict-aliasing
QMAKE_CFLAGS += -Wno-unused-parameter
QMAKE_CFLAGS += -Wno-missing-field-initializers
QMAKE_CFLAGS += -Wno-multichar
