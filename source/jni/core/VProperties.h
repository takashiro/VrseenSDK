/*
 * VProperties.h
 *
 *  Created on: 2016年5月17日
 *      Author: yangkai
 */
#pragma once
#include <string>
#include <fstream>
#include <map>
#include "vglobal.h"
using namespace std;
NV_NAMESPACE_BEGIN
class VProperties
{
private:
    map<string, string> data;
public:
    VProperties(string path);
    string operator [] (string key);
};
NV_NAMESPACE_END
