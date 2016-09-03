# This file is included in all .mk files to ensure their compilation flags are in sync
# across debug and release builds.

# NOTE: this is not part of import_vrlib.mk because VRLib itself needs to have these flags
# set, but VRLib's make file cannot include import_vrlib.mk or it would be importing itself.

LOCAL_CFLAGS	:= -DANDROID_NDK
LOCAL_CFLAGS	+= -Werror			# error on warnings
LOCAL_CFLAGS    += -Wno-error=unused-parameter
LOCAL_CFLAGS	+= -Wall
LOCAL_CFLAGS	+= -Wextra
LOCAL_CFLAGS	+= -Wno-strict-aliasing
LOCAL_CFLAGS	+= -Wno-multichar	# used in internal Android headers:  DISPLAY_EVENT_VSYNC = 'vsyn',
LOCAL_CPPFLAGS := -Wno-type-limits -std=c++0x
LOCAL_CPPFLAGS += -Wno-invalid-offsetof
LOCAL_CPPFLAGS += -Wno-error=unused-parameter
ifeq ($(OVR_DEBUG),1)
  LOCAL_CFLAGS += -DOVR_BUILD_DEBUG=1 -O0 -g
else
  LOCAL_CFLAGS += -O3
endif
