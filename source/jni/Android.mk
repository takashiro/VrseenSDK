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
	$(LOCAL_PATH)/$(NV_ROOT)/media \
	$(LOCAL_PATH)/$(NV_ROOT)/scene

LOCAL_EXPORT_C_INCLUDES := $(LOCAL_C_INCLUDES)

LOCAL_SRC_FILES  := \
	core/VConstants.cpp \
	core/Allocator.cpp \
	core/VAtomicInt.cpp \
	core/VByteArray.cpp \
	core/VChar.cpp \
	core/VEventLoop.cpp \
	core/VJson.cpp \
	core/Log.cpp \
	core/VLog.cpp \
	core/Lockless.cpp \
	core/VMath.cpp \
	core/VPath.cpp \
	core/RefCount.cpp \
	core/VStandardPath.cpp \
	core/VString.cpp \
	core/VDir.cpp \
	core/VSignal.cpp \
	core/System.cpp \
	core/VLock.cpp \
	core/VThread.cpp \
	core/VTimer.cpp \
	core/MappedFile.cpp \
	core/MemBuffer.cpp \
	core/VMutex.cpp \
	core/VUserSettings.cpp \
	core/VVariant.cpp \
	core/VWaitCondition.cpp \
	core/android/JniUtils.cpp \
	core/android/LogUtils.cpp \
	core/android/VOsBuild.cpp \
	api/VKernel.cpp \
	api/Vsync.cpp \
	api/VDevice.cpp \
	api/HmdSensors.cpp \
	api/VLensDistortion.cpp \
	api/SystemActivities.cpp \
	api/VFrameSmooth.cpp \
	api/VGlGeometry.cpp \
	api/VMainActivity.cpp \
	api/VGlOperation.cpp \
	api/VGlShader.cpp \
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
	api/sensor/ThreadCommandQueue.cpp \
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
	gui/KeyState.cpp \
	io/VApkFile.cpp \
	io/VBinaryFile.cpp \
	io/VFileOperation.cpp \
	io/VSysFile.cpp \
	media/VSoundManager.cpp \
	scene/BitmapFont.cpp \
	scene/DebugLines.cpp \
	scene/EyeBuffers.cpp \
	scene/EyePostRender.cpp \
	scene/GazeCursor.cpp \
	scene/GlTexture.cpp \
	scene/ImageData.cpp \
	scene/ModelRender.cpp \
	scene/ModelFile.cpp \
	scene/ModelCollision.cpp \
	scene/ModelTrace.cpp \
	scene/ModelView.cpp \
	scene/SurfaceTexture.cpp \
	scene/SwipeView.cpp \
	App.cpp \
	VrLocale.cpp \
	VConsole.cpp


LOCAL_SRC_FILES += \
	3rdParty/stb/stb_image.c \
	3rdParty/stb/stb_image_write.c
# minizip for loading ovrscene files
LOCAL_C_INCLUDES += $(LOCAL_PATH)/$(NV_ROOT)/3rdparty/minizip

LOCAL_SRC_FILES += \
	3rdParty/minizip/ioapi.c \
	3rdParty/minizip/miniunz.c \
	3rdParty/minizip/mztools.c \
	3rdParty/minizip/unzip.c \
	3rdParty/minizip/zip.c

LOCAL_CPPFLAGS += -std=c++0x

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
