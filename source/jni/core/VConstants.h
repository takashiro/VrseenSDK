/* Copyright NVSence
 * By Jogin 2016/3/4
 */

#pragma once

#include "vglobal.h"

#include <assert.h>
#include <stdlib.h>
#include <math.h>
#include "Types.h"
#include "VrApi.h"

typedef struct ovrQuatf_ ovrQuatf;
typedef struct ovrQuatd_ ovrQuatd;
typedef struct ovrSizei_ ovrSizei;
typedef struct ovrSizef_ ovrSizef;
typedef struct ovrVector2i_ ovrVector2i;
typedef struct ovrVector2f_ ovrVector2f;
typedef struct ovrVector3f_ ovrVector3f;
typedef struct ovrVector3d_ ovrVector3d;
typedef struct ovrMatrix4f_ ovrMatrix4f;
typedef struct ovrPosef_ ovrPosef;
typedef struct ovrPoseStatef_ ovrPoseStatef;
typedef struct ovrSensorState_ ovrSensorState;

NV_NAMESPACE_BEGIN

template<class T> class VQuat;
template<class T> class VSize;
template<class T> class VRect;
template<class T> class V2Vect;
template<class T> class V3Vect;
template<class T> class VR3Matrix;
template<class T> class VR4matrix;
template<class T> class VPos;
template<class T> class PoseState;


// CompatibleTypes::Type is used to lookup a compatible C-version of a C++ class.
template<class C>
struct VCompatibleTypes
{
    // Declaration here seems necessary for MSVC; specializations are
    // used instead.
	typedef struct {} Type;
};
//------------------------------------------------------------------------------------//
// ***** VConstants
//
// VConstants class contains constants and functions. This class is a template specialized
// per type, with VConstants<float> and VConstants<double> being distinct.
template<class Type>
class VConstants
{
public:
    // By default, support explicit conversion to float. This allows Vector2<int> to
    // compile, for example.
    typedef float VdifFloat; // replace OtherFloatType with VdifFloat
};

// Single-precision VConstants class.
template<>
class VConstants<float>
{
public:
    static const float Pi;

    static const float E;

    static const float MaxValue;			// Largest positive float Value
    static const float MinPositiveValue;	// Smallest possible positive value

    static const float VRTD; // replace RadToDegreeFactor VRTD
    static const float VDTR; // replace DegreeToRadFactor VDTR

    static const float Tolerance;			// 0.00001f;
    static const float SingularityRadius;	// 0.0000001f for Gimbal lock numerical problems

	static const float SmallestNonDenormal;	// ( numerator <= 4 ) / ( denominator > SmallestNonDenormal ) < infinite
	static const float HugeNumber;			// HugeNumber * HugeNumber < infinite

    // Used to support direct conversions in template classes.
    typedef double VdifFloat;
};

// Double-precision VConstants class.
template<>
class VConstants<double>
{
public:
    static const double Pi;

    static const double E;

    static const double MaxValue;				// Largest positive double Value
    static const double MinPositiveValue;		// Smallest possible positive value

    static const double VRTD;
    static const double VDTR;

    static const double Tolerance;				// 0.00001;
    static const double SingularityRadius;		// 0.000000000001 for Gimbal lock numerical problems

	static const double SmallestNonDenormal;	// ( numerator <= 4 ) / ( denominator > SmallestNonDenormal ) < infinite
	static const double HugeNumber;				// HugeNumber * HugeNumber < infinite

    typedef float VdifFloat;
};


typedef VConstants<float>  VConstantsf;
typedef VConstants<double> VConstantsd;

// Safe reciprocal square root.
template<class T>
T VRcpSqrt( const T f ) { return ( f >= VConstants<T>::SmallestNonDenormal ) ? T(1) / sqrt( f ) : VConstants<T>::HugeNumber; }

// Conversion functions between degrees and radians
template<class T>
T VRadToDegree(T rads) { return rads * VConstants<T>::VRTD; }
template<class T>
T VDegreeToRad(T rads) { return rads * VConstants<T>::VDTR; }

// Numerically stable acos function
template<class T>
T VArccos(T val)
{
		if (val > T(1))				return T(0);
		else if (val < T(-1))		return VConstants<T>::Pi;
		else						return acos(val);
};

// Numerically stable asin function
template<class T>
T VArcsin(T val)
{
	if (val > T(1))				return VConstants<T>::Pi/2.0;
	else if (val < T(-1))		return VConstants<T>::Pi/2.0 * T(3);
	else						return asin(val);
};



template <typename T>
const T V_Min(const T a, const T b)
{ return (a < b) ? a : b; }

template <typename T>
const T V_Max(const T a, const T b)
{ return (b < a) ? a : b; }

//-------------------------------------------------------------------------------------
// ***** Constants for 3D world/axis definitions.

// Definitions of axes for coordinate and rotation conversions.
enum VAxis
{
    VAxis_X = 0, VAxis_Y = 1, VAxis_Z = 2
};

// RotateDirection describes the rotation direction around an axis, interpreted as follows:
//  CW  - Clockwise while looking "down" from positive axis towards the origin.
//  CCW - Counter-clockwise while looking from the positive axis towards the origin,
//        which is in the negative axis direction.
//  CCW is the default for the RHS coordinate system. Oculus standard RHS coordinate
//  system defines Y up, X right, and Z back (pointing out from the screen). In this
//  system Rotate_CCW around Z will specifies counter-clockwise rotation in XY plane.
enum VRotateDirection
{
    VRotate_CCW = 1,
    VRotate_CW  = -1
};

// Constants for right handed and left handed coordinate systems
enum VHandedSystem
{
    VHanded_R = 1, VHanded_L = -1
};

// AxisDirection describes which way the coordinate axis points. Used by WorldAxes.
enum VAxisDirection
{
    VAxis_Up    =  2,
    VAxis_Down  = -2,
    VAxis_Right =  1,
    VAxis_Left  = -1,
    VAxis_In    =  3,
    VAxis_Out   = -3
};

struct VWorldAxes
{
    VAxisDirection XAxis, YAxis, ZAxis;

    VWorldAxes(VAxisDirection x, VAxisDirection y, VAxisDirection z)
        : XAxis(x), YAxis(y), ZAxis(z)
    {}
};

float LinearRangeMapFloat( float inValue, float inStart, float inEnd, float outStart, float outEnd );

NV_NAMESPACE_END
