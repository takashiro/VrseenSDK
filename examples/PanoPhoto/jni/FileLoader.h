/************************************************************************************

Filename    :   FileLoader.h
Content     :
Created     :   August 13, 2014
Authors     :   John Carmack

Copyright   :   Copyright 2014 Oculus VR, LLC. All Rights reserved.

This source code is licensed under the BSD-style license found in the
LICENSE file in the Oculus360Photos/ directory. An additional grant
of patent rights can be found in the PATENTS file in the same directory.

************************************************************************************/

#include "VEventLoop.h"

NV_NAMESPACE_BEGIN

class App;
class Oculus360Photos;

void InitFileQueue( App * app, Oculus360Photos * photos);

extern VEventLoop		Queue1;

NV_NAMESPACE_END

