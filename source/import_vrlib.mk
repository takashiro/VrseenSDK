#
# import_vrlib.mk
#
# Common settings used by all VRLib based projects.  Import at the start of any module using VRLib.
#
# Use the LOCAL_EXPORT_ variants for the oculus prebuilt library instead of
# LOCAL_. This works for the following variables:
# 	LOCAL_EXPORT_LDLIBS
# 	LOCAL_EXPORT_CFLAGS
# 	LOCAL_EXPORT_C_INCLUDES
#
# See the NDK documentation for more details.

# save off the local path
LOCAL_PATH_TEMP := $(LOCAL_PATH)
LOCAL_PATH := $(call my-dir)

# VrseenSDK dependencies

include $(CLEAR_VARS)

LOCAL_MODULE := minizip
LOCAL_SRC_FILES := obj/local/armeabi-v7a/libminizip.a

include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)

LOCAL_MODULE := stb
LOCAL_SRC_FILES := obj/local/armeabi-v7a/libstb.a

include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)

LOCAL_MODULE := nanovg
LOCAL_SRC_FILES := obj/local/armeabi-v7a/libnanovg.a

include $(PREBUILT_STATIC_LIBRARY)

# VrseenSDK
include $(CLEAR_VARS)

LOCAL_MODULE := vrseen

LOCAL_EXPORT_C_INCLUDES := \
  $(LOCAL_PATH)/jni \
  $(LOCAL_PATH)/jni/api \
  $(LOCAL_PATH)/jni/core \
  $(LOCAL_PATH)/jni/gui \
  $(LOCAL_PATH)/jni/io \
  $(LOCAL_PATH)/jni/media \
  $(LOCAL_PATH)/jni/scene \
  $(LOCAL_PATH)/jni/3rdParty/TinyXml \
  $(LOCAL_PATH)/jni/3rdParty/minizip

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

LOCAL_SRC_FILES := obj/local/armeabi-v7a/libvrseen.a

LOCAL_STATIC_LIBRARIES := \
	jpeg \
	minizip \
	stb \
	nanovg

include $(PREBUILT_STATIC_LIBRARY)


include $(CLEAR_VARS)

LOCAL_ARM_MODE  := arm				# full speed arm instead of thumb

LOCAL_STATIC_LIBRARIES := vrseen

# Restore the local path since we overwrote it when we started
LOCAL_PATH := $(LOCAL_PATH_TEMP)
