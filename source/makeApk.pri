ANDROID_PACKAGE_SOURCE_DIR = $$OUT_PWD/java
system(mkdir $$system_path($$ANDROID_PACKAGE_SOURCE_DIR))

ANDROID_SOURCE_DIRS = res src assets
for(dir, ANDROID_APP_DIRS) {
    for(file, ANDROID_SOURCE_DIRS) {
        system(mkdir $$system_path($$ANDROID_PACKAGE_SOURCE_DIR/$$file))
        system(xcopy /y /e $$system_path($$dir/$$file) $$system_path($$ANDROID_PACKAGE_SOURCE_DIR/$$file))
    }
}
system($$QMAKE_COPY $$system_path($$PWD/AndroidManifest.xml) $$system_path($$ANDROID_PACKAGE_SOURCE_DIR/AndroidManifest.xml))
