TEMPLATE = lib
CONFIG += console
CONFIG -= app_bundle
CONFIG -= qt

CONFIG(debug, debug|release): DEFINES += OVR_DEBUG=1 NDK_DEBUG=1 OVR_BUILD_DEBUG

DEFINES += NV_NAMESPACE=NervGear

INCLUDEPATH += \
    jni \
    jni/api \
    jni/core \
    jni/gui \
    jni/io \
    jni/media \
    jni/scene

SOURCES += \
    jni/core/VConstants.cpp \
    jni/core/Allocator.cpp \
    jni/core/VAtomicInt.cpp \
    jni/core/VByteArray.cpp \
    jni/core/VChar.cpp \
    jni/core/VDir.cpp \
    jni/core/VEventLoop.cpp \
    jni/core/VJson.cpp \
    jni/core/Log.cpp \
    jni/core/VLog.cpp \
    jni/core/VLock.cpp \
    jni/core/Lockless.cpp \
    jni/core/VPath.cpp \
    jni/core/RefCount.cpp \
    jni/core/VSignal.cpp \
    jni/core/VStandardPath.cpp \
    jni/core/VString.cpp \
    jni/core/System.cpp \
    jni/core/VThread.cpp \
    jni/core/VTimer.cpp \
    jni/core/MappedFile.cpp \
    jni/core/MemBuffer.cpp \
    jni/core/VMutex.cpp \
    jni/core/VUserSettings.cpp \
    jni/core/VVariant.cpp \
    jni/core/VWaitCondition.cpp \
    jni/core/android/JniUtils.cpp \
    jni/core/android/LogUtils.cpp \
    jni/core/android/VOsBuild.cpp \
    jni/api/VrApi.cpp \
    jni/api/Vsync.cpp \
    jni/api/VDevice.cpp \
    jni/api/VGlOperation.cpp \
    jni/api/HmdSensors.cpp \
    jni/api/VLensDistortion.cpp \
    jni/api/SystemActivities.cpp \
    jni/api/VFrameSmooth.cpp \
    jni/api/VMainActivity.cpp \
    jni/api/VGlShader.cpp \
    jni/api/VGlGeometry.cpp \
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
    jni/api/sensor/ThreadCommandQueue.cpp \
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
    jni/gui/KeyState.cpp \
    jni/io/VApkFile.cpp \
    jni/io/VBinaryFile.cpp \
    jni/io/VFileOperation.cpp \
    jni/io/VSysFile.cpp \
    jni/media/VSoundManager.cpp \
    jni/scene/BitmapFont.cpp \
    jni/scene/DebugLines.cpp \
    jni/scene/EyeBuffers.cpp \
    jni/scene/EyePostRender.cpp \
    jni/scene/GazeCursor.cpp \
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
    jni/VConsole.cpp \
    jni/VrLocale.cpp


HEADERS += \
    jni/core/VBasicmath.h \
    jni/core/VConstants.h \
    jni/core/VVector.h \
    jni/core/VMatrix.h \
    jni/core/VGeometry.h \
    jni/core/VTransform.h \
    jni/core/android/GlUtils.h \
    jni/core/android/JniUtils.h \
    jni/core/android/LogUtils.h \
    jni/core/android/VOsBuild.h \
    jni/core/VAlgorithm.h \
    jni/core/Allocator.h \
    jni/core/Atomic.h \
    jni/core/Array.h \
    jni/core/VByteArray.h \
    jni/core/VColor.h \
    jni/core/VChar.h \
    jni/core/VCircularQueue.h \
    jni/core/VDeque.h \
    jni/core/VDir.h \
    jni/core/VEvent.h \
    jni/core/VEventLoop.h \
    jni/core/VFlags.h \
    jni/core/VJson.h \
    jni/core/VList.h \
    jni/core/Log.h \
    jni/core/VLog.h \
    jni/core/VLock.h \
    jni/core/Lockless.h \
    jni/core/VMap.h \
    jni/core/VPath.h \
    jni/core/RefCount.h \
    jni/core/VSignal.h \
    jni/core/VStandardPath.h \
    jni/core/VString.h \
    jni/core/VStringHash.h \
    jni/core/String_FormatUtil.h \
    jni/core/String_PathUtil.h \
    jni/core/System.h \
    jni/core/VThread.h \
    jni/core/VTimer.h \
    jni/core/MappedFile.h \
    jni/core/MemBuffer.h \
    jni/core/VMutex.h \
    jni/core/Types.h \
    jni/core/List.h \
    jni/core/VVariant.h \
    jni/core/TypesafeNumber.h \
    jni/core/VUserSettings.h \
    jni/core/VWaitCondition.h \
    jni/core/VConstants.h \
    jni/core/VGeometry.h \
    jni/core/VMatrix.h \
    jni/core/VVector.h \
    jni/core/VTransform.h \
    jni/core/VBasicmath.h \
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
    jni/api/HmdSensors.h \
    jni/api/VLensDistortion.h \
    jni/api/SystemActivities.h \
    jni/api/VFrameSmooth.h \
    jni/api/VGlGeometry.h \
    jni/api/VGlShader.h \
    jni/api/VMainActivity.h \
    jni/api/VrApi_local.h \
    jni/api/VDevice.h \
    jni/api/VGlOperation.h \
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
    jni/api/sensor/PhoneSensors.h \
    jni/api/sensor/Device.h \
    jni/api/sensor/DeviceMessages.h \
    jni/api/sensor/HIDDevice.h \
    jni/api/sensor/HIDDeviceBase.h \
    jni/api/sensor/HIDDeviceImpl.h \
    jni/api/sensor/ThreadCommandQueue.h \
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
    jni/gui/KeyState.h \
    jni/io/VApkFile.h \
    jni/io/VBinaryFile.h \
    jni/io/VDelegatedFile.h \
    jni/io/VFile.h \
    jni/io/VFileOperation.h \
    jni/io/VSysFile.h \
    jni/media/VSoundManager.h \
    jni/scene/BitmapFont.h \
    jni/scene/DebugLines.h \
    jni/scene/EyeBuffers.h \
    jni/scene/EyePostRender.h \
    jni/scene/GazeCursor.h \
    jni/scene/GazeCursorLocal.h \
    jni/scene/GlTexture.h \
    jni/scene/ImageData.h \
    jni/scene/ModelCollision.h \
    jni/scene/ModelFile.h \
    jni/scene/ModelRender.h \
    jni/scene/ModelTrace.h \
    jni/scene/ModelView.h \
    jni/scene/SurfaceTexture.h \
    jni/scene/SwipeView.h \
    jni/App.h \
    jni/VrLocale.h \
    jni/VConsole.h \
    jni/vglobal.h \
    jni/Input.h

include(jni/3rdparty/minizip/minizip.pri)
include(jni/3rdparty/stb/stb.pri)

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
