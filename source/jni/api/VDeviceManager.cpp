//
// Created by vrsuser004 on 2016/8/9.
//
#include "VDeviceManager.h"
#include "android/VOsBuild.h"

VDeviceManager::VDevice_Type VDeviceManager::getSupportedVRDeviceType() {
    if (VOsBuild::getString(VOsBuild::Model).contains("SM-N910") ||
        VOsBuild::getString(VOsBuild::Model).contains("SM-N916") ||
        VOsBuild::getString(VOsBuild::Model).contains("SM-N920") ||
        VOsBuild::getString(VOsBuild::Model).contains("SM-G920") ||
        VOsBuild::getString(VOsBuild::Model).contains("SM-G925") ||
        VOsBuild::getString(VOsBuild::Model).contains("SM-G928"))
    {
        return VDevice_Type_Sumsung;
    } else if (VOsBuild::getString(VOsBuild::Model).contains("ZTE A2017")) {
        return VDevice_Type_ZTE;
    }
    else if(VOsBuild::getString(VOsBuild::Model).contains("Xplay5S"))
    {
        return VDevice_Type_VIVO;
    }

    return VDevice_Type_Unknown;
}