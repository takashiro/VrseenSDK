DEFINES += ANDROID_NDK

CONFIG += c++11

QMAKE_CFLAGS	+= -Werror			# error on warnings
QMAKE_CFLAGS	+= -Wall
QMAKE_CFLAGS	+= -Wextra
QMAKE_CFLAGS	+= -Wno-strict-aliasing		# TODO: need to rewrite some code
QMAKE_CFLAGS	+= -Wno-missing-field-initializers	# warns on this: SwipeAction	ret = {}
QMAKE_CFLAGS	+= -Wno-multichar	# used in internal Android headers:  DISPLAY_EVENT_VSYNC = 'vsyn',
QMAKE_CXXFLAGS  += -Werror
QMAKE_CXXFLAGS  += -Wno-type-limits
QMAKE_CXXFLAGS  += -Wno-invalid-offsetof
