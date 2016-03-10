LOCAL_PATH := $(call my-dir)

# jni is always prepended to this, unfortunately
NV_ROOT := ../../source/jni

include $(CLEAR_VARS)				# clean everything up to prepare for a module

APP_MODULE := nervgear

LOCAL_MODULE    := nervgear			# generate libnervgear.so

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
	$(LOCAL_PATH)/$(NV_ROOT)/scene

LOCAL_EXPORT_C_INCLUDES := $(LOCAL_C_INCLUDES)

LOCAL_SRC_FILES  := core/Alg.cpp \
                    core/Allocator.cpp \
                    core/Atomic.cpp \
                    core/VByteArray.cpp \
                    core/VChar.cpp \
                    core/VEvent.cpp \
                    core/File.cpp \
                    core/FileFILE.cpp \
                    core/VJson.cpp \
                    core/Log.cpp \
                    core/VLog.cpp \
                    core/Lockless.cpp \
                    core/VMath.cpp \
                    core/VPath.cpp \
                    core/RefCount.cpp \
                    core/VStandardPath.cpp \
                    core/Std.cpp \
                    core/VString.cpp \
                    core/SysFile.cpp \
                    core/System.cpp \
                    core/ThreadCommandQueue.cpp \
                    core/VThread.cpp \
                    core/VTimer.cpp \
                    core/BinaryFile.cpp \
                    core/MappedFile.cpp \
                    core/MemBuffer.cpp \
                    core/VMutex.cpp \
                    core/VWaitCondition.cpp \
                    core/android/GlUtils.cpp \
                    core/android/JniUtils.cpp \
                    core/android/LogUtils.cpp \
                    core/android/VOsBuild.cpp \
                    api/VrApi.cpp \
                    api/Vsync.cpp \
                    api/DirectRender.cpp \
                    api/HmdInfo.cpp \
                    api/HmdSensors.cpp \
                    api/Distortion.cpp \
                    api/SystemActivities.cpp \
                    api/TimeWarp.cpp \
                    api/TimeWarpProgs.cpp \
                    api/ImageServer.cpp \
                    api/LocalPreferences.cpp \
                    api/WarpGeometry.cpp \
                    api/WarpProgram.cpp \
                    api/sensor/DeviceHandle.cpp \
                    api/sensor/DeviceImpl.cpp \
                    api/sensor/LatencyTest.cpp \
                    api/sensor/LatencyTestDeviceImpl.cpp \
                    api/sensor/Profile.cpp \
                    api/sensor/SensorFilter.cpp \
                    api/sensor/SensorCalibration.cpp \
                    api/sensor/GyroTempCalibration.cpp \
                    api/sensor/SensorFusion.cpp \
                    api/sensor/SensorTimeFilter.cpp \
                    api/sensor/SensorDeviceImpl.cpp \
                    api/sensor/Android_DeviceManager.cpp \
                    api/sensor/Android_HIDDevice.cpp \
                    api/sensor/Android_HMDDevice.cpp \
                    api/sensor/Android_SensorDevice.cpp \
                    api/sensor/Android_PhoneSensors.cpp \
                    api/sensor/Common_HMDDevice.cpp \
                    api/sensor/Stereo.cpp \
                    gui/VRMenuComponent.cpp \
                    gui/VRMenuMgr.cpp \
                    gui/VRMenuObjectLocal.cpp \
                    gui/VRMenuEvent.cpp \
                    gui/VRMenuEventHandler.cpp \
                    gui/SoundLimiter.cpp \
                    gui/VRMenu.cpp \
                    gui/GuiSys.cpp \
                    gui/FolderBrowser.cpp \
                    gui/Fader.cpp \
                    gui/DefaultComponent.cpp \
                    gui/TextFade_Component.cpp \
                    gui/CollisionPrimitive.cpp \
                    gui/ActionComponents.cpp \
                    gui/AnimComponents.cpp \
                    gui/VolumePopup.cpp \
                    gui/ScrollManager.cpp \
                    gui/ScrollBarComponent.cpp \
                    gui/ProgressBarComponent.cpp \
                    gui/SwipeHintComponent.cpp \
                    gui/MetaDataManager.cpp \
                    gui/OutOfSpaceMenu.cpp \
                    io/VApkFile.cpp \
                    scene/BitmapFont.cpp \
                    scene/EyeBuffers.cpp \
                    scene/EyePostRender.cpp \
                    scene/GazeCursor.cpp \
                    scene/GlSetup.cpp \
                    scene/GlTexture.cpp \
                    scene/GlProgram.cpp \
                    scene/GlGeometry.cpp \
                    scene/ImageData.cpp \
                    scene/ModelRender.cpp \
                    scene/ModelFile.cpp \
                    scene/ModelCollision.cpp \
                    scene/ModelTrace.cpp \
                    scene/ModelView.cpp \
                    scene/SurfaceTexture.cpp \
                    scene/SwipeView.cpp \
                    VrCommon.cpp \
                    VMessageQueue.cpp \
                    TalkToJava.cpp \
                    KeyState.cpp \
                    App.cpp \
                    AppRender.cpp \
                    DebugLines.cpp \
                    SoundManager.cpp \
                    VUserProfile.cpp \
                    VrLocale.cpp \
                    Console.cpp


LOCAL_SRC_FILES +=	3rdParty/stb/stb_image.c \
					3rdParty/stb/stb_image_write.c

# minizip for loading ovrscene files
LOCAL_C_INCLUDES += $(LOCAL_PATH)/$(NV_ROOT)/3rdparty/minizip

LOCAL_SRC_FILES +=	3rdParty/minizip/ioapi.c \
					3rdParty/minizip/miniunz.c \
					3rdParty/minizip/mztools.c \
					3rdParty/minizip/unzip.c \
					3rdParty/minizip/zip.c

# OVR::Capture support...
LOCAL_C_INCLUDES  += $(LOCAL_PATH)/core/capture
LOCAL_SRC_FILES   += $(wildcard $(realpath $(LOCAL_PATH))/core/capture/*.cpp)
LOCAL_CFLAGS      += -DOVR_ENABLE_CAPTURE=1


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

include $(BUILD_STATIC_LIBRARY)		# start building based on everything since CLEAR_VARS

#--------------------------------------------------------
# Unity plugin
#--------------------------------------------------------
include $(CLEAR_VARS)

LOCAL_MODULE := UnityPlugin

LOCAL_STATIC_LIBRARIES := nervgear
#LOCAL_STATIC_LIBRARIES += android-ndk-profiler

LOCAL_CFLAGS += -DNV_NAMESPACE=NervGear

LOCAL_SRC_FILES  := $(NV_ROOT)/Integrations/Unity/UnityPlugin.cpp \
                    $(NV_ROOT)/Integrations/Unity/MediaSurface.cpp \
                    $(NV_ROOT)/Integrations/Unity/SensorPlugin.cpp \
                    $(NV_ROOT)/Integrations/Unity/RenderingPlugin.cpp

include $(BUILD_SHARED_LIBRARY)


#--------------------------------------------------------
# JavaVr.so
#
# This .so can be loaded by a java project that wants to
# do frame and eye rendering completely in java without
# needing the NDK.
#--------------------------------------------------------
include $(CLEAR_VARS)

LOCAL_MODULE := JavaVr

LOCAL_STATIC_LIBRARIES := nervgear
#LOCAL_STATIC_LIBRARIES += android-ndk-profiler

LOCAL_CFLAGS += -DNV_NAMESPACE=NervGear

LOCAL_SRC_FILES  := $(NV_ROOT)/Integrations/PureJava/PureJava.cpp

include $(BUILD_SHARED_LIBRARY)

#$(call import-module,android-ndk-profiler)
