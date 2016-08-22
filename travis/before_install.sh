#!/usr/bin/env sh

wget http://dl.google.com/android/repository/android-ndk-r12b-linux-x86_64.zip -O ndk.zip
unzip -q ndk.zip

export ANDROID_NDK_HOME=`pwd`/android-ndk-r12b
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
cd ..

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

echo "ndk.dir=${ANDROID_NDK_HOME}" >> local.properties
