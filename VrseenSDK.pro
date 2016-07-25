TEMPLATE = subdirs

SUBDIRS += \
    PanoPhoto \
    PanoVideo \
    PanoCinema \
    VrLibrary \
    minizip \
    stb

minizip.file = source/jni/3rdparty/minizip/minizip.pro
stb.file = source/jni/3rdparty/stb/stb.pro

VrLibrary.file = source/VrLibrary.pro
VrLibrary.depends = minizip stb

PanoPhoto.subdir = examples/PanoPhoto
PanoPhoto.depends = VrLibrary

PanoVideo.subdir = examples/PanoVideo
PanoVideo.depends = VrLibrary

PanoCinema.subdir = examples/PanoCinema
PanoCinema.depends = VrLibrary

VRPlayer.subdir = examples/VRPlayer
VRPlayer.depends = VrLibrary
