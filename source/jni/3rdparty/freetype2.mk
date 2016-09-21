include $(CLEAR_VARS)

LOCAL_MODULE := freetype2

FREETYPE_ROOT := $(LOCAL_PATH)/3rdparty/freetype2

LOCAL_CFLAGS := -DANDROID_NDK \
		-DFT2_BUILD_LIBRARY=1

LOCAL_C_INCLUDES := \
		$(FREETYPE_ROOT)/include \
		$(FREETYPE_ROOT)/src

addsource = $(wildcard $(FREETYPE_ROOT)/$(1))

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
	$(FREETYPE_ROOT)/src/cid/type1cid.c \
	$(FREETYPE_ROOT)/src/pshinter/pshinter.c \
	$(FREETYPE_ROOT)/src/psnames/psnames.c \
	$(FREETYPE_ROOT)/src/raster/raster.c \
	$(FREETYPE_ROOT)/src/sfnt/sfnt.c \
	$(FREETYPE_ROOT)/src/smooth/smooth.c \
	$(FREETYPE_ROOT)/src/truetype/truetype.c \
	$(FREETYPE_ROOT)/src/gzip/ftgzip.c \
	$(FREETYPE_ROOT)/src/type1/type1.c \
	$(FREETYPE_ROOT)/src/type42/type42.c \
	$(FREETYPE_ROOT)/src/pcf/pcf.c \
	$(FREETYPE_ROOT)/src/pfr/pfr.c \
	$(FREETYPE_ROOT)/src/winfonts/winfnt.c \
	$(FREETYPE_ROOT)/src/lzw/ftlzw.c \
	$(FREETYPE_ROOT)/src/lzw/ftzopen.c \
	$(FREETYPE_ROOT)/src/bdf/bdf.c \
	$(FREETYPE_ROOT)/src/bdf/bdfdrivr.c \
	$(FREETYPE_ROOT)/src/psaux/psaux.c

LOCAL_SRC_FILES := $(FILE_LIST:$(LOCAL_PATH)/%=%)

LOCAL_EXPORT_C_INCLUDES := $(FREETYPE_ROOT)/include

include $(BUILD_STATIC_LIBRARY)
