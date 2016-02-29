TEMPLATE = subdirs

SUBDIRS += VrLib NervGearPhoto NervGearVideo

VrLib.file = source/NervGear.pro

NervGearPhoto.subdir = examples/NervGearPhoto
NervGearPhoto.depends = VrLib

NervGearVideo.subdir = examples/NervGearVideo
NervGearVideo.depends = VrLib
