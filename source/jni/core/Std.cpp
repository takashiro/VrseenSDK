/************************************************************************************

Filename    :   OVR_Std.cpp
Content     :   Standard C function implementation
Created     :   September 19, 2012
Notes       :

Copyright   :   Copyright 2014 Oculus VR, LLC. All Rights reserved.

************************************************************************************/

#include "Std.h"
#include "Alg.h"

// localeconv() call in OVR_strtod()
#include <locale.h>

namespace NervGear {

// Source for functions not available on all platforms is included here.

// Case insensitive compare implemented in platform-specific way.


// This function is not inline because of dependency on <locale.h>

#endif //OVR_NO_WCTYPE

} // OVR
