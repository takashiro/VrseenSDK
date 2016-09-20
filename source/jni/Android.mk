LOCAL_PATH := $(call my-dir)

include $(LOCAL_PATH)/3rdparty/*.mk

include $(CLEAR_VARS)				# clean everything up to prepare for a module

APP_MODULE := vrseen

LOCAL_MODULE := vrseen

LOCAL_ARM_MODE  := arm				# full speed arm instead of thumb
LOCAL_ARM_NEON  := true				# compile with neon support enabled

# Applications will need to link against these libraries, we
# can't link them into a static VrLib ourselves.
#
# LOCAL_LDLIBS	+= -lGLESv3			# OpenGL ES 3.0
# LOCAL_LDLIBS	+= -lEGL			# GL platform interface
# LOCAL_LDLIBS	+= -llog			# logging
# LOCAL_LDLIBS	+= -landroid		# native windows

include $(LOCAL_PATH)/../cflags.mk

LOCAL_C_INCLUDES :=  \
    $(LOCAL_PATH) \
	$(LOCAL_PATH)/api \
	$(LOCAL_PATH)/core \
	$(LOCAL_PATH)/gui \
	$(LOCAL_PATH)/io \
	$(LOCAL_PATH)/media \
	$(LOCAL_PATH)/scene

LOCAL_EXPORT_C_INCLUDES := $(LOCAL_C_INCLUDES)

addsource = $(wildcard $(LOCAL_PATH)/$(1)/*.cpp)

FILE_LIST := \
	$(call addsource,core) \
	$(call addsource,core/android) \
	$(call addsource,api) \
	$(call addsource,gui) \
	$(call addsource,io) \
	$(call addsource,media) \
	$(call addsource,scene) \
	$(LOCAL_PATH)/App.cpp

LOCAL_SRC_FILES := $(FILE_LIST:$(LOCAL_PATH)/%=%)

# OpenGL ES 3.0
LOCAL_EXPORT_LDLIBS := -lGLESv3
# GL platform interface
LOCAL_EXPORT_LDLIBS += -lEGL
# native multimedia
LOCAL_EXPORT_LDLIBS += -lOpenMAXAL
# logging
LOCAL_EXPORT_LDLIBS += -llog
# native windows
LOCAL_EXPORT_LDLIBS += -landroid
# For minizip
LOCAL_EXPORT_LDLIBS += -lz
# audio
LOCAL_EXPORT_LDLIBS += -lOpenSLES

LOCAL_STATIC_LIBRARIES := \
	minizip \
	stb \
	nanovg \
	assimp \
	freetype2

include $(BUILD_STATIC_LIBRARY)		# start building based on everything since CLEAR_VARS
