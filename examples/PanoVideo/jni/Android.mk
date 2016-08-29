LOCAL_PATH := $(call my-dir)

PROJECT_PATH := $(LOCAL_PATH)/../../../

include $(CLEAR_VARS)

include $(PROJECT_PATH)/source/import_vrlib.mk
include $(PROJECT_PATH)/source/cflags.mk

LOCAL_MODULE := panovideo
LOCAL_SRC_FILES := PanoVideo.cpp

include $(BUILD_SHARED_LIBRARY)