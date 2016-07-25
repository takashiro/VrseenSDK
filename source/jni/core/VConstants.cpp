#include "VConstants.h"

#include <float.h>

NV_NAMESPACE_BEGIN

// Single-precision VConstants constants class.
const float VConstants<float>::Pi = 3.1415926f;

const float VConstants<float>::E = 2.7182818f;

const float VConstants<float>::MaxValue = FLT_MAX;
const float VConstants<float>::MinPositiveValue = FLT_MIN;

const float VConstants<float>::VRTD = 360.0f / VConstants<float>::Pi*2.0;
const float VConstants<float>::VDTR = VConstants<float>::Pi*2.0 / 360.0f;

const float VConstants<float>::Tolerance = 0.00001f;
const float VConstants<float>::SingularityRadius = 0.0000001f; // Use for Gimbal lock numerical problems

const float VConstants<float>::SmallestNonDenormal = float( 1.1754943508222875e-038f );	// ( 1U << 23 )
const float VConstants<float>::HugeNumber = float( 1.8446742974197924e+019f );	// ( ( ( 127U * 3 / 2 ) << 23 ) | ( ( 1 << 23 ) - 1 ) )

// Double-precision VConstants constants class.
const double VConstants<double>::Pi = 3.14159265358979;

const double VConstants<double>::E = 2.71828182845905;

const double VConstants<double>::MaxValue = DBL_MAX;
const double VConstants<double>::MinPositiveValue = DBL_MIN;

const double VConstants<double>::VRTD = 360.0 / VConstants<double>::Pi*2.0;
const double VConstants<double>::VDTR = VConstants<double>::Pi*2.0 / 360.0;

const double VConstants<double>::Tolerance = 0.00001;
const double VConstants<double>::SingularityRadius = 0.000000000001; // Use for Gimbal lock numerical problems

const double VConstants<double>::SmallestNonDenormal = double( 2.2250738585072014e-308 );	// ( 1ULL << 52 )
const double VConstants<double>::HugeNumber = double( 1.3407807929942596e+154 );	// ( ( ( 1023ULL * 3 / 2 ) << 52 ) | ( ( 1 << 52 ) - 1 ) )

NV_NAMESPACE_END
