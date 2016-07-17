#pragma once

#include "vglobal.h"
#include "VVect3.h"
#include "VMatrix.h"
#include <assert.h>
#include <stdlib.h>
#include "VConstants.h"

#include "VQuat.h"

NV_NAMESPACE_BEGIN

// Forward-declare our templates.
template<class T> class VR4Matrix;

template<class T> class VR3Matrix;

template<class T> class PoseState;


//-------------------------------------------------------------------------------------
// ***** VBox

// Bounds class used to describe a 3D axis aligned bounding box.


// ***** VQuat
//
// VQuatf represents a VQuaternion class used for rotations.
//
// VQuaternion multiplications are done in right-to-left order, to match the
// behavior of matrices.




//-------------------------------------------------------------------------------------
// ***** VPos

// Position and orientation combined.

template<class T>
class VPos
{
public:



    VPos() { }
    VPos(const VQuat<T>& orientation, const VVect3<T>& pos)
        : Orientation(orientation), Position(pos) {  }
    VPos(const VPos& s)
        : Orientation(s.Orientation), Position(s.Position) {  }

    explicit VPos(const VPos<typename VConstants<T>::VdifFloat> &s)
        : Orientation(s.Orientation), Position(s.Position) {  }


    VVect3<T> Rotate(const VVect3<T>& v) const
    {
        return Orientation.Rotate(v);
    }

    VVect3<T> Translate(const VVect3<T>& v) const
    {
        return v + Position;
    }

    VVect3<T> Apply(const VVect3<T>& v) const
    {
        return Translate(Rotate(v));
    }

    VPos operator*(const VPos& other) const
    {
        return VPos(Orientation * other.Orientation, Apply(other.Position));
    }

    VPos Inverted() const
    {
        VQuat<T> inv = Orientation.Inverted();
        return VPos(inv, inv.Rotate(-Position));
    }

    VQuat<T>    Orientation;
    VVect3<T> Position;
};

typedef VPos<float>  VPosf;
typedef VPos<double> VPosd;

NV_NAMESPACE_END
