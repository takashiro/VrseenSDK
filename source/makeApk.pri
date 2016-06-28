ANDROID_PACKAGE_SOURCE_DIR = $$OUT_PWD/java
#system(mkdir $$system_path($$ANDROID_PACKAGE_SOURCE_DIR))

ANDROID_LIB_DIRS += \
    $$PWD \
    $$ANDROID_APP_DIR

ANDROID_SOURCE_DIRS = res src assets
for(dir, ANDROID_LIB_DIRS) {
    for(file, ANDROID_SOURCE_DIRS) {
        #system(mkdir $$system_path($$ANDROID_PACKAGE_SOURCE_DIR/$$file))
        #system(xcopy /y /e $$system_path($$dir/$$file) $$system_path($$ANDROID_PACKAGE_SOURCE_DIR/$$file))
    }
}
#system($$QMAKE_COPY $$system_path($$ANDROID_APP_DIR/AndroidManifest.xml) $$system_path($$ANDROID_PACKAGE_SOURCE_DIR/AndroidManifest.xml))
