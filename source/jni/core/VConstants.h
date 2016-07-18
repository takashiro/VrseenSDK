#pragma once

#include "vglobal.h"

#include <math.h>

NV_NAMESPACE_BEGIN

template<class Type>
class VConstants
{
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


typedef VConstants<float> VConstantsf;
typedef VConstants<double> VConstantsd;

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

// Conversion functions between degrees and radians
template<class T>
T VRadToDegree(T rads) { return rads * VConstants<T>::VRTD; }
template<class T>
T VDegreeToRad(T rads) { return rads * VConstants<T>::VDTR; }

NV_NAMESPACE_END
