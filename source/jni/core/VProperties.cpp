/*
 * VProperties.cpp
 *
 *  Created on: 2016年5月17日
 *      Author: yangkai
 */
#include <VProperties.h>
#include <cctype>
#include <iostream>
static bool isBlankLine(string line)
{
    for(auto e:line) {
        if(!isspace(e)) {
            return false;
        }
    }
    return true;
}
VProperties::VProperties(string path)
{
    ifstream ifs(path);
    if(!ifs) {
        cout << "failed" << endl;
        return;
    }
    string line;
    int pos;
    while(getline(ifs, line))
    {
        if(isBlankLine(line)) {
            continue;
        }
        if('#' == line[0]) {
            continue;
        }
        pos = line.find('=');
        if(string::npos == pos) {
            continue;
        }
        string key = line.substr(0, pos);
        string val = line.substr(pos+1);
        if(0 >= val.size()) {
            continue;
        }
        data[key] = val;
    }
}
string VProperties:: operator [] (string key)
{
    try {
        return data.at(key);
    }
    catch (const out_of_range& oor) {
        cerr << oor.what() << endl;
        return nullptr;
    }
}



