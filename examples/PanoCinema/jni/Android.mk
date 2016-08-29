LOCAL_PATH := $(call my-dir)

PROJECT_PATH := $(LOCAL_PATH)/../../../

include $(CLEAR_VARS)					# clean everything up to prepare for a module

include $(PROJECT_PATH)/source/import_vrlib.mk
include $(PROJECT_PATH)/source/cflags.mk

LOCAL_MODULE    := panocinema
LOCAL_SRC_FILES	:= $(wildcard *.cpp)

include $(BUILD_SHARED_LIBRARY)			# start building based on everything since CLEAR_VARS
