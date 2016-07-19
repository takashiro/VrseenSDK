# Copyright (C) 2009 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

include ../source/import_vrlib.mk
include ../source/cflags.mk

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
