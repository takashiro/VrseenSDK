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
using namespace std;
class VProperties
{
private:
    map<string, string> data;
public:
    VProperties(string path);
    string operator [] (string key);
};
