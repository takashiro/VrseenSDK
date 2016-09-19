include $(CLEAR_VARS)

LOCAL_MODULE := freetype2

FREETYPE_ROOT := $(LOCAL_PATH)/3rdparty/freetype2

LOCAL_CFLAGS := -DANDROID_NDK \
		-DFT2_BUILD_LIBRARY=1

LOCAL_C_INCLUDES := \
		$(FREETYPE_ROOT)/include \
		$(FREETYPE_ROOT)/src

FILE_LIST := \
	$(FREETYPE_ROOT)/src/autofit/autofit.c \
	$(FREETYPE_ROOT)/src/base/basepic.c \
	$(FREETYPE_ROOT)/src/base/ftapi.c \
	$(FREETYPE_ROOT)/src/base/ftbase.c \
	$(FREETYPE_ROOT)/src/base/ftbbox.c \
	$(FREETYPE_ROOT)/src/base/ftbitmap.c \
	$(FREETYPE_ROOT)/src/base/ftdbgmem.c \
	$(FREETYPE_ROOT)/src/base/ftdebug.c \
	$(FREETYPE_ROOT)/src/base/ftglyph.c \
	$(FREETYPE_ROOT)/src/base/ftinit.c \
	$(FREETYPE_ROOT)/src/base/ftpic.c \
	$(FREETYPE_ROOT)/src/base/ftstroke.c \
	$(FREETYPE_ROOT)/src/base/ftsynth.c \
	$(FREETYPE_ROOT)/src/base/ftsystem.c \
	$(FREETYPE_ROOT)/src/cff/cff.c \
	$(FREETYPE_ROOT)/src/pshinter/pshinter.c \
	$(FREETYPE_ROOT)/src/psnames/psnames.c \
	$(FREETYPE_ROOT)/src/raster/raster.c \
	$(FREETYPE_ROOT)/src/sfnt/sfnt.c \
	$(FREETYPE_ROOT)/src/smooth/smooth.c \
	$(FREETYPE_ROOT)/src/truetype/truetype.c

LOCAL_SRC_FILES := $(FILE_LIST:$(LOCAL_PATH)/%=%)

include $(BUILD_STATIC_LIBRARY)
