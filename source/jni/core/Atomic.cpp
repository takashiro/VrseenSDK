#include "Atomic.h"

NV_NAMESPACE_BEGIN

pthread_mutexattr_t Lock::RecursiveAttr = 0;
bool Lock::RecursiveAttrInit;

NV_NAMESPACE_END
