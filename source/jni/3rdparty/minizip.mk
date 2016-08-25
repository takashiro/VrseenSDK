include $(CLEAR_VARS)

LOCAL_MODULE := minizip

LOCAL_CFLAGS += -Wno-strict-aliasing
LOCAL_CFLAGS += -Wno-unused-parameter
LOCAL_CFLAGS += -Wno-missing-field-initializers

LOCAL_SRC_FILES := \
	3rdparty/minizip/ioapi.c \
	3rdparty/minizip/miniunz.c \
	3rdparty/minizip/mztools.c \
	3rdparty/minizip/unzip.c \
	3rdparty/minizip/zip.c

include $(BUILD_STATIC_LIBRARY)
