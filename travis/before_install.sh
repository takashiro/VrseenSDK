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
if [ $? -ne 0 ]; then exit 1; fi

cd ../examples
for subdir in $(ls -p | grep '/'); do
    cd $subdir
    ndk-build -j16 APP_CFLAGS=-DNV_NAMESPACE=Vrseen
    if [ $? -ne 0 ]; then exit 1; fi
    cd ..
done

cd ../tests
ndk-build -j16 APP_CFLAGS=-DNV_NAMESPACE=Vrseen
if [ $? -ne 0 ]; then exit 1; fi

cd ../source
ndk-build -B -j16
if [ $? -ne 0 ]; then exit 1; fi

cd ../examples
for subdir in $(ls -p | grep '/'); do
    cd $subdir
    ndk-build -B -j16
    if [ $? -ne 0 ]; then exit 1; fi
    cd ..
done

cd ../tests
ndk-build -B -j16
if [ $? -ne 0 ]; then exit 1; fi
cd ..

chmod +x ./gradlew

cp -f travis/build.gradle build.gradle
cp -f travis/source.gradle source/build.gradle
cp -f travis/tests.gradle tests/build.gradle
for source_file in $(ls travis/examples/*.gradle); do
    example_name=$(basename $source_file .gradle)
    cp -f $source_file examples/$example_name/build.gradle
done
