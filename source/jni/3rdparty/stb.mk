include $(CLEAR_VARS)

LOCAL_MODULE := stb

LOCAL_CFLAGS += -Wno-strict-aliasing
LOCAL_CFLAGS += -Wno-unused-parameter
LOCAL_CFLAGS += -Wno-missing-field-initializers

LOCAL_SRC_FILES := \
	3rdparty/stb/stb_image.c \
	3rdparty/stb/stb_image_write.c

include $(BUILD_STATIC_LIBRARY)
