TEMPLATE = subdirs

SUBDIRS += \
    PanoPhoto \
    PanoVideo \
    PanoCinema \
    VrLibrary

VrLibrary.file = source/VrLibrary.pro

PanoPhoto.subdir = examples/PanoPhoto
PanoPhoto.depends = VrLibrary

PanoVideo.subdir = examples/PanoVideo
PanoVideo.depends = VrLibrary

PanoCinema.subdir = examples/PanoCinema
PanoCinema.depends = VrLibrary

VRPlayer.subdir = examples/VRPlayer
VRPlayer.depends = VrLibrary
