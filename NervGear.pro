TEMPLATE = subdirs

SUBDIRS += NervGear NervGearPhoto NervGearVideo

NervGear.file = source/NervGear.pro

NervGearPhoto.subdir = examples/NervGearPhoto
NervGearPhoto.depends = NervGear

NervGearVideo.subdir = examples/NervGearVideo
NervGearVideo.depends = NervGear
