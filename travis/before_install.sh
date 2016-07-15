#!/usr/bin/env sh

if [ `uname -m` = x86_64 ]; then
    wget http://dl.google.com/android/repository/android-ndk-r10e-linux-x86_64.zip -O ndk.zip
else
    wget http://dl.google.com/android/repository/android-ndk-r10e-linux-x86.zip -O ndk.zip
fi
unzip -q ndk.zip

export ANDROID_NDK_HOME=`pwd`/android-ndk-r10e
export PATH=${PATH}:${ANDROID_HOME}/tools:${ANDROID_HOME}/platform-tools:${ANDROID_NDK_HOME}

cd source
ndk-build -j16 APP_CFLAGS=-DNV_NAMESPACE=Vrseen

cd ../examples
for subdir in $(ls -p | grep '/'); do
    cd $subdir
    ndk-build -j16 APP_CFLAGS=-DNV_NAMESPACE=Vrseen
    cd ..
done

cd ../tests
ndk-build -j16 APP_CFLAGS=-DNV_NAMESPACE=Vrseen

cd ../source
ndk-build -B -j16

cd ../examples
for subdir in $(ls -p | grep '/'); do
    cd $subdir
    ndk-build -B -j16
    cd ..
done

cd ../tests
ndk-build -B -j16
cd ..

chmod +x ./gradlew
mv -f travis/build.gradle build.gradle
mv -f travis/source.gradle source/build.gradle
mv -f travis/tests.gradle tests/build.gradle
mv -f travis/PanoPhoto.gradle examples/PanoPhoto/build.gradle
mv -f travis/PanoVideo.gradle examples/PanoVideo/build.gradle
