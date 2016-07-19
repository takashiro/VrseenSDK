TEMPLATE = app
CONFIG += console
CONFIG -= app_bundle
QT -= core
QT -= gui
TARGET = cinema

INCLUDEPATH += jni/

SOURCES += \
    jni/PanoCinema.cpp \
    jni/Native.cpp \
    jni/View.cpp \
    jni/SceneManager.cpp \
    jni/ViewManager.cpp \
    jni/ShaderManager.cpp \
    jni/ModelManager.cpp \
    jni/MovieManager.cpp \
    jni/MoviePlayerView.cpp \
    jni/MovieSelectionView.cpp \
    jni/TheaterSelectionView.cpp \
    jni/TheaterSelectionComponent.cpp \
    jni/CarouselBrowserComponent.cpp \
    jni/MovieCategoryComponent.cpp \
    jni/MoviePosterComponent.cpp \
    jni/MovieSelectionComponent.cpp \
    jni/ResumeMovieView.cpp \
    jni/ResumeMovieComponent.cpp \
    jni/SwipeHintComponent.cpp \
    jni/CinemaStrings.cpp \
    jni/UI/UITexture.cpp \
    jni/UI/UIMenu.cpp \
    jni/UI/UIWidget.cpp \
    jni/UI/UIContainer.cpp \
    jni/UI/UILabel.cpp \
    jni/UI/UIImage.cpp \
    jni/UI/UIButton.cpp

HEADERS += \
    jni/PanoCinema.h \
    jni/Native.h \
    jni/View.h \
    jni/SceneManager.h \
    jni/ViewManager.h \
    jni/ShaderManager.h \
    jni/ModelManager.h \
    jni/MovieManager.h \
    jni/MoviePlayerView.h \
    jni/MovieSelectionView.h \
    jni/TheaterSelectionView.h \
    jni/TheaterSelectionComponent.h \
    jni/CarouselBrowserComponent.h \
    jni/MovieCategoryComponent.h \
    jni/MoviePosterComponent.h \
    jni/MovieSelectionComponent.h \
    jni/ResumeMovieView.h \
    jni/ResumeMovieComponent.h \
    jni/SwipeHintComponent.h \
    jni/CinemaStrings.h \
    jni/UI/UITexture.h \
    jni/UI/UIMenu.h \
    jni/UI/UIWidget.h \
    jni/UI/UIContainer.h \
    jni/UI/UILabel.h \
    jni/UI/UIImage.h \
    jni/UI/UIButton.h

ANDROID_APP_DIR = $$PWD
include(../../source/makeApk.pri)

include(../../source/dynamicVrLib.pri)
