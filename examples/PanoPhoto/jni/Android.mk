LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)					# clean everything up to prepare for a module
include ../../source/import_vrlib.mk		# import VRLib for this module.  Do NOT call $(CLEAR_VARS) until after building your module.
										# use += instead of := when defining the following variables: LOCAL_LDLIBS, LOCAL_CFLAGS, LOCAL_C_INCLUDES, LOCAL_STATIC_LIBRARIES

include ../../source/cflags.mk


LOCAL_ARM_MODE  := arm					# full speed arm instead of thumb
LOCAL_ARM_NEON  := true					# compile with neon support enabled

LOCAL_MODULE    := nervgearphoto		# generate oculus360photos.so

LOCAL_SRC_FILES := Oculus360Photos.cpp FileLoader.cpp PanoBrowser.cpp PanoMenu.cpp PhotosMetaData.cpp OVR_TurboJpeg.cpp
#	../../../source/jni/core/VDir.cpp

include $(BUILD_SHARED_LIBRARY)			# start building based on everything since CLEAR_VARS
