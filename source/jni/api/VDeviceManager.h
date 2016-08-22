//
// Created by vrsuser004 on 2016/8/9.
//

#ifndef VRSEEN_VDEVICEMANAGER_H
#define VRSEEN_VDEVICEMANAGER_H

class VDeviceManager
{
public:
   enum VDevice_Type
    {
        VDevice_Type_Unknown = 0,
        VDevice_Type_Sumsung = 1,
        VDevice_Type_ZTE = 2,
        VDevice_Type_VIVO = 3,
    };

    static VDevice_Type getSupportedVRDeviceType();

};
#endif //VRSEEN_VDEVICEMANAGER_H
