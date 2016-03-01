#pragma once

#define NV_MAJOR_VERSION 0
#define NV_MINOR_VERSION 0
#define NV_BUILD_VERSION 0
#define NV_PATCH_VERSION 0

#define NV_VERSION_CHECK(major, minor, build, patch) ((major<<24)|(minor<<16)|(build<<8)|patch)
#define NV_VERSION NV_VERSION_CHECK(NV_MAJOR_VERSION, NV_MINOR_VERSION, NV_BUILD_VERSION, NV_PATCH_VERSION)

//@to-do: remove this
#define NV_VERSION_STRING "0.0.0"
