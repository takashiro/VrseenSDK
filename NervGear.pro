TEMPLATE = subdirs

SUBDIRS += VrLib NervGearVideo

VrLib.file = source/NervGear.pro

NervGearVideo.subdir = examples/NervGearVideo
NervGearVideo.depends = VrLib
