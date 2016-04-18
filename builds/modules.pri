DEFINES += NDK_DEBUG=0

DEFINES += NV_NAMESPACE=NervGear

NV_ROOT = $$absolute_path($PWD/../../source/jni)
DESTDIR = $$absolute_path($PWD/../libs)
LIBS += -L$$DESTDIR

INCLUDEPATH += \
    $$NV_ROOT \
    $$NV_ROOT/api \
    $$NV_ROOT/core \
    $$NV_ROOT/gui \
    $$NV_ROOT/io \
    $$NV_ROOT/media \
    $$NV_ROOT/scene \
    $$NV_ROOT/3rdparty/minizip

# OpenGL ES 3.0
LIBS += -lGLESv3
# GL platform interface
LIBS += -lEGL
# native multimedia
LIBS += -lOpenMAXAL
# logging
LIBS += -llog
# native windows
LIBS += -landroid
# For minizip
LIBS += -lz
# audio
LIBS += -lOpenSLES

DEFINES += ANDROID_NDK

CONFIG += c++11

QMAKE_CFLAGS	+= -Werror			# error on warnings
QMAKE_CFLAGS	+= -Wall
QMAKE_CFLAGS	+= -Wextra
#QMAKE_CFLAGS	+= -Wlogical-op		# not part of -Wall or -Wextra
#QMAKE_CFLAGS	+= -Weffc++			# too many issues to fix for now
QMAKE_CFLAGS	+= -Wno-strict-aliasing		# TODO: need to rewrite some code
QMAKE_CFLAGS	+= -Wno-unused-parameter
QMAKE_CFLAGS	+= -Wno-missing-field-initializers	# warns on this: SwipeAction	ret = {}
QMAKE_CFLAGS	+= -Wno-multichar	# used in internal Android headers:  DISPLAY_EVENT_VSYNC = 'vsyn',
QMAKE_CXXFLAGS	+= -Wno-unused-parameter
QMAKE_CXXFLAGS	+= -Wno-missing-field-initializers	# warns on this: SwipeAction	ret = {}
QMAKE_CXXFLAGS  += -Wno-type-limits
QMAKE_CXXFLAGS  += -Wno-invalid-offsetof

QMAKE_LFLAGS += -Wl,--unresolved-symbols=ignore-all
