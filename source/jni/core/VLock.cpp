/*
 * VLock.cpp
 *
 *  Created on: 2016年3月21日
 *      Author: yangkai
 */

#include "VLock.h"

NV_NAMESPACE_BEGIN

pthread_mutexattr_t VLock::RecursiveAttr = 0;
bool VLock::RecursiveAttrInit;

NV_NAMESPACE_END


