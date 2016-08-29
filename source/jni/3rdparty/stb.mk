include $(CLEAR_VARS)

STB_ROOT := 3rdparty/stb

LOCAL_MODULE := stb

LOCAL_CFLAGS += -Wno-strict-aliasing
LOCAL_CFLAGS += -Wno-unused-parameter
LOCAL_CFLAGS += -Wno-missing-field-initializers

LOCAL_SRC_FILES := \
	$(STB_ROOT)/stb_image.c \
	$(STB_ROOT)/stb_image_write.c

include $(BUILD_STATIC_LIBRARY)
