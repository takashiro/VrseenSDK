LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)					# clean everything up to prepare for a module

include ../../source/import_vrlib.mk		# import VRLib for this module.  Do NOT call $(CLEAR_VARS) until after building your module.
										# use += instead of := when defining the following variables: LOCAL_LDLIBS, LOCAL_CFLAGS, LOCAL_C_INCLUDES, LOCAL_STATIC_LIBRARIES

include ../../source/cflags.mk

LOCAL_MODULE    := panocinema
LOCAL_SRC_FILES	:= $(wildcard *.cpp)

include $(BUILD_SHARED_LIBRARY)			# start building based on everything since CLEAR_VARS
