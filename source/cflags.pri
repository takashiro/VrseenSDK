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
