#pragma once

#include "VVect3.h"
#include "VVect4.h"
#include "VQuat.h"

#include <math.h>
#include <memory>

NV_NAMESPACE_BEGIN

template<class T>
class VMatrix4
{
    static const VMatrix4 IdentityValue;

public:
    T cell[4][4];

    // By default, we construct identity matrix.
    VMatrix4()
    {
        memset(cell, 0, sizeof(cell));
        cell[0][0] = cell[1][1] = cell[2][2] = cell[3][3] = 1;
    }

    VMatrix4(T m11, T m12, T m13, T m14,
             T m21, T m22, T m23, T m24,
             T m31, T m32, T m33, T m34,
             T m41, T m42, T m43, T m44)
    {
        cell[0][0] = m11; cell[0][1] = m12; cell[0][2] = m13; cell[0][3] = m14;
        cell[1][0] = m21; cell[1][1] = m22; cell[1][2] = m23; cell[1][3] = m24;
        cell[2][0] = m31; cell[2][1] = m32; cell[2][2] = m33; cell[2][3] = m34;
        cell[3][0] = m41; cell[3][1] = m42; cell[3][2] = m43; cell[3][3] = m44;
    }

    VMatrix4(T m11, T m12, T m13,
             T m21, T m22, T m23,
             T m31, T m32, T m33)
    {
        cell[0][0] = m11; cell[0][1] = m12; cell[0][2] = m13; cell[0][3] = 0;
        cell[1][0] = m21; cell[1][1] = m22; cell[1][2] = m23; cell[1][3] = 0;
        cell[2][0] = m31; cell[2][1] = m32; cell[2][2] = m33; cell[2][3] = 0;
        cell[3][0] = 0;   cell[3][1] = 0;   cell[3][2] = 0;   cell[3][3] = 1;
    }

    VMatrix4(const VQuat<T> &q)
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

    VVect3<T> xBasis() const { return VVect3<T>(cell[0][0], cell[1][0], cell[2][0]); }

    void setYBasis(const VVect3<T> &v)
    {
        cell[0][1] = v.x;
        cell[1][1] = v.y;
        cell[2][1] = v.z;
    }

    VVect3<T> yBasis() const { return VVect3<T>(cell[0][1], cell[1][1], cell[2][1]); }

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

    bool operator== (const VMatrix4 &matrix) const
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

    VMatrix4 operator + (const VMatrix4 &matrix) const
    {
        VMatrix4 result(*this);
        result += matrix;
        return result;
    }

    VMatrix4 &operator += (const VMatrix4 &matrix)
    {
        for (int i = 0; i < 4; i++) {
            for (int j = 0; j < 4; j++) {
                cell[i][j] += matrix.cell[i][j];
            }
        }
        return *this;
    }

    VMatrix4 operator - (const VMatrix4 &matrix) const
    {
        VMatrix4 result(*this);
        result -= matrix;
        return result;
    }

    VMatrix4 &operator -= (const VMatrix4 &matrix)
    {
        for (int i = 0; i < 4; i++) {
            for (int j = 0; j < 4; j++) {
                cell[i][j] -= matrix.cell[i][j];
            }
        }
        return *this;
    }

    VMatrix4 operator * (const VMatrix4 &matrix) const
    {
        VMatrix4 result;
        for (int i = 0; i < 4; i++) {
            result.cell[i][0] = cell[i][0] * matrix.cell[0][0] + cell[i][1] * matrix.cell[1][0] + cell[i][2] * matrix.cell[2][0] + cell[i][3] * matrix.cell[3][0];
            result.cell[i][1] = cell[i][0] * matrix.cell[0][1] + cell[i][1] * matrix.cell[1][1] + cell[i][2] * matrix.cell[2][1] + cell[i][3] * matrix.cell[3][1];
            result.cell[i][2] = cell[i][0] * matrix.cell[0][2] + cell[i][1] * matrix.cell[1][2] + cell[i][2] * matrix.cell[2][2] + cell[i][3] * matrix.cell[3][2];
            result.cell[i][3] = cell[i][0] * matrix.cell[0][3] + cell[i][1] * matrix.cell[1][3] + cell[i][2] * matrix.cell[2][3] + cell[i][3] * matrix.cell[3][3];
        }
        return result;
    }

    VMatrix4 &operator *= (const VMatrix4 &matrix)
    {
        *this = *this * matrix;
        return *this;
    }

    VMatrix4 operator * (T factor) const
    {
        VMatrix4 result(*this);
        result *= factor;
        return result;
    }

    VMatrix4 &operator*= (T factor)
    {
        for (T (&row)[4] : cell) {
            for (T &value : row) {
                value *= factor;
            }
        }
        return *this;
    }

    VMatrix4 operator / (T divisor) const
    {
        VMatrix4 result(*this);
        result /= divisor;
        return result;
    }

    VMatrix4 &operator /= (T factor)
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

    VMatrix4 transposed() const
    {
        return VMatrix4(cell[0][0], cell[1][0], cell[2][0], cell[3][0],
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

    VMatrix4 adjugated() const
    {
        return VMatrix4(cofactor(0,0), cofactor(1,0), cofactor(2,0), cofactor(3,0),
                        cofactor(0,1), cofactor(1,1), cofactor(2,1), cofactor(3,1),
                        cofactor(0,2), cofactor(1,2), cofactor(2,2), cofactor(3,2),
                        cofactor(0,3), cofactor(1,3), cofactor(2,3), cofactor(3,3));
    }

    VMatrix4 inverted() const { return adjugated() * (1.0f / determinant()); }
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

    static inline T abs(T value) { return value >= 0 ? value : -value; }

    // Creates a matrix that converts the vertices from one coordinate system
    // to another.
    static VMatrix4 VAxisConversion(const VWorldAxes &to, const VWorldAxes &from)
    {
        // Holds VAxis values from the 'to' structure
        int toArray[3] = {to.XAxis, to.YAxis, to.ZAxis};

        // The inverse of the toArray
        int inv[4];
        inv[0] = inv[abs(to.XAxis)] = 0;
        inv[abs(to.YAxis)] = 1;
        inv[abs(to.ZAxis)] = 2;

        VMatrix4 m(0, 0, 0,
                    0, 0, 0,
                    0, 0, 0);

        // Only three values in the matrix need to be changed to 1 or -1.
        m.cell[inv[abs(from.XAxis)]][0] = T(from.XAxis / toArray[inv[abs(from.XAxis)]]);
        m.cell[inv[abs(from.YAxis)]][1] = T(from.YAxis / toArray[inv[abs(from.YAxis)]]);
        m.cell[inv[abs(from.ZAxis)]][2] = T(from.ZAxis / toArray[inv[abs(from.ZAxis)]]);
        return m;
    }

    // Creates a matrix for translation by vector
    static VMatrix4 Translation(const VVect3<T> &v)
    {
        VMatrix4 t;
        t.cell[0][3] = v.x;
        t.cell[1][3] = v.y;
        t.cell[2][3] = v.z;
        return t;
    }

    // Creates a matrix for translation by vector
    static VMatrix4 Translation(T x, T y, T z = T(0))
    {
        VMatrix4 t;
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
    static VMatrix4 Scaling(const VVect3<T> &v)
    {
        VMatrix4 t;
        t.cell[0][0] = v.x;
        t.cell[1][1] = v.y;
        t.cell[2][2] = v.z;
        return t;
    }

    // Creates a matrix for scaling by vector
    static VMatrix4 Scaling(T x, T y, T z)
    {
        VMatrix4 t;
        t.cell[0][0] = x;
        t.cell[1][1] = y;
        t.cell[2][2] = z;
        return t;
    }

    // Creates a matrix for scaling by constant
    static VMatrix4 Scaling(T s)
    {
        VMatrix4 t;
        t.cell[0][0] = s;
        t.cell[1][1] = s;
        t.cell[2][2] = s;
        return t;
    }

       // Simple L1 distance in R^12
    T distanceTo(const VMatrix4 &matrix) const
    {
        T d = abs(cell[0][0] - matrix.cell[0][0]) + abs(cell[0][1] - matrix.cell[0][1]);
        d += abs(cell[0][2] - matrix.cell[0][2]) + abs(cell[0][3] - matrix.cell[0][3]);
        d += abs(cell[1][0] - matrix.cell[1][0]) + abs(cell[1][1] - matrix.cell[1][1]);
        d += abs(cell[1][2] - matrix.cell[1][2]) + abs(cell[1][3] - matrix.cell[1][3]);
        d += abs(cell[2][0] - matrix.cell[2][0]) + abs(cell[2][1] - matrix.cell[2][1]);
        d += abs(cell[2][2] - matrix.cell[2][2]) + abs(cell[2][3] - matrix.cell[2][3]);
        d += abs(cell[3][0] - matrix.cell[3][0]) + abs(cell[3][1] - matrix.cell[3][1]);
        d += abs(cell[3][2] - matrix.cell[3][2]) + abs(cell[3][3] - matrix.cell[3][3]);
        return d;
    }

    // Creates a rotation matrix rotating around the X VAxis by 'angle' radians.
    // Just for quick testing.  Not for final API.  Need to remove case.
    static VMatrix4 RotationVAxis(VAxis A, T angle, VRotateDirection d, VHandedSystem s)
    {
        T sina = s * d *sin(angle);
        T cosa = cos(angle);

        switch(A) {
        case VAxis_X:
            return VMatrix4(1, 0,    0,
                            0, cosa, -sina,
                            0, sina, cosa);
        case VAxis_Y:
            return VMatrix4(cosa, 0, sina,
                               0, 1,    0,
                           -sina, 0, cosa);
        case VAxis_Z:
            return VMatrix4(cosa,  -sina,  0,
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
    static VMatrix4 RotationX(T angle)
    {
        T sina = sin(angle);
        T cosa = cos(angle);
        return VMatrix4(1,  0,     0,
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
    static VMatrix4 RotationY(T angle)
    {
        T sina = sin(angle);
        T cosa = cos(angle);
        return VMatrix4(cosa, 0, sina,
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
    static VMatrix4 RotationZ(T angle)
    {
        T sina = sin(angle);
        T cosa = cos(angle);
        return VMatrix4(cosa, -sina, 0,
                         sina,  cosa, 0,
                            0,     0, 1);
    }

    static VMatrix4 RotationVAxisAngle(const VVect3<T> &vAxis, T angle)
    {
        T x = vAxis.x;
        T y = vAxis.y;
        T z = vAxis.z;
        T c = cos(angle);
        T s = sin(angle);
        T t = 1 - c;
        return VMatrix4(t * x * x + c, t * x * y - z * s, t * x * z + y * s,
                         t * x * y + z * s, t * y * y + c, t * y * z - x * s,
                         y * x * z - y * s, t * y * z + x * s, t * z * z + c);
    }

    // LookAtRH creates a View transformation matrix for right-handed coordinate system.
    // The resulting matrix points camera from 'eye' towards 'at' direction, with 'up'
    // specifying the up vector. The resulting matrix should be used with PerspectiveRH
    // projection.
    static VMatrix4 LookAtRH(const VVect3<T>& eye, const VVect3<T>& at, const VVect3<T>& up)
    {
        // FIXME: this fails when looking straight up, should probably at least assert
        VVect3<T> z = (eye - at).normalized();  // Forward
        VVect3<T> x = up.crossProduct(z).normalized(); // Right
        VVect3<T> y = z.crossProduct(x);

        return VMatrix4(x.x,  x.y,  x.z,  -(x.dotProduct(eye)),
                         y.x,  y.y,  y.z,  -(y.dotProduct(eye)),
                         z.x,  z.y,  z.z,  -(z.dotProduct(eye)),
                         0,    0,    0,    1 );
    }

    // LookAtLH creates a View transformation matrix for left-handed coordinate system.
    // The resulting matrix points camera from 'eye' towards 'at' direction, with 'up'
    // specifying the up vector.
    static VMatrix4 LookAtLH(const VVect3<T>& eye, const VVect3<T>& at, const VVect3<T>& up)
    {
        // FIXME: this fails when looking straight up, should probably at least assert
        VVect3<T> z = (at - eye).normalized();  // Forward
        VVect3<T> x = up.crossProduct(z).normalized(); // Right
        VVect3<T> y = z.crossProduct(x);

        return VMatrix4(x.x,  x.y,  x.z,  -(x.dotProduct(eye)),
                         y.x,  y.y,  y.z,  -(y.dotProduct(eye)),
                         z.x,  z.y,  z.z,  -(z.dotProduct(eye)),
                         0,    0,    0,    1 );
    }

    static VMatrix4 CreateFromBasisVectors( VVect3<T> const & zBasis, VVect3<T> const & up )
    {
        T dot = zBasis.dotProduct(up);
        if ( dot < (T)-0.9999 || dot > (T)0.9999 ) {
            return VMatrix4();
        }

        VVect3<T> xBasis = up.crossProduct( zBasis );
        xBasis.normalize();

        VVect3<T> yBasis = zBasis.crossProduct( xBasis );
           // no need to normalize yBasis because xBasis and zBasis must already be orthogonal

        return VMatrix4<T>(xBasis.x, yBasis.x, zBasis.x, (T) 0,
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
    static VMatrix4 PerspectiveRH(T yfov, T aspect, T znear, T zfar)
    {
        VMatrix4 m;
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
    static VMatrix4 PerspectiveLH(T yfov, T aspect, T znear, T zfar)
    {
        VMatrix4 m;
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

    static VMatrix4 Ortho2D(T width, T height)
    {
        VMatrix4 m;
        m.cell[0][0] = T(2) / width;
        m.cell[1][1] = T(-2) / height;
        m.cell[0][3] = T(-1);
        m.cell[1][3] = T(1);
        m.cell[2][2] = T(0);
        return m;
    }

    // Trivial version of TanAngleMatrixFromProjection() for a symmetric field of view.
    static VMatrix4<T> TanAngleMatrixFromFov(float fovDegrees)
    {
        const float tanHalfFov = tanf(0.5f * fovDegrees * (M_PI / 180.0f));
        VMatrix4<T> tanAngleMatrix(0.5f / tanHalfFov, 0.0f, -0.5f, 0.0f,
                                    0.0f, 0.5f / tanHalfFov, -0.5f, 0.0f,
                                    0.0f, 0.0f, -1.0f, 0.0f,
                                    0.0f, 0.0f, -1.0f, 0.0f);
        return tanAngleMatrix;
    }

    // Utility function to calculate external velocity for smooth stick yaw turning.
    // To reduce judder in FPS style experiences when the application framerate is
    // lower than the vsync rate, the rotation from a joypad can be applied to the
    // view space distorted eye vectors before applying the time warp.
    VMatrix4 calculateExternalVelocity(float yawRadiansPerSecond) const
    {
        const float angle = yawRadiansPerSecond * ( -1.0f / 60.0f );
        const float sinHalfAngle = sinf( angle * 0.5f );
        const float cosHalfAngle = cosf( angle * 0.5f );

        // Yaw is always going to be around the world Y axis
        VQuat<T> quat;
        quat.x = cell[0][1] * sinHalfAngle;
        quat.y = cell[1][1] * sinHalfAngle;
        quat.z = cell[2][1] * sinHalfAngle;
        quat.w = cosHalfAngle;
        return quat;
    }

    VVect3<T> viewPosition() const { return inverted().translation(); }
    VVect3<T> viewForward() const {return VVect3<T>(-cell[2][0], -cell[2][1], -cell[2][2]).normalized(); }

    VMatrix4 tanAngleMatrixFromUnitSquare() const
    {
        const VMatrix4 inv = inverted();
        VMatrix4 m;
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
};

typedef VMatrix4<float> VMatrix4f;
typedef VMatrix4<double> VMatrix4d;

NV_NAMESPACE_END
