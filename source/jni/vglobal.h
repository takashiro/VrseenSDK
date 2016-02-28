#pragma once

#ifdef NV_NAMESPACE
# define NV_NAMESPACE_BEGIN namespace NV_NAMESPACE {
# define NV_NAMESPACE_END }
#else
# define NV_NAMESPACE_BEGIN
# define NV_NAMESPACE_END
#endif
