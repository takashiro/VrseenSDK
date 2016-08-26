include $(CLEAR_VARS)

LOCAL_MODULE := nanovg

LOCAL_CFLAGS += -Wno-all

LOCAL_SRC_FILES := 3rdparty/nanovg/nanovg.c

include $(BUILD_STATIC_LIBRARY)
