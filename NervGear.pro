TEMPLATE = subdirs

SUBDIRS += NervGear \
    NervGearCinema \
    NervGearPhoto \
    NervGearScene \
    NervGearVideo \
    NervGearTemplate

NervGear.file = source/NervGear.pro

NervGearCinema.subdir = examples/NervGearCinema
NervGearCinema.depends = NervGear


NervGearPhoto.subdir = examples/NervGearPhoto
NervGearPhoto.depends = NervGear

NervGearScene.subdir = examples/NervGearScene
NervGearScene.depends = NervGear

NervGearVideo.subdir = examples/NervGearVideo
NervGearVideo.depends = NervGear

NervGearTemplate.subdir = examples/NervGearTemplate
NervGearTemplate.depends = NervGear
