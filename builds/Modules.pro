TEMPLATE = subdirs

SUBDIRS += \
    api \
    core \
    gui \
    io \
    media \
    scene \
    app

core.file = module/core.pro

api.file = module/api.pro
api.depends = core

gui.file = module/gui.pro
io.file = module/io.pro
media.file = module/media.pro
scene.file = module/scene.pro

app.file = module/app.pro
app.depends = api core gui io media scene
