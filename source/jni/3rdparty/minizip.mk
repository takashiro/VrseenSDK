include $(CLEAR_VARS)

MINIZIP_ROOT := 3rdparty/minizip

LOCAL_MODULE := minizip

LOCAL_CFLAGS += -Wno-strict-aliasing
LOCAL_CFLAGS += -Wno-unused-parameter
LOCAL_CFLAGS += -Wno-missing-field-initializers

LOCAL_SRC_FILES := \
	$(MINIZIP_ROOT)/ioapi.c \
	$(MINIZIP_ROOT)/miniunz.c \
	$(MINIZIP_ROOT)/mztools.c \
	$(MINIZIP_ROOT)/unzip.c \
	$(MINIZIP_ROOT)/zip.c

include $(BUILD_STATIC_LIBRARY)
