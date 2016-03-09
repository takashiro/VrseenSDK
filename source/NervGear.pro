TEMPLATE = lib
CONFIG += console
CONFIG -= app_bundle
CONFIG -= qt

CONFIG += nervgear_capture
CONFIG(debug, debug|release): DEFINES += OVR_DEBUG=1 NDK_DEBUG=1 OVR_BUILD_DEBUG

DEFINES += NV_NAMESPACE=NervGear

INCLUDEPATH += \
    jni \
    jni/api \
    jni/core \
    jni/gui \
    jni/scene

SOURCES += \
    jni/core/Alg.cpp \
    jni/core/Allocator.cpp \
    jni/core/Atomic.cpp \
    jni/core/VByteArray.cpp \
    jni/core/VChar.cpp \
    jni/core/VEvent.cpp \
    jni/core/File.cpp \
    jni/core/FileFILE.cpp \
    jni/core/VJson.cpp \
    jni/core/Log.cpp \
    jni/core/VLog.cpp \
    jni/core/Lockless.cpp \
    jni/core/VMath.cpp \
    jni/core/VPath.cpp \
    jni/core/RefCount.cpp \
    jni/core/VStandardPath.cpp \
    jni/core/Std.cpp \
    jni/core/VString.cpp \
    jni/core/SysFile.cpp \
    jni/core/System.cpp \
    jni/core/ThreadCommandQueue.cpp \
    jni/core/VThread.cpp \
    jni/core/VTimer.cpp \
    jni/core/BinaryFile.cpp \
    jni/core/MappedFile.cpp \
    jni/core/MemBuffer.cpp \
    jni/core/VMutex.cpp \
    jni/core/VWaitCondition.cpp \
    jni/core/android/GlUtils.cpp \
    jni/core/android/JniUtils.cpp \
    jni/core/android/LogUtils.cpp \
    jni/core/android/VOsBuild.cpp \
    jni/api/VrApi.cpp \
    jni/api/Vsync.cpp \
    jni/api/DirectRender.cpp \
    jni/api/HmdInfo.cpp \
    jni/api/HmdSensors.cpp \
    jni/api/Distortion.cpp \
    jni/api/SystemActivities.cpp \
    jni/api/TimeWarp.cpp \
    jni/api/TimeWarpProgs.cpp \
    jni/api/ImageServer.cpp \
    jni/api/LocalPreferences.cpp \
    jni/api/WarpGeometry.cpp \
    jni/api/WarpProgram.cpp \
    jni/api/sensor/DeviceHandle.cpp \
    jni/api/sensor/DeviceImpl.cpp \
    jni/api/sensor/LatencyTest.cpp \
    jni/api/sensor/LatencyTestDeviceImpl.cpp \
    jni/api/sensor/Profile.cpp \
    jni/api/sensor/SensorFilter.cpp \
    jni/api/sensor/SensorCalibration.cpp \
    jni/api/sensor/GyroTempCalibration.cpp \
    jni/api/sensor/SensorFusion.cpp \
    jni/api/sensor/SensorTimeFilter.cpp \
    jni/api/sensor/SensorDeviceImpl.cpp \
    jni/api/sensor/Android_DeviceManager.cpp \
    jni/api/sensor/Android_HIDDevice.cpp \
    jni/api/sensor/Android_HMDDevice.cpp \
    jni/api/sensor/Android_SensorDevice.cpp \
    jni/api/sensor/Android_PhoneSensors.cpp \
    jni/api/sensor/Common_HMDDevice.cpp \
    jni/api/sensor/Stereo.cpp \
    jni/gui/VRMenuComponent.cpp \
    jni/gui/VRMenuMgr.cpp \
    jni/gui/VRMenuObjectLocal.cpp \
    jni/gui/VRMenuEvent.cpp \
    jni/gui/VRMenuEventHandler.cpp \
    jni/gui/SoundLimiter.cpp \
    jni/gui/VRMenu.cpp \
    jni/gui/GuiSys.cpp \
    jni/gui/FolderBrowser.cpp \
    jni/gui/Fader.cpp \
    jni/gui/DefaultComponent.cpp \
    jni/gui/TextFade_Component.cpp \
    jni/gui/CollisionPrimitive.cpp \
    jni/gui/ActionComponents.cpp \
    jni/gui/AnimComponents.cpp \
    jni/gui/VolumePopup.cpp \
    jni/gui/ScrollManager.cpp \
    jni/gui/ScrollBarComponent.cpp \
    jni/gui/ProgressBarComponent.cpp \
    jni/gui/SwipeHintComponent.cpp \
    jni/gui/MetaDataManager.cpp \
    jni/gui/OutOfSpaceMenu.cpp \
    jni/scene/BitmapFont.cpp \
    jni/scene/EyeBuffers.cpp \
    jni/scene/EyePostRender.cpp \
    jni/scene/GazeCursor.cpp \
    jni/scene/GlGeometry.cpp \
    jni/scene/GlProgram.cpp \
    jni/scene/GlSetup.cpp \
    jni/scene/GlTexture.cpp \
    jni/scene/ImageData.cpp \
    jni/scene/ModelCollision.cpp \
    jni/scene/ModelFile.cpp \
    jni/scene/ModelRender.cpp \
    jni/scene/ModelTrace.cpp \
    jni/scene/ModelView.cpp \
    jni/scene/SurfaceTexture.cpp \
    jni/scene/SwipeView.cpp \
    jni/App.cpp \
    jni/AppRender.cpp \
    jni/Console.cpp \
    jni/DebugLines.cpp \
    jni/KeyState.cpp \
    jni/MessageQueue.cpp \
    jni/PackageFiles.cpp \
    jni/SoundManager.cpp \
    jni/VUserProfile.cpp \
    jni/TalkToJava.cpp \
    jni/VrCommon.cpp \
    jni/VrLocale.cpp \
    jni/core/VFile.cpp \
    jni/core/VFileFILE.cpp \
    jni/core/VSysFile.cpp \
    jni/core/VBuffer.cpp

HEADERS += \
    jni/core/android/GlUtils.h \
    jni/core/android/JniUtils.h \
    jni/core/android/LogUtils.h \
    jni/core/android/VOsBuild.h \
    jni/core/capture/Capture.h \
    jni/core/capture/Capture_Config.h \
    jni/core/capture/Capture_LegacyPackets.h \
    jni/core/capture/Capture_Packets.h \
    jni/core/capture/Capture_Types.h \
    jni/core/Alg.h \
    jni/core/Allocator.h \
    jni/core/Atomic.h \
    jni/core/Array.h \
    jni/core/VArray.h \
    jni/core/VByteArray.h \
    jni/core/VColor.h \
    jni/core/VChar.h \
    jni/core/ContainerAllocator.h \
    jni/core/Deque.h \
    jni/core/VEvent.h \
    jni/core/File.h \
    jni/core/FileFILE.h \
    jni/core/VBufferedFile.h \
    jni/core/VFlags.h \
    jni/core/VJson.h \
    jni/core/Log.h \
    jni/core/VLog.h \
    jni/core/Lockless.h \
    jni/core/VMath.h \
    jni/core/VPath.h \
    jni/core/RefCount.h \
    jni/core/VStandardPath.h \
    jni/core/Std.h \
    jni/core/VString.h \
    jni/core/String_FormatUtil.h \
    jni/core/String_PathUtil.h \
    jni/core/SysFile.h \
    jni/core/System.h \
    jni/core/ThreadCommandQueue.h \
    jni/core/VThread.h \
    jni/core/VTimer.h \
    jni/core/BinaryFile.h \
    jni/core/MappedFile.h \
    jni/core/MemBuffer.h \
    jni/core/VMutex.h \
    jni/core/Types.h \
    jni/core/List.h \
    jni/core/OVR.h \
    jni/core/OVRVersion.h \
    jni/core/VSharedPointer.h \
    jni/core/String_Utils.h \
    jni/core/StringHash.h \
    jni/core/TypesafeNumber.h \
    jni/core/VWaitCondition.h \
    jni/embedded/dependency_error_de.h \
    jni/embedded/dependency_error_en.h \
    jni/embedded/dependency_error_es.h \
    jni/embedded/dependency_error_fr.h \
    jni/embedded/dependency_error_it.h \
    jni/embedded/dependency_error_ja.h \
    jni/embedded/dependency_error_ko.h \
    jni/embedded/oculus_loading_indicator.h \
    jni/api/VrApi.h \
    jni/api/Vsync.h \
    jni/api/DirectRender.h \
    jni/api/HmdInfo.h \
    jni/api/HmdSensors.h \
    jni/api/Distortion.h \
    jni/api/SystemActivities.h \
    jni/api/TimeWarp.h \
    jni/api/TimeWarpProgs.h \
    jni/api/ImageServer.h \
    jni/api/LocalPreferences.h \
    jni/api/WarpGeometry.h \
    jni/api/WarpProgram.h \ \
    jni/api/TimeWarpLocal.h \
    jni/api/VrApi_Android.h \
    jni/api/VrApi_Helpers.h \
    jni/api/VrApi_local.h \
    jni/api/sensor/DeviceConstants.h \
    jni/api/sensor/DeviceHandle.h \
    jni/api/sensor/DeviceImpl.h \
    jni/api/sensor/LatencyTest.h \
    jni/api/sensor/LatencyTestDeviceImpl.h \
    jni/api/sensor/Profile.h \
    jni/api/sensor/SensorFilter.h \
    jni/api/sensor/SensorCalibration.h \
    jni/api/sensor/GyroTempCalibration.h \
    jni/api/sensor/SensorFusion.h \
    jni/api/sensor/SensorTimeFilter.h \
    jni/api/sensor/SensorDeviceImpl.h \
    jni/api/sensor/Android_DeviceManager.h \
    jni/api/sensor/Android_HIDDevice.h \
    jni/api/sensor/Android_HMDDevice.h \
    jni/api/sensor/Android_SensorDevice.h \
    jni/api/sensor/Android_PhoneSensors.h \
    jni/api/sensor/Stereo.h \
    jni/api/sensor/PhoneSensors.h \
    jni/api/sensor/Device.h \
    jni/api/sensor/DeviceMessages.h \
    jni/api/sensor/HIDDevice.h \
    jni/api/sensor/HIDDeviceBase.h \
    jni/api/sensor/HIDDeviceImpl.h \
    jni/gui/VRMenuComponent.h \
    jni/gui/VRMenuMgr.h \
    jni/gui/VRMenuObjectLocal.h \
    jni/gui/VRMenuEvent.h \
    jni/gui/VRMenuEventHandler.h \
    jni/gui/SoundLimiter.h \
    jni/gui/VRMenu.h \
    jni/gui/GuiSys.h \
    jni/gui/FolderBrowser.h \
    jni/gui/Fader.h \
    jni/gui/DefaultComponent.h \
    jni/gui/TextFade_Component.h \
    jni/gui/CollisionPrimitive.h \
    jni/gui/ActionComponents.h \
    jni/gui/AnimComponents.h \
    jni/gui/VolumePopup.h \
    jni/gui/VRMenuObject.h \
    jni/gui/ScrollManager.h \
    jni/gui/ScrollBarComponent.h \
    jni/gui/ProgressBarComponent.h \
    jni/gui/SwipeHintComponent.h \
    jni/gui/MetaDataManager.h \
    jni/gui/OutOfSpaceMenu.h \
    jni/gui/GuiSysLocal.h \
    jni/gui/ui_default.h \
    jni/scene/BitmapFont.h \
    jni/scene/EyeBuffers.h \
    jni/scene/EyePostRender.h \
    jni/scene/GazeCursor.h \
    jni/scene/GazeCursorLocal.h \
    jni/scene/GlGeometry.h \
    jni/scene/GlProgram.h \
    jni/scene/GlSetup.h \
    jni/scene/GlTexture.h \
    jni/scene/ImageData.h \
    jni/scene/ModelCollision.h \
    jni/scene/ModelFile.h \
    jni/scene/ModelRender.h \
    jni/scene/ModelTrace.h \
    jni/scene/ModelView.h \
    jni/scene/SurfaceTexture.h \
    jni/scene/SwipeView.h \
    jni/PackageFiles.h \
    jni/VrCommon.h \
    jni/MessageQueue.h \
    jni/TalkToJava.h \
    jni/KeyState.h \
    jni/App.h \
    jni/AppRender.h \
    jni/DebugLines.h \
    jni/SoundManager.h \
    jni/VUserProfile.h \
    jni/VrLocale.h \
    jni/Console.h \
    jni/vglobal.h \
    jni/AppLocal.h \
    jni/Input.h \
    jni/PointTracker.h \
    jni/UniversalMenu_Commands.h \
    jni/core/VFile.h \
    jni/core/VFileFlags.h \
    jni/core/VDelegatedFile.h \
    jni/core/VSysFile.h \
    jni/core/VUnopenedFile.h \
    jni/PointTracker.h
    jni/core/VBuffer.h


include(jni/3rdparty/minizip/minizip.pri)
include(jni/3rdparty/stb/stb.pri)

# NervGear::Capture support...
nervgear_capture{
    SOURCES += \
        jni/core/capture/Capture.cpp \
        jni/core/capture/Capture_AsyncStream.cpp \
        jni/core/capture/Capture_FileIO.cpp \
        jni/core/capture/Capture_GLES3.cpp \
        jni/core/capture/Capture_Socket.cpp \
        jni/core/capture/Capture_StandardSensors.cpp \
        jni/core/capture/Capture_Thread.cpp

    HEADERS += \
        jni/core/capture/Capture_AsyncStream.h \
        jni/core/capture/Capture_FileIO.h \
        jni/core/capture/Capture_Socket.h \
        jni/core/capture/Capture_StandardSensors.h \
        jni/core/capture/Capture_GLES3.h \
        jni/core/capture/Capture_Thread.h

    DEFINES += OVR_ENABLE_CAPTURE=1
}

# OpenGL ES 3.0
LIBS += -lGLESv3
# GL platform interface
LIBS += -lEGL
# native multimedia
LIBS += -lOpenMAXAL
# logging
LIBS += -llog
# native windows
LIBS += -landroid
# For minizip
LIBS += -lz
# audio
LIBS += -lOpenSLES

SOURCES += \
    jni/Integrations/Unity/UnityPlugin.cpp \
    jni/Integrations/Unity/MediaSurface.cpp \
    jni/Integrations/Unity/SensorPlugin.cpp \
    jni/Integrations/Unity/RenderingPlugin.cpp

HEADERS += \
    jni/Integrations/Unity/GlStateSave.h \
    jni/Integrations/Unity/MediaSurface.h

linux {
    CONFIG(staticlib) {
        QMAKE_POST_LINK = $$QMAKE_COPY $$system_path($$OUT_PWD/lib$${TARGET}.a) $$system_path($$PWD/libs)
    } else {
        QMAKE_POST_LINK = $$QMAKE_COPY $$system_path($$OUT_PWD/lib$${TARGET}.so) $$system_path($$PWD/libs)
    }
}

include(cflags.pri)
include(deployment.pri)
qtcAddDeployment()
