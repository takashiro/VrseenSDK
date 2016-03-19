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
#path=D:/Oculus/android-ndk-r10e/
LOCAL_MODULE    := JNI_Tests
LOCAL_SRC_FILES := JNI_Tests.cpp \
				../../../source/jni/core/VAtomicInt.cpp
				
#LOCAL_C_INCLUDES := D:/Oculus/android-ndk-r10e/platforms/android-19/arch-arm/usr/include
#LOCAL_C_INCLUDES += D:/Oculus/android-ndk-r10e/sources/cxx-stl/gnu-libstdc++/4.8/include
#LOCAL_C_INCLUDES += D:/Oculus/android-ndk-r10e/sources/cxx-stl/gnu-libstdc++/4.8/include/backward
#LOCAL_C_INCLUDES += D:/Oculus/android-ndk-r10e/sources/cxx-stl/gnu-libstdc++/4.8/libs/armeabi-v7a/include
#LOCAL_C_INCLUDES += D:/Oculus/android-ndk-r10e/sources/cxx-stl/stlport/stlport
#LOCAL_C_INCLUDES += D:/Oculus/android-ndk-r10e/toolchains/arm-linux-androideabi-4.8/prebuilt/windows-x86_64/lib/gcc/arm-linux-androideabi/4.8/include
#LOCAL_C_INCLUDES += D:/Oculus/android-ndk-r10e/toolchains/arm-linux-androideabi-4.8/prebuilt/windows-x86_64/lib/gcc/arm-linux-androideabi/4.8/include-fixed
LOCAL_C_INCLUDES += $(LOCAL_PATH)/../../../source/jni/core
LOCAL_C_INCLUDES += $(LOCAL_PATH)/../../../source/jni
LOCAL_CFLAGS += -DNV_NAMESPACE=NervGear
LOCAL_CPPFLAGS += -std=c++11
# logging
LOCAL_LDLIBS += -llog

LOCAL_LDLIBS += -latomic
include $(BUILD_SHARED_LIBRARY)
