LOCAL_PATH := $(call my-dir)

# jni is always prepended to this, unfortunately
NV_ROOT := ../../source/jni

include $(LOCAL_PATH)/$(NV_ROOT)/3rdparty/*.mk

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
    $(LOCAL_PATH)/$(NV_ROOT) \
	$(LOCAL_PATH)/$(NV_ROOT)/api \
	$(LOCAL_PATH)/$(NV_ROOT)/core \
	$(LOCAL_PATH)/$(NV_ROOT)/gui \
	$(LOCAL_PATH)/$(NV_ROOT)/io \
	$(LOCAL_PATH)/$(NV_ROOT)/media \
	$(LOCAL_PATH)/$(NV_ROOT)/scene

LOCAL_EXPORT_C_INCLUDES := $(LOCAL_C_INCLUDES)

addsource = $(addprefix $(1)/, $(notdir $(wildcard $(LOCAL_PATH)/$(1)/*.cpp)))

LOCAL_SRC_FILES := \
	$(call addsource,core) \
	$(call addsource,core/android) \
	$(call addsource,api) \
	$(call addsource,gui) \
	$(call addsource,io) \
	$(call addsource,media) \
	$(call addsource,scene) \
	App.cpp

LOCAL_CPPFLAGS += -std=c++0x

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
	nanovg

include $(BUILD_STATIC_LIBRARY)		# start building based on everything since CLEAR_VARS
