#pragma once

#include "VConstants.h"
#include "VVect3.h"
#include "VVect4.h"

#include "VLog.h"

#include <assert.h>
#include <stdlib.h>
#include <math.h>

NV_NAMESPACE_BEGIN

template <class T> class VR4Matrix;


template<class T> class VQuat;
template<class T> class VPos;
//-------------------------------------------------------------------------------------
// ***** VR4Matrix
//
// VR4Matrix is a 4x4 matrix used for 3d transformations and projections.
// Translation stored in the last column.
// The matrix is stored in row-major order in memory, meaning that values
// of the first row are stored before the next one.
//
// The arrangement of the matrix is chosen to be in Right-Handed
// coordinate system and counterclockwise rotations when looking down
// the VAxis
//
// Transformation Order:
//   - Transformations are applied from right to left, so the expression
//     M1 * M2 * M3 * V means that the vector V is transformed by M3 first,
//     followed by M2 and M1.
//
// Coordinate system: Right Handed
//
// Rotations: Counterclockwise when looking down the VAxis. All angles are in radians.
//
//  | sx   01   02   tx |    // First column  (sx, 10, 20): VAxis X basis vector.
//  | 10   sy   12   ty |    // Second column (01, sy, 21): VAxis Y basis vector.
//  | 20   21   sz   tz |    // Third columnt (02, 12, sz): VAxis Z basis vector.
//  | 30   31   32   33 |
//
//  The basis vectors are first three columns.

template<class T>
class VR4Matrix
{
    static const VR4Matrix IdentityValue;

public:
    T cell[4][4];

    // By default, we construct identity matrix.
    VR4Matrix()
    {
        memset(cell, 0, sizeof(cell));
        cell[0][0] = cell[1][1] = cell[2][2] = cell[3][3] = 1;
    }

    VR4Matrix(T m11, T m12, T m13, T m14,
              T m21, T m22, T m23, T m24,
              T m31, T m32, T m33, T m34,
              T m41, T m42, T m43, T m44)
    {
        cell[0][0] = m11; cell[0][1] = m12; cell[0][2] = m13; cell[0][3] = m14;
        cell[1][0] = m21; cell[1][1] = m22; cell[1][2] = m23; cell[1][3] = m24;
        cell[2][0] = m31; cell[2][1] = m32; cell[2][2] = m33; cell[2][3] = m34;
        cell[3][0] = m41; cell[3][1] = m42; cell[3][2] = m43; cell[3][3] = m44;
    }

    VR4Matrix(T m11, T m12, T m13,
              T m21, T m22, T m23,
              T m31, T m32, T m33)
    {
        cell[0][0] = m11; cell[0][1] = m12; cell[0][2] = m13; cell[0][3] = 0;
        cell[1][0] = m21; cell[1][1] = m22; cell[1][2] = m23; cell[1][3] = 0;
        cell[2][0] = m31; cell[2][1] = m32; cell[2][2] = m33; cell[2][3] = 0;
        cell[3][0] = 0;   cell[3][1] = 0;   cell[3][2] = 0;   cell[3][3] = 1;
    }

    VR4Matrix(const VQuat<T> &q)
    {
        T ww = q.w * q.w;
        T xx = q.x * q.x;
        T yy = q.y * q.y;
        T zz = q.z * q.z;

        cell[0][0] = ww + xx - yy - zz;
        cell[0][1] = 2 * (q.x * q.y - q.w * q.z);
        cell[0][2] = 2 * (q.x * q.z + q.w * q.y);
        cell[0][3] = 0;
        cell[1][0] = 2 * (q.x * q.y + q.w * q.z);
        cell[1][1] = ww - xx + yy - zz;
        cell[1][2] = 2 * (q.y * q.z - q.w * q.x);
        cell[1][3] = 0;
        cell[2][0] = 2 * (q.x * q.z - q.w * q.y);
        cell[2][1] = 2 * (q.y * q.z + q.w * q.x);
        cell[2][2] = ww - xx - yy + zz;
        cell[2][3] = 0;
        cell[3][0] = 0;
        cell[3][1] = 0;
        cell[3][2] = 0;
        cell[3][3] = 1;
    }

    T *data() { return reinterpret_cast<T *>(cell); }
    const T *data() const { return reinterpret_cast<const T*>(cell); }

    void setXBasis(const VVect3<T> &v)
    {
        cell[0][0] = v.x;
        cell[1][0] = v.y;
        cell[2][0] = v.z;
    }

    VVect3<T> xBasis() const
    {
        return VVect3<T>(cell[0][0], cell[1][0], cell[2][0]);
    }

    void setYBasis(const VVect3<T> &v)
    {
        cell[0][1] = v.x;
        cell[1][1] = v.y;
        cell[2][1] = v.z;
    }

    VVect3<T> yBasis() const
    {
        return VVect3<T>(cell[0][1], cell[1][1], cell[2][1]);
    }

    void setZBasis(const VVect3<T> &v)
    {
        cell[0][2] = v.x;
        cell[1][2] = v.y;
        cell[2][2] = v.z;
    }

    VVect3<T> zBasis() const
    {
        return VVect3<T>(cell[0][2], cell[1][2], cell[2][2]);
    }


    bool operator== (const VR4Matrix &matrix) const
    {
        for (int i = 0; i < 4; i++) {
            for (int j = 0; j < 4; j++) {
                if (cell[i][j] != matrix.cell[i][j]) {
                    return false;
                }
            }
        }
        return true;
    }

    VR4Matrix operator + (const VR4Matrix &matrix) const
    {
        VR4Matrix result(*this);
        result += matrix;
        return result;
    }

    VR4Matrix& operator+= (const VR4Matrix &matrix)
    {
        for (int i = 0; i < 4; i++) {
            for (int j = 0; j < 4; j++) {
                cell[i][j] += matrix.cell[i][j];
            }
        }
        return *this;
    }

    VR4Matrix operator - (const VR4Matrix &matrix) const
    {
        VR4Matrix result(*this);
        result -= matrix;
        return result;
    }

    VR4Matrix &operator -= (const VR4Matrix &matrix)
    {
        for (int i = 0; i < 4; i++) {
            for (int j = 0; j < 4; j++) {
                cell[i][j] -= matrix.cell[i][j];
            }
        }
        return *this;
    }

    VR4Matrix operator * (const VR4Matrix &matrix) const
    {
        VR4Matrix result;
        for (int i = 0; i < 4; i++) {
            result.cell[i][0] = cell[i][0] * matrix.cell[0][0] + cell[i][1] * matrix.cell[1][0] + cell[i][2] * matrix.cell[2][0] + cell[i][3] * matrix.cell[3][0];
            result.cell[i][1] = cell[i][0] * matrix.cell[0][1] + cell[i][1] * matrix.cell[1][1] + cell[i][2] * matrix.cell[2][1] + cell[i][3] * matrix.cell[3][1];
            result.cell[i][2] = cell[i][0] * matrix.cell[0][2] + cell[i][1] * matrix.cell[1][2] + cell[i][2] * matrix.cell[2][2] + cell[i][3] * matrix.cell[3][2];
            result.cell[i][3] = cell[i][0] * matrix.cell[0][3] + cell[i][1] * matrix.cell[1][3] + cell[i][2] * matrix.cell[2][3] + cell[i][3] * matrix.cell[3][3];
        }
        return result;
    }

    VR4Matrix &operator *= (const VR4Matrix &matrix)
    {
        *this = *this * matrix;
        return *this;
    }

    VR4Matrix operator * (T factor) const
    {
        VR4Matrix result(*this);
        result *= factor;
        return result;
    }

    VR4Matrix &operator*= (T factor)
    {
        for (T (&row)[4] : cell) {
            for (T &value : row) {
                value *= factor;
            }
        }
        return *this;
    }

    VR4Matrix operator / (T divisor) const
    {
        VR4Matrix result(*this);
        result /= divisor;
        return result;
    }

    VR4Matrix &operator /= (T factor)
    {
        for (T (&row)[4] : cell) {
            for (T &value : row) {
                value /= factor;
            }
        }
        return *this;
    }

    VVect3<T> transform(const VVect3<T> &vect) const
    {
        const T rcpW = T(1) / (cell[3][0] * vect.x + cell[3][1] * vect.y + cell[3][2] * vect.z + cell[3][3]);
           return VVect3<T>((cell[0][0] * vect.x + cell[0][1] * vect.y + cell[0][2] * vect.z + cell[0][3]) * rcpW,
                            (cell[1][0] * vect.x + cell[1][1] * vect.y + cell[1][2] * vect.z + cell[1][3]) * rcpW,
                            (cell[2][0] * vect.x + cell[2][1] * vect.y + cell[2][2] * vect.z + cell[2][3]) * rcpW);
    }

    V4Vect<T> transform(const V4Vect<T> &vect) const
    {
        return V4Vect<T>(cell[0][0] * vect.x + cell[0][1] * vect.y + cell[0][2] * vect.z + cell[0][3] * vect.w,
                         cell[1][0] * vect.x + cell[1][1] * vect.y + cell[1][2] * vect.z + cell[1][3] * vect.w,
                         cell[2][0] * vect.x + cell[2][1] * vect.y + cell[2][2] * vect.z + cell[2][3] * vect.w,
                         cell[3][0] * vect.x + cell[3][1] * vect.y + cell[3][2] * vect.z + cell[3][3] * vect.w);
    }

    VR4Matrix transposed() const
    {
        return VR4Matrix(cell[0][0], cell[1][0], cell[2][0], cell[3][0],
                         cell[0][1], cell[1][1], cell[2][1], cell[3][1],
                         cell[0][2], cell[1][2], cell[2][2], cell[3][2],
                         cell[0][3], cell[1][3], cell[2][3], cell[3][3]);
    }

    void transpose()
    {
        for (int i = 0; i < 4; i++) {
            for (int j = i + 1; j < 4; j++) {
                std::swap(cell[i][j], cell[j][i]);
            }
        }
    }

    T subdeterminant(const uint* rows, const uint* cols) const
    {
        return cell[rows[0]][cols[0]] * (cell[rows[1]][cols[1]] * cell[rows[2]][cols[2]] - cell[rows[1]][cols[2]] * cell[rows[2]][cols[1]])
             - cell[rows[0]][cols[1]] * (cell[rows[1]][cols[0]] * cell[rows[2]][cols[2]] - cell[rows[1]][cols[2]] * cell[rows[2]][cols[0]])
             + cell[rows[0]][cols[2]] * (cell[rows[1]][cols[0]] * cell[rows[2]][cols[1]] - cell[rows[1]][cols[1]] * cell[rows[2]][cols[0]]);
    }

    T cofactor(uint i, uint j) const
    {
        const uint indices[4][3] = {
            {1, 2, 3},
            {0, 2, 3},
            {0, 1, 3},
            {0, 1, 2}
        };
        return ((i + j)&1) ? -subdeterminant(indices[i], indices[j]) : subdeterminant(indices[i], indices[j]);
    }

    T determinant() const
    {
        return cell[0][0] * cofactor(0, 0) + cell[0][1] * cofactor(0, 1) + cell[0][2] * cofactor(0, 2) + cell[0][3] * cofactor(0, 3);
    }

    VR4Matrix adjugated() const
    {
        return VR4Matrix(cofactor(0,0), cofactor(1,0), cofactor(2,0), cofactor(3,0),
                         cofactor(0,1), cofactor(1,1), cofactor(2,1), cofactor(3,1),
                         cofactor(0,2), cofactor(1,2), cofactor(2,2), cofactor(3,2),
                         cofactor(0,3), cofactor(1,3), cofactor(2,3), cofactor(3,3));
    }

    VR4Matrix inverted() const { return adjugated() * (1.0f / determinant()); }
    void invert() { *this = inverted(); }

    // Matrix to Euler Angles conversion
    // a,b,c, are the YawPitchRoll angles to be returned
    // rotation a around VAxis A1
    // is followed by rotation b around VAxis A2
    // is followed by rotation c around VAxis A3
    // rotations are CCW or CW (D) in LH or RH coordinate system (S)
    template <VAxis A1, VAxis A2, VAxis A3, VRotateDirection D, VHandedSystem S>
    void toEulerAngles(T *a, T *b, T *c) const
    {
        static_assert((A1 != A2) && (A2 != A3) && (A1 != A3), "A1,A2,A3 should be different");

        T psign = -1;
        if (((A1 + 1) % 3 == A2) && ((A2 + 1) % 3 == A3)) // Determine whether even permutation
        psign = 1;

        T pm = psign*cell[A1][A3];

        if (pm < -1.0f + VConstants<T>::SingularityRadius) {
            // South pole singularity
            *a = 0;
            *b = -S*D*VConstants<T>::Pi/2.0;
            *c = S*D*atan2( psign*cell[A2][A1], cell[A2][A2] );

        } else if (pm > 1.0f - VConstants<T>::SingularityRadius) {
            // North pole singularity
            *a = 0;
            *b = S*D*VConstants<T>::Pi/2.0;
            *c = S*D*atan2( psign*cell[A2][A1], cell[A2][A2] );
        } else {
            // Normal case (nonsingular)
            *a = S*D*atan2( -psign*cell[A2][A3], cell[A3][A3] );
            *b = S*D*asin(pm);
            *c = S*D*atan2( -psign*cell[A1][A2], cell[A1][A1] );
        }
    }

    // Matrix to Euler Angles conversion
    // a,b,c, are the YawPitchRoll angles to be returned
    // rotation a around VAxis A1
    // is followed by rotation b around VAxis A2
    // is followed by rotation c around VAxis A1
    // rotations are CCW or CW (D) in LH or RH coordinate system (S)
    template <VAxis A1, VAxis A2, VRotateDirection D, VHandedSystem S>
    void ToEulerAnglesABA(T *a, T *b, T *c) const
    {
        static_assert(A1 != A2, "A1 and A2 should be different");

        // Determine the VAxis that was not supplied
        int m = 3 - A1 - A2;

        T psign = -1;
        if ((A1 + 1) % 3 == A2) {
            // Determine whether even permutation
            psign = 1.0f;
        }

        T c2 = cell[A1][A1];
        if (c2 < -1 + VConstants<T>::SingularityRadius) {
            // South pole singularity
            *a = 0;
            *b = S*D*VConstants<T>::Pi;
            *c = S*D*atan2( -psign*cell[A2][m],cell[A2][A2]);
        } else if (c2 > 1.0f - VConstants<T>::SingularityRadius) {
            // North pole singularity
            *a = 0;
            *b = 0;
            *c = S*D*atan2( -psign*cell[A2][m],cell[A2][A2]);
        } else {
            // Normal case (nonsingular)
            *a = S*D*atan2( cell[A2][A1],-psign*cell[m][A1]);
            *b = S*D*acos(c2);
            *c = S*D*atan2( cell[A1][A2],psign*cell[A1][m]);
        }
    }

    // Creates a matrix that converts the vertices from one coordinate system
    // to another.
    static VR4Matrix VAxisConversion(const VWorldAxes &to, const VWorldAxes &from)
    {
        // Holds VAxis values from the 'to' structure
        int toArray[3] = {to.XAxis, to.YAxis, to.ZAxis};

        // The inverse of the toArray
        int inv[4];
        inv[0] = inv[abs(to.XAxis)] = 0;
        inv[abs(to.YAxis)] = 1;
        inv[abs(to.ZAxis)] = 2;

        VR4Matrix m(0, 0, 0,
                    0, 0, 0,
                    0, 0, 0);

        // Only three values in the matrix need to be changed to 1 or -1.
        m.cell[inv[abs(from.XAxis)]][0] = T(from.XAxis / toArray[inv[abs(from.XAxis)]]);
        m.cell[inv[abs(from.YAxis)]][1] = T(from.YAxis / toArray[inv[abs(from.YAxis)]]);
        m.cell[inv[abs(from.ZAxis)]][2] = T(from.ZAxis / toArray[inv[abs(from.ZAxis)]]);
        return m;
    }


    // Creates a matrix for translation by vector
    static VR4Matrix Translation(const VVect3<T> &v)
    {
        VR4Matrix t;
        t.cell[0][3] = v.x;
        t.cell[1][3] = v.y;
        t.cell[2][3] = v.z;
        return t;
    }

    // Creates a matrix for translation by vector
    static VR4Matrix Translation(T x, T y, T z = T(0))
    {
        VR4Matrix t;
        t.cell[0][3] = x;
        t.cell[1][3] = y;
        t.cell[2][3] = z;
        return t;
    }

    // Sets the translation part
    void setTranslation(const VVect3<T> &v)
    {
        cell[0][3] = v.x;
        cell[1][3] = v.y;
        cell[2][3] = v.z;
    }

    VVect3<T> translation() const
    {
        return VVect3<T>(cell[0][3], cell[1][3], cell[2][3]);
    }

    // Creates a matrix for scaling by vector
    static VR4Matrix Scaling(const VVect3<T>& v)
    {
        VR4Matrix t;
        t.cell[0][0] = v.x;
        t.cell[1][1] = v.y;
        t.cell[2][2] = v.z;
        return t;
    }

    // Creates a matrix for scaling by vector
    static VR4Matrix Scaling(T x, T y, T z)
    {
        VR4Matrix t;
        t.cell[0][0] = x;
        t.cell[1][1] = y;
        t.cell[2][2] = z;
        return t;
    }

    // Creates a matrix for scaling by constant
    static VR4Matrix Scaling(T s)
    {
        VR4Matrix t;
        t.cell[0][0] = s;
        t.cell[1][1] = s;
        t.cell[2][2] = s;
        return t;
    }

       // Simple L1 distance in R^12
    T distanceTo(const VR4Matrix &m2) const
    {
        T d = fabs(cell[0][0] - m2.cell[0][0]) + fabs(cell[0][1] - m2.cell[0][1]);
        d += fabs(cell[0][2] - m2.cell[0][2]) + fabs(cell[0][3] - m2.cell[0][3]);
        d += fabs(cell[1][0] - m2.cell[1][0]) + fabs(cell[1][1] - m2.cell[1][1]);
        d += fabs(cell[1][2] - m2.cell[1][2]) + fabs(cell[1][3] - m2.cell[1][3]);
        d += fabs(cell[2][0] - m2.cell[2][0]) + fabs(cell[2][1] - m2.cell[2][1]);
        d += fabs(cell[2][2] - m2.cell[2][2]) + fabs(cell[2][3] - m2.cell[2][3]);
        d += fabs(cell[3][0] - m2.cell[3][0]) + fabs(cell[3][1] - m2.cell[3][1]);
        d += fabs(cell[3][2] - m2.cell[3][2]) + fabs(cell[3][3] - m2.cell[3][3]);
        return d;
    }

    // Creates a rotation matrix rotating around the X VAxis by 'angle' radians.
    // Just for quick testing.  Not for final API.  Need to remove case.
    static VR4Matrix RotationVAxis(VAxis A, T angle, VRotateDirection d, VHandedSystem s)
    {
        T sina = s * d *sin(angle);
        T cosa = cos(angle);

        switch(A) {
        case VAxis_X:
            return VR4Matrix(1,  0,     0,
                             0,  cosa,  -sina,
                             0,  sina,  cosa);
        case VAxis_Y:
            return VR4Matrix(cosa, 0, sina,
                                0, 1,    0,
                            -sina, 0, cosa);
        case VAxis_Z:
            return VR4Matrix(cosa,  -sina,  0,
                             sina,  cosa,   0,
                                0,     0,   1);
        }
    }


    // Creates a rotation matrix rotating around the X VAxis by 'angle' radians.
    // Rotation direction is depends on the coordinate system:
    // RHS (Oculus default): Positive angle values rotate Counter-clockwise (CCW),
    //                        while looking in the negative VAxis direction. This is the
    //                        same as looking down from positive VAxis values towards origin.
    // LHS: Positive angle values rotate clock-wise (CW), while looking in the
    //       negative VAxis direction.
    static VR4Matrix RotationX(T angle)
    {
        T sina = sin(angle);
        T cosa = cos(angle);
        return VR4Matrix(1,  0,     0,
                         0,  cosa,  -sina,
                         0,  sina,  cosa);
    }

    // Creates a rotation matrix rotating around the Y VAxis by 'angle' radians.
    // Rotation direction is depends on the coordinate system:
    //  RHS (Oculus default): Positive angle values rotate Counter-clockwise (CCW),
    //                        while looking in the negative VAxis direction. This is the
    //                        same as looking down from positive VAxis values towards origin.
    //  LHS: Positive angle values rotate clock-wise (CW), while looking in the
    //       negative VAxis direction.
    static VR4Matrix RotationY(T angle)
    {
        T sina = sin(angle);
        T cosa = cos(angle);
        return VR4Matrix(cosa, 0, sina,
                            0, 1,    0,
                        -sina, 0, cosa);
    }

    // Creates a rotation matrix rotating around the Z VAxis by 'angle' radians.
    // Rotation direction is depends on the coordinate system:
    //  RHS (Oculus default): Positive angle values rotate Counter-clockwise (CCW),
    //                        while looking in the negative VAxis direction. This is the
    //                        same as looking down from positive VAxis values towards origin.
    //  LHS: Positive angle values rotate clock-wise (CW), while looking in the
    //       negative VAxis direction.
    static VR4Matrix RotationZ(T angle)
    {
        T sina = sin(angle);
        T cosa = cos(angle);
        return VR4Matrix(cosa, -sina, 0,
                         sina,  cosa, 0,
                            0,     0, 1);
    }

    static VR4Matrix RotationVAxisAngle(const VVect3<T> &vAxis, T angle)
    {
        T x = vAxis.x;
        T y = vAxis.y;
        T z = vAxis.z;
        T c = cos(angle);
        T s = sin(angle);
        T t = 1 - c;
        return VR4Matrix(t * x * x + c, t * x * y - z * s, t * x * z + y * s,
                         t * x * y + z * s, t * y * y + c, t * y * z - x * s,
                         y * x * z - y * s, t * y * z + x * s, t * z * z + c);
    }

    // LookAtRH creates a View transformation matrix for right-handed coordinate system.
    // The resulting matrix points camera from 'eye' towards 'at' direction, with 'up'
    // specifying the up vector. The resulting matrix should be used with PerspectiveRH
    // projection.
    static VR4Matrix LookAtRH(const VVect3<T>& eye, const VVect3<T>& at, const VVect3<T>& up)
    {
        // FIXME: this fails when looking straight up, should probably at least assert
        VVect3<T> z = (eye - at).normalized();  // Forward
        VVect3<T> x = up.crossProduct(z).normalized(); // Right
        VVect3<T> y = z.crossProduct(x);

        return VR4Matrix(x.x,  x.y,  x.z,  -(x.dotProduct(eye)),
                         y.x,  y.y,  y.z,  -(y.dotProduct(eye)),
                         z.x,  z.y,  z.z,  -(z.dotProduct(eye)),
                         0,    0,    0,    1 );
    }

    // LookAtLH creates a View transformation matrix for left-handed coordinate system.
    // The resulting matrix points camera from 'eye' towards 'at' direction, with 'up'
    // specifying the up vector.
    static VR4Matrix LookAtLH(const VVect3<T>& eye, const VVect3<T>& at, const VVect3<T>& up)
    {
        // FIXME: this fails when looking straight up, should probably at least assert
        VVect3<T> z = (at - eye).normalized();  // Forward
        VVect3<T> x = up.crossProduct(z).normalized(); // Right
        VVect3<T> y = z.crossProduct(x);

        return VR4Matrix(x.x,  x.y,  x.z,  -(x.dotProduct(eye)),
                         y.x,  y.y,  y.z,  -(y.dotProduct(eye)),
                         z.x,  z.y,  z.z,  -(z.dotProduct(eye)),
                         0,    0,    0,    1 );
    }

    static VR4Matrix CreateFromBasisVectors( VVect3<T> const & zBasis, VVect3<T> const & up )
    {
        vAssert(zBasis.isNormalized());
        vAssert(up.isNormalized());
        T dot = zBasis.dotProduct(up);
        if ( dot < (T)-0.9999 || dot > (T)0.9999 ) {
            // z basis cannot be parallel to the specified up
            vAssert( dot >= (T)-0.9999 || dot <= (T)0.9999 );
            return VR4Matrix();
        }

        VVect3<T> xBasis = up.crossProduct( zBasis );
        xBasis.normalize();

        VVect3<T> yBasis = zBasis.crossProduct( xBasis );
           // no need to normalize yBasis because xBasis and zBasis must already be orthogonal

        return VR4Matrix<T>(xBasis.x, yBasis.x, zBasis.x, (T) 0,
                            xBasis.y, yBasis.y, zBasis.y, (T) 0,
                            xBasis.z, yBasis.z, zBasis.z, (T) 0,
                               (T) 0,    (T) 0,    (T) 0, (T) 1);
    }

    // PerspectiveRH creates a right-handed perspective projection matrix that can be
    // used with the Oculus sample renderer.
    //  yfov   - Specifies vertical field of view in radians.
    //  aspect - Screen aspect ration, which is usually width/height for square pixels.
    //           Note that xfov = yfov * aspect.
    //  znear  - Absolute value of near Z clipping clipping range.
    //  zfar   - Absolute value of far  Z clipping clipping range (larger then near).
    // Even though RHS usually looks in the direction of negative Z, positive values
    // are expected for znear and zfar.
    static VR4Matrix PerspectiveRH(T yfov, T aspect, T znear, T zfar)
    {
        VR4Matrix m;
        T tanHalfFov = tan(yfov * T(0.5f));

        m.cell[0][0] = T(1) / (aspect * tanHalfFov);
        m.cell[1][1] = T(1) / tanHalfFov;
        m.cell[2][2] = zfar / (znear - zfar);
        // m.M[2][2] = zfar / (zfar - znear);
        m.cell[3][2] = T(-1);
        m.cell[2][3] = (zfar * znear) / (znear - zfar);
        m.cell[3][3] = T(0);

        // Note: Post-projection matrix result assumes Left-Handed coordinate system,
        //       with Y up, X right and Z forward. This supports positive z-buffer values.
        // This is the case even for RHS cooridnate input.
        return m;
    }

    // PerspectiveLH creates a left-handed perspective projection matrix that can be
    // used with the Oculus sample renderer.
    //  yfov   - Specifies vertical field of view in radians.
    //  aspect - Screen aspect ration, which is usually width/height for square pixels.
    //           Note that xfov = yfov * aspect.
    //  znear  - Absolute value of near Z clipping clipping range.
    //  zfar   - Absolute value of far  Z clipping clipping range (larger then near).
    static VR4Matrix PerspectiveLH(T yfov, T aspect, T znear, T zfar)
    {
        VR4Matrix m;
        T tanHalfFov = tan(yfov * T(0.5f));

        m.cell[0][0] = T(1) / (aspect * tanHalfFov);
        m.cell[1][1] = T(1) / tanHalfFov;
        m.cell[2][2] = zfar / (zfar - znear);
        m.cell[3][2] = T(1);
        m.cell[2][3] = (zfar * znear) / (znear - zfar);
        m.cell[3][3] = T(0);

        // Note: Post-projection matrix result assumes Left-Handed coordinate system,
        //       with Y up, X right and Z forward. This supports positive z-buffer values.
        return m;
    }

    static VR4Matrix Ortho2D(T width, T height)
    {
        VR4Matrix m;
        m.cell[0][0] = T(2) / width;
        m.cell[1][1] = T(-2) / height;
        m.cell[0][3] = T(-1);
        m.cell[1][3] = T(1);
        m.cell[2][2] = T(0);
        return m;
    }

    // Returns a 3x3 minor of a 4x4 matrix.
    static T Minor(const VR4Matrix<T> * m, int r0, int r1, int r2, int c0, int c1, int c2)
    {
        return m->cell[r0][c0] * (m->cell[r1][c1] * m->cell[r2][c2] - m->cell[r2][c1] * m->cell[r1][c2]) -
               m->cell[r0][c1] * (m->cell[r1][c0] * m->cell[r2][c2] - m->cell[r2][c0] * m->cell[r1][c2]) +
               m->cell[r0][c2] * (m->cell[r1][c0] * m->cell[r2][c1] - m->cell[r2][c0] * m->cell[r1][c1]);
    }

    // Returns the inverse of a 4x4 matrix.
    static VR4Matrix<T> Inverse(const VR4Matrix<T> *m)
    {
        const float rcpDet = 1.0f / (m->cell[0][0] * Minor(m, 1, 2, 3, 1, 2, 3) -
                                     m->cell[0][1] * Minor(m, 1, 2, 3, 0, 2, 3) +
                                     m->cell[0][2] * Minor(m, 1, 2, 3, 0, 1, 3) -
                                     m->cell[0][3] * Minor(m, 1, 2, 3, 0, 1, 2));
        VR4Matrix<T> out;
        out.cell[0][0] =  Minor(m, 1, 2, 3, 1, 2, 3) * rcpDet;
        out.cell[0][1] = -Minor(m, 0, 2, 3, 1, 2, 3) * rcpDet;
        out.cell[0][2] =  Minor(m, 0, 1, 3, 1, 2, 3) * rcpDet;
        out.cell[0][3] = -Minor(m, 0, 1, 2, 1, 2, 3) * rcpDet;
        out.cell[1][0] = -Minor(m, 1, 2, 3, 0, 2, 3) * rcpDet;
        out.cell[1][1] =  Minor(m, 0, 2, 3, 0, 2, 3) * rcpDet;
        out.cell[1][2] = -Minor(m, 0, 1, 3, 0, 2, 3) * rcpDet;
        out.cell[1][3] =  Minor(m, 0, 1, 2, 0, 2, 3) * rcpDet;
        out.cell[2][0] =  Minor(m, 1, 2, 3, 0, 1, 3) * rcpDet;
        out.cell[2][1] = -Minor(m, 0, 2, 3, 0, 1, 3) * rcpDet;
        out.cell[2][2] =  Minor(m, 0, 1, 3, 0, 1, 3) * rcpDet;
        out.cell[2][3] = -Minor(m, 0, 1, 2, 0, 1, 3) * rcpDet;
        out.cell[3][0] = -Minor(m, 1, 2, 3, 0, 1, 2) * rcpDet;
        out.cell[3][1] =  Minor(m, 0, 2, 3, 0, 1, 2) * rcpDet;
        out.cell[3][2] = -Minor(m, 0, 1, 3, 0, 1, 2) * rcpDet;
        out.cell[3][3] =  Minor(m, 0, 1, 2, 0, 1, 2) * rcpDet;
        return out;
    }

    // Returns the 4x4 rotation matrix for the given quaternion.
    static VR4Matrix<T> CreateFromQuaternion(const VQuat<T> *q)
    {
        const float ww = q->w * q->w;
        const float xx = q->x * q->x;
        const float yy = q->y * q->y;
        const float zz = q->z * q->z;

        VR4Matrix<T> out;
        out.cell[0][0] = ww + xx - yy - zz;
        out.cell[0][1] = 2 * ( q->x * q->y - q->w * q->z );
        out.cell[0][2] = 2 * ( q->x * q->z + q->w * q->y );
        out.cell[0][3] = 0;
        out.cell[1][0] = 2 * ( q->x * q->y + q->w * q->z );
        out.cell[1][1] = ww - xx + yy - zz;
        out.cell[1][2] = 2 * ( q->y * q->z - q->w * q->x );
        out.cell[1][3] = 0;
        out.cell[2][0] = 2 * ( q->x * q->z - q->w * q->y );
        out.cell[2][1] = 2 * ( q->y * q->z + q->w * q->x );
        out.cell[2][2] = ww - xx - yy + zz;
        out.cell[2][3] = 0;
        out.cell[3][0] = 0;
        out.cell[3][1] = 0;
        out.cell[3][2] = 0;
        out.cell[3][3] = 1;
        return out;
    }


    // Convert a standard projection matrix into a TanAngle matrix for
    // the primary time warp surface.
    static VR4Matrix<T> TanAngleMatrixFromProjection(const VR4Matrix<T> *projection)
    {
        // A projection matrix goes from a view point to NDC, or -1 to 1 space.
        // Scale and bias to convert that to a 0 to 1 space.
        const VR4Matrix<T> tanAngleMatrix = {
            {
                {0.5f * projection->cell[0][0], 0.5f * projection->cell[0][1], 0.5f * projection->cell[0][2] - 0.5f, 0.5f * projection->cell[0][3]},
                {0.5f * projection->cell[1][0], 0.5f * projection->cell[1][1], 0.5f * projection->cell[1][2] - 0.5f, 0.5f * projection->cell[1][3]},
                {0.0f, 0.0f, -1.0f, 0.0f},
                {0.0f, 0.0f, -1.0f, 0.0f}
            }
        };
        return tanAngleMatrix;
    }

    // Trivial version of TanAngleMatrixFromProjection() for a symmetric field of view.
    static VR4Matrix<T> TanAngleMatrixFromFov(float fovDegrees)
    {
        const float tanHalfFov = tanf(0.5f * fovDegrees * (M_PI / 180.0f));
        VR4Matrix<T> tanAngleMatrix(0.5f / tanHalfFov, 0.0f, -0.5f, 0.0f,
                                    0.0f, 0.5f / tanHalfFov, -0.5f, 0.0f,
                                    0.0f, 0.0f, -1.0f, 0.0f,
                                    0.0f, 0.0f, -1.0f, 0.0f);
        return tanAngleMatrix;
    }

    // If a simple quad defined as a -1 to 1 XY unit square is transformed to
    // the camera view with the given modelView matrix, it can alternately be
    // drawn as a TimeWarp overlay image to take advantage of the full window
    // resolution, which is usually higher than the eye buffer textures, and
    // avoid resampling both into the eye buffer, and again to the screen.
    // This is used for high quality movie screens and user interface planes.
    //
    // Note that this is NOT an MVP matrix -- the "projection" is handled
    // by the distortion process.
    //
    // The exact composition of the overlay image and the base image is
    // determined by the warpProgram, you may still need to draw the geometry
    // into the eye buffer to punch a hole in the alpha channel to let the
    // overlay/underlay show through.
    //
    // This utility functions converts a model-view matrix that would normally
    // draw a -1 to 1 unit square to the view into a TanAngle matrix for an
    // overlay surface.
    //
    // The resulting z value should be straight ahead distance to the plane.
    // The x and y values will be pre-multiplied by z for projective texturing.
    static VR4Matrix<T> TanAngleMatrixFromUnitSquare( const VR4Matrix<T> * modelView )
    {
        const VR4Matrix<T> inv = Inverse( modelView );
        VR4Matrix<T> m;
        m.cell[0][0] = 0.5f * inv.cell[2][0] - 0.5f * (inv.cell[0][0] * inv.cell[2][3] - inv.cell[0][3] * inv.cell[2][0]);
        m.cell[0][1] = 0.5f * inv.cell[2][1] - 0.5f * (inv.cell[0][1] * inv.cell[2][3] - inv.cell[0][3] * inv.cell[2][1]);
        m.cell[0][2] = 0.5f * inv.cell[2][2] - 0.5f * (inv.cell[0][2] * inv.cell[2][3] - inv.cell[0][3] * inv.cell[2][2]);
        m.cell[0][3] = 0.0f;
        m.cell[1][0] = 0.5f * inv.cell[2][0] + 0.5f * (inv.cell[1][0] * inv.cell[2][3] - inv.cell[1][3] * inv.cell[2][0]);
        m.cell[1][1] = 0.5f * inv.cell[2][1] + 0.5f * (inv.cell[1][1] * inv.cell[2][3] - inv.cell[1][3] * inv.cell[2][1]);
        m.cell[1][2] = 0.5f * inv.cell[2][2] + 0.5f * (inv.cell[1][2] * inv.cell[2][3] - inv.cell[1][3] * inv.cell[2][2]);
        m.cell[1][3] = 0.0f;
        m.cell[2][0] = m.cell[3][0] = inv.cell[2][0];
        m.cell[2][1] = m.cell[3][1] = inv.cell[2][1];
        m.cell[2][2] = m.cell[3][2] = inv.cell[2][2];
        m.cell[2][3] = m.cell[3][3] = 0.0f;
        return m;
    }

    // Utility function to calculate external velocity for smooth stick yaw turning.
    // To reduce judder in FPS style experiences when the application framerate is
    // lower than the vsync rate, the rotation from a joypad can be applied to the
    // view space distorted eye vectors before applying the time warp.
    static VR4Matrix<T> CalculateExternalVelocity( const VR4Matrix<T> * viewMatrix, const float yawRadiansPerSecond )
    {
        const float angle = yawRadiansPerSecond * ( -1.0f / 60.0f );
        const float sinHalfAngle = sinf( angle * 0.5f );
        const float cosHalfAngle = cosf( angle * 0.5f );

        // Yaw is always going to be around the world Y axis
        VQuat<T> quat;
        quat.x = viewMatrix->cell[0][1] * sinHalfAngle;
        quat.y = viewMatrix->cell[1][1] * sinHalfAngle;
        quat.z = viewMatrix->cell[2][1] * sinHalfAngle;
        quat.w = cosHalfAngle;
        return CreateFromQuaternion(&quat);
    }
};

typedef VR4Matrix<float>  VR4Matrixf;
typedef VR4Matrix<double> VR4Matrixd;

//-------------------------------------------------------------------------------------
// ***** VR3Matrix
//
// VR3Matrix is a 3x3 matrix used for representing a rotation matrix.
// The matrix is stored in row-major order in memory, meaning that values
// of the first row are stored before the next one.
//
// The arrangement of the matrix is chosen to be in Right-Handed
// coordinate system and counterclockwise rotations when looking down
// the VAxis
//
// Transformation Order:
//   - Transformations are applied from right to left, so the expression
//     M1 * M2 * M3 * V means that the vector V is transformed by M3 first,
//     followed by M2 and M1.
//
// Coordinate system: Right Handed
//
// Rotations: Counterclockwise when looking down the VAxis. All angles are in radians.

template<typename T>
class VSymMat3;

template<class T>
class VR3Matrix
{
    static const VR3Matrix IdentityValue;

public:

    T M[3][3];

        enum NoInitType { NoInit };

        // Construct with no memory initialization.
        VR3Matrix(NoInitType) { }

        // By default, we construct identity matrix.
        VR3Matrix()
        {
            SetIdentity();
        }

        VR3Matrix(T m11, T m12, T m13,
                T m21, T m22, T m23,
                T m31, T m32, T m33)
        {
            M[0][0] = m11; M[0][1] = m12; M[0][2] = m13;
            M[1][0] = m21; M[1][1] = m22; M[1][2] = m23;
            M[2][0] = m31; M[2][1] = m32; M[2][2] = m33;
        }

        /*
        explicit VR3Matrix(const VQuat<T>& q)
        {
            T ww = q.w*q.w;
            T xx = q.x*q.x;
            T yy = q.y*q.y;
            T zz = q.z*q.z;

            M[0][0] = ww + xx - yy - zz;       M[0][1] = 2 * (q.x*q.y - q.w*q.z); M[0][2] = 2 * (q.x*q.z + q.w*q.y);
            M[1][0] = 2 * (q.x*q.y + q.w*q.z); M[1][1] = ww - xx + yy - zz;       M[1][2] = 2 * (q.y*q.z - q.w*q.x);
            M[2][0] = 2 * (q.x*q.z - q.w*q.y); M[2][1] = 2 * (q.y*q.z + q.w*q.x); M[2][2] = ww - xx - yy + zz;
        }
        */





    explicit VR3Matrix(const VQuat<T>& q)
    {
        const T tx  = q.x+q.x,  ty  = q.y+q.y,  tz  = q.z+q.z;
        const T twx = q.w*tx,   twy = q.w*ty,   twz = q.w*tz;
        const T txx = q.x*tx,   txy = q.x*ty,   txz = q.x*tz;
        const T tyy = q.y*ty,   tyz = q.y*tz,   tzz = q.z*tz;
        M[0][0] = T(1) - (tyy + tzz);	M[0][1] = txy - twz;			M[0][2] = txz + twy;
        M[1][0] = txy + twz;			M[1][1] = T(1) - (txx + tzz);	M[1][2] = tyz - twx;
        M[2][0] = txz - twy;			M[2][1] = tyz + twx;			M[2][2] = T(1) - (txx + tyy);
    }

    inline explicit VR3Matrix(T s)
    {
        M[0][0] = M[1][1] = M[2][2] = s;
        M[0][1] = M[0][2] = M[1][0] = M[1][2] = M[2][0] = M[2][1] = 0;
    }





    static VR3Matrix FromString(const char* src)
    {
        VR3Matrix result;
        for (int r=0; r<3; r++)
            for (int c=0; c<3; c++)
            {
                result.M[r][c] = (T)atof(src);
                while (src && *src != ' ')
                    src++;
                while (src && *src == ' ')
                    src++;
            }
            return result;
    }

    static const VR3Matrix& Identity()  { return IdentityValue; }

    void SetIdentity()
    {
        M[0][0] = M[1][1] = M[2][2] = 1;
        M[0][1] = M[1][0] = M[2][0] = 0;
        M[0][2] = M[1][2] = M[2][1] = 0;
    }

    bool operator== (const VR3Matrix& b) const
    {
        bool isEqual = true;
        for (int i = 0; i < 3; i++)
            for (int j = 0; j < 3; j++)
                isEqual &= (M[i][j] == b.M[i][j]);

        return isEqual;
    }

    VR3Matrix operator+ (const VR3Matrix& b) const
    {
        VR4Matrix<T> result(*this);
        result += b;
        return result;
    }

    VR3Matrix& operator+= (const VR3Matrix& b)
    {
        for (int i = 0; i < 3; i++)
            for (int j = 0; j < 3; j++)
                M[i][j] += b.M[i][j];
        return *this;
    }

    void operator= (const VR3Matrix& b)
    {
        for (int i = 0; i < 3; i++)
            for (int j = 0; j < 3; j++)
                M[i][j] = b.M[i][j];
        return;
    }

    void operator= (const VSymMat3<T>& b)
    {
        for (int i = 0; i < 3; i++)
            for (int j = 0; j < 3; j++)
                M[i][j] = 0;

        M[0][0] = b.v[0];
        M[0][1] = b.v[1];
        M[0][2] = b.v[2];
        M[1][1] = b.v[3];
        M[1][2] = b.v[4];
        M[2][2] = b.v[5];

        return;
    }

    VR3Matrix operator- (const VR3Matrix& b) const
    {
        VR3Matrix result(*this);
        result -= b;
        return result;
    }

    VR3Matrix& operator-= (const VR3Matrix& b)
    {
        for (int i = 0; i < 3; i++)
            for (int j = 0; j < 3; j++)
                M[i][j] -= b.M[i][j];
        return *this;
    }

    // Multiplies two matrices into destination with minimum copying.
    // FIXME: take advantage of return value optimization instead.
    static VR3Matrix& Multiply(VR3Matrix* d, const VR3Matrix& a, const VR3Matrix& b)
    {
        vAssert((d != &a) && (d != &b));
        int i = 0;
        do {
            d->M[i][0] = a.M[i][0] * b.M[0][0] + a.M[i][1] * b.M[1][0] + a.M[i][2] * b.M[2][0];
            d->M[i][1] = a.M[i][0] * b.M[0][1] + a.M[i][1] * b.M[1][1] + a.M[i][2] * b.M[2][1];
            d->M[i][2] = a.M[i][0] * b.M[0][2] + a.M[i][1] * b.M[1][2] + a.M[i][2] * b.M[2][2];
        } while((++i) < 3);

        return *d;
    }

    VR3Matrix operator* (const VR3Matrix& b) const
    {
        VR3Matrix result(VR3Matrix::NoInit);
        Multiply(&result, *this, b);
        return result;
    }

    VR3Matrix& operator*= (const VR3Matrix& b)
    {
        return Multiply(this, VR3Matrix(*this), b);
    }

    VR3Matrix operator* (T s) const
    {
        VR3Matrix result(*this);
        result *= s;
        return result;
    }

    VR3Matrix& operator*= (T s)
    {
        for (int i = 0; i < 3; i++)
            for (int j = 0; j < 3; j++)
                M[i][j] *= s;
        return *this;
    }

    VVect3<T> operator* (const VVect3<T> &b) const
    {
        VVect3<T> result;
        result.x = M[0][0]*b.x + M[0][1]*b.y + M[0][2]*b.z;
        result.y = M[1][0]*b.x + M[1][1]*b.y + M[1][2]*b.z;
        result.z = M[2][0]*b.x + M[2][1]*b.y + M[2][2]*b.z;

        return result;
    }

    VR3Matrix operator/ (T s) const
    {
        VR3Matrix result(*this);
        result /= s;
        return result;
    }

    VR3Matrix& operator/= (T s)
    {
        for (int i = 0; i < 3; i++)
            for (int j = 0; j < 3; j++)
                M[i][j] /= s;
        return *this;
    }

    V2Vect<T> Transform(const V2Vect<T>& v) const
    {
        const T rcpZ = T(1) / ( M[2][0] * v.x + M[2][1] * v.y + M[2][2] );
        return V2Vect<T>((M[0][0] * v.x + M[0][1] * v.y + M[0][2]) * rcpZ,
                          (M[1][0] * v.x + M[1][1] * v.y + M[1][2]) * rcpZ);
    }

    VVect3<T> Transform(const VVect3<T>& v) const
    {
        return VVect3<T>(M[0][0] * v.x + M[0][1] * v.y + M[0][2] * v.z,
                          M[1][0] * v.x + M[1][1] * v.y + M[1][2] * v.z,
                          M[2][0] * v.x + M[2][1] * v.y + M[2][2] * v.z);
    }

    VR3Matrix Transposed() const
    {
        return VR3Matrix(M[0][0], M[1][0], M[2][0],
                       M[0][1], M[1][1], M[2][1],
                       M[0][2], M[1][2], M[2][2]);
    }

    void     Transpose()
    {
        *this = Transposed();
    }


    T SubDet (const uint* rows, const uint* cols) const
    {
        return M[rows[0]][cols[0]] * (M[rows[1]][cols[1]] * M[rows[2]][cols[2]] - M[rows[1]][cols[2]] * M[rows[2]][cols[1]])
             - M[rows[0]][cols[1]] * (M[rows[1]][cols[0]] * M[rows[2]][cols[2]] - M[rows[1]][cols[2]] * M[rows[2]][cols[0]])
             + M[rows[0]][cols[2]] * (M[rows[1]][cols[0]] * M[rows[2]][cols[1]] - M[rows[1]][cols[1]] * M[rows[2]][cols[0]]);
    }

    // M += a*b.t()
    inline void Rank1Add(const VVect3<T> &a, const VVect3<T> &b)
    {
        M[0][0] += a.x*b.x;		M[0][1] += a.x*b.y;		M[0][2] += a.x*b.z;
        M[1][0] += a.y*b.x;		M[1][1] += a.y*b.y;		M[1][2] += a.y*b.z;
        M[2][0] += a.z*b.x;		M[2][1] += a.z*b.y;		M[2][2] += a.z*b.z;
    }

    // M -= a*b.t()
    inline void Rank1Sub(const VVect3<T> &a, const VVect3<T> &b)
    {
        M[0][0] -= a.x*b.x;		M[0][1] -= a.x*b.y;		M[0][2] -= a.x*b.z;
        M[1][0] -= a.y*b.x;		M[1][1] -= a.y*b.y;		M[1][2] -= a.y*b.z;
        M[2][0] -= a.z*b.x;		M[2][1] -= a.z*b.y;		M[2][2] -= a.z*b.z;
    }

    inline VVect3<T> Col(int c) const
    {
        return VVect3<T>(M[0][c], M[1][c], M[2][c]);
    }

    inline VVect3<T> Row(int r) const
    {
        return VVect3<T>(M[r][0], M[r][1], M[r][2]);
    }

    inline T Determinant() const
    {
        const VR3Matrix<T>& m = *this;
        T d;

        d  = m.M[0][0] * (m.M[1][1]*m.M[2][2] - m.M[1][2] * m.M[2][1]);
        d -= m.M[0][1] * (m.M[1][0]*m.M[2][2] - m.M[1][2] * m.M[2][0]);
        d += m.M[0][2] * (m.M[1][0]*m.M[2][1] - m.M[1][1] * m.M[2][0]);

        return d;
    }

    inline VR3Matrix<T> Inverse() const
    {
        VR3Matrix<T> a;
        const  VR3Matrix<T>& m = *this;
        T d = Determinant();

        assert(d != 0);
        T s = T(1)/d;

        a.M[0][0] = s * (m.M[1][1] * m.M[2][2] - m.M[1][2] * m.M[2][1]);
        a.M[1][0] = s * (m.M[1][2] * m.M[2][0] - m.M[1][0] * m.M[2][2]);
        a.M[2][0] = s * (m.M[1][0] * m.M[2][1] - m.M[1][1] * m.M[2][0]);

        a.M[0][1] = s * (m.M[0][2] * m.M[2][1] - m.M[0][1] * m.M[2][2]);
        a.M[1][1] = s * (m.M[0][0] * m.M[2][2] - m.M[0][2] * m.M[2][0]);
        a.M[2][1] = s * (m.M[0][1] * m.M[2][0] - m.M[0][0] * m.M[2][1]);

        a.M[0][2] = s * (m.M[0][1] * m.M[1][2] - m.M[0][2] * m.M[1][1]);
        a.M[1][2] = s * (m.M[0][2] * m.M[1][0] - m.M[0][0] * m.M[1][2]);
        a.M[2][2] = s * (m.M[0][0] * m.M[1][1] - m.M[0][1] * m.M[1][0]);

        return a;
    }

};

typedef VR3Matrix<float>  VR3Matrixf;
typedef VR3Matrix<double> VR3Matrixd;

//-------------------------------------------------------------------------------------
template<typename T>
class VSymMat3
{
private:
    typedef VSymMat3<T> this_type;

public:
    typedef T Value_t;
    // Upper symmetric
    T v[6]; // _00 _01 _02 _11 _12 _22

    inline VSymMat3() {}

    inline explicit VSymMat3(T s)
    {
        v[0] = v[3] = v[5] = s;
        v[1] = v[2] = v[4] = 0;
    }

    inline explicit VSymMat3(T a00, T a01, T a02, T a11, T a12, T a22)
    {
        v[0] = a00; v[1] = a01; v[2] = a02;
        v[3] = a11; v[4] = a12;
        v[5] = a22;
    }

    static inline int Index(unsigned int i, unsigned int j)
    {
        return (i <= j) ? (3*i - i*(i+1)/2 + j) : (3*j - j*(j+1)/2 + i);
    }

    inline T operator()(int i, int j) const { return v[Index(i,j)]; }

    inline T &operator()(int i, int j) { return v[Index(i,j)]; }

    template<typename U>
    inline VSymMat3<U> CastTo() const
    {
        return VSymMat3<U>(static_cast<U>(v[0]), static_cast<U>(v[1]), static_cast<U>(v[2]),
                          static_cast<U>(v[3]), static_cast<U>(v[4]), static_cast<U>(v[5]));
    }

    inline this_type& operator+=(const this_type& b)
    {
        v[0]+=b.v[0];
        v[1]+=b.v[1];
        v[2]+=b.v[2];
        v[3]+=b.v[3];
        v[4]+=b.v[4];
        v[5]+=b.v[5];
        return *this;
    }

    inline this_type& operator-=(const this_type& b)
    {
        v[0]-=b.v[0];
        v[1]-=b.v[1];
        v[2]-=b.v[2];
        v[3]-=b.v[3];
        v[4]-=b.v[4];
        v[5]-=b.v[5];

        return *this;
    }

    inline this_type& operator*=(T s)
    {
        v[0]*=s;
        v[1]*=s;
        v[2]*=s;
        v[3]*=s;
        v[4]*=s;
        v[5]*=s;

        return *this;
    }

    inline VSymMat3 operator*(T s) const
    {
        VSymMat3 d;
        d.v[0] = v[0]*s;
        d.v[1] = v[1]*s;
        d.v[2] = v[2]*s;
        d.v[3] = v[3]*s;
        d.v[4] = v[4]*s;
        d.v[5] = v[5]*s;

        return d;
    }

    // Multiplies two matrices into destination with minimum copying.
    static VSymMat3& Multiply(VSymMat3* d, const VSymMat3& a, const VSymMat3& b)
    {
        // _00 _01 _02 _11 _12 _22

        d->v[0] = a.v[0] * b.v[0];
        d->v[1] = a.v[0] * b.v[1] + a.v[1] * b.v[3];
        d->v[2] = a.v[0] * b.v[2] + a.v[1] * b.v[4];

        d->v[3] = a.v[3] * b.v[3];
        d->v[4] = a.v[3] * b.v[4] + a.v[4] * b.v[5];

        d->v[5] = a.v[5] * b.v[5];

        return *d;
    }

    inline T Determinant() const
    {
        const this_type& m = *this;
        T d;

        d  = m(0,0) * (m(1,1)*m(2,2) - m(1,2) * m(2,1));
        d -= m(0,1) * (m(1,0)*m(2,2) - m(1,2) * m(2,0));
        d += m(0,2) * (m(1,0)*m(2,1) - m(1,1) * m(2,0));

        return d;
    }

    inline this_type Inverse() const
    {
        this_type a;
        const this_type& m = *this;
        T d = Determinant();

        assert(d != 0);
        T s = T(1)/d;

        a(0,0) = s * (m(1,1) * m(2,2) - m(1,2) * m(2,1));

        a(0,1) = s * (m(0,2) * m(2,1) - m(0,1) * m(2,2));
        a(1,1) = s * (m(0,0) * m(2,2) - m(0,2) * m(2,0));

        a(0,2) = s * (m(0,1) * m(1,2) - m(0,2) * m(1,1));
        a(1,2) = s * (m(0,2) * m(1,0) - m(0,0) * m(1,2));
        a(2,2) = s * (m(0,0) * m(1,1) - m(0,1) * m(1,0));

        return a;
    }

    inline T Trace() const { return v[0] + v[3] + v[5]; }

    // M = a*a.t()
    inline void Rank1(const VVect3<T> &a)
    {
        v[0] = a.x*a.x; v[1] = a.x*a.y; v[2] = a.x*a.z;
        v[3] = a.y*a.y; v[4] = a.y*a.z;
        v[5] = a.z*a.z;
    }

    // M += a*a.t()
    inline void Rank1Add(const VVect3<T> &a)
    {
        v[0] += a.x*a.x; v[1] += a.x*a.y; v[2] += a.x*a.z;
        v[3] += a.y*a.y; v[4] += a.y*a.z;
        v[5] += a.z*a.z;
    }

    // M -= a*a.t()
    inline void Rank1Sub(const VVect3<T> &a)
    {
        v[0] -= a.x*a.x; v[1] -= a.x*a.y; v[2] -= a.x*a.z;
        v[3] -= a.y*a.y; v[4] -= a.y*a.z;
        v[5] -= a.z*a.z;
    }
};

typedef VSymMat3<float>  VSymMat3f;
typedef VSymMat3<double> VSymMat3d;

template<typename T>
inline VR3Matrix<T> operator*(const VSymMat3<T>& a, const VSymMat3<T>& b)
{
    #define AJB_ARBC(r,c) (a(r,0)*b(0,c)+a(r,1)*b(1,c)+a(r,2)*b(2,c))
    return VR3Matrix<T>(
        AJB_ARBC(0,0), AJB_ARBC(0,1), AJB_ARBC(0,2),
        AJB_ARBC(1,0), AJB_ARBC(1,1), AJB_ARBC(1,2),
        AJB_ARBC(2,0), AJB_ARBC(2,1), AJB_ARBC(2,2));
    #undef AJB_ARBC
}

template<typename T>
inline VR3Matrix<T> operator*(const VR3Matrix<T>& a, const VSymMat3<T>& b)
{
    #define AJB_ARBC(r,c) (a(r,0)*b(0,c)+a(r,1)*b(1,c)+a(r,2)*b(2,c))
    return VR3Matrix<T>(
        AJB_ARBC(0,0), AJB_ARBC(0,1), AJB_ARBC(0,2),
        AJB_ARBC(1,0), AJB_ARBC(1,1), AJB_ARBC(1,2),
        AJB_ARBC(2,0), AJB_ARBC(2,1), AJB_ARBC(2,2));
    #undef AJB_ARBC
}

inline VVect3f GetViewMatrixPosition( VR4Matrixf const & m )
{
    return m.inverted().translation();
}

inline VVect3f GetViewMatrixForward( VR4Matrixf const & m )
{
    return VVect3f( -m.cell[2][0], -m.cell[2][1], -m.cell[2][2] ).normalized();
}

NV_NAMESPACE_END
