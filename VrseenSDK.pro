TEMPLATE = subdirs

SUBDIRS += \
    VrLibrary \
    PanoPhoto \
    PanoVideo

VrLibrary.file = source/VrLibrary.pro

PanoPhoto.subdir = examples/PanoPhoto
PanoPhoto.depends = VrLibrary

PanoVideo.subdir = examples/PanoVideo
PanoVideo.depends = VrLibrary

