LOCAL_PATH := $(call my-dir)
PROJECT_PATH := $(LOCAL_PATH)/../../../

include $(CLEAR_VARS)

include $(PROJECT_PATH)/source/import_vrlib.mk
include $(PROJECT_PATH)/source/cflags.mk

LOCAL_ARM_MODE  := arm					# full speed arm instead of thumb
LOCAL_ARM_NEON  := true					# compile with neon support enabled

LOCAL_MODULE    := vrlauncher

LOCAL_SRC_FILES := VRLauncher.cpp FileLoader.cpp

include $(BUILD_SHARED_LIBRARY)			# start building based on everything since CLEAR_VARS
