LOCAL_PATH := $(call my-dir)

PROJECT_PATH := $(LOCAL_PATH)/../../

include $(CLEAR_VARS)

include $(PROJECT_PATH)/source/import_vrlib.mk
include $(PROJECT_PATH)/source/cflags.mk

LOCAL_MODULE    := unittest

addsource = $(addprefix $(1)/, $(notdir $(wildcard $(LOCAL_PATH)/$(1)/*.cpp)))

LOCAL_SRC_FILES := \
	$(call addsource,core) \
	$(call addsource,api) \
	$(call addsource,gui) \
	$(call addsource,io) \
	$(call addsource,media) \
	$(call addsource,scene) \
	main.cpp

NDK_MODULE_PATH := ../../Tools/
$(call import-add-path,$(NDK_MODULE_PATH))

include $(BUILD_SHARED_LIBRARY)
