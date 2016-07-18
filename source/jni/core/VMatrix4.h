#pragma once

#include "VVect3.h"
#include "VVect4.h"

#include <math.h>
#include <memory>

NV_NAMESPACE_BEGIN

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

    static inline T abs(T value) { return value >= 0 ? value : -value; }

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
        T dot = zBasis.dotProduct(up);
        if ( dot < (T)-0.9999 || dot > (T)0.9999 ) {
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

    VVect3<T> viewPosition() const { return inverted().translation(); }
    VVect3<T> viewForward() const {return VVect3<T>(-cell[2][0], -cell[2][1], -cell[2][2]).normalized(); }
};

typedef VR4Matrix<float> VR4Matrixf;
typedef VR4Matrix<double> VR4Matrixd;

NV_NAMESPACE_END
