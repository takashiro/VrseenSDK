#pragma once

#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>

#include "android/VOsBuild.h"
#include "android/LogUtils.h"
#include "VLog.h"
#include "vglobal.h"
#define __gl2_h_

#include <EGL/egl.h>
#include <EGL/eglext.h>
#ifdef __gl2_h_
#include <GLES3/gl3.h>
#include <GLES3/gl3ext.h>
static const int GL_ES_VERSION = 3;
#else
#include <GLES2/gl2.h>
static const int GL_ES_VERSION = 2;
#endif
#include <GLES2/gl2ext.h>


