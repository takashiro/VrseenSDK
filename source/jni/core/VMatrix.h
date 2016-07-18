#pragma once

#include "VConstants.h"
#include "VVect3.h"
#include "VVect4.h"

#include "VMatrix4.h"

#include "VLog.h"

#include <assert.h>
#include <stdlib.h>
#include <math.h>

NV_NAMESPACE_BEGIN

template <class T> class VR4Matrix;


template<class T> class VQuat;
template<class T> class VPos;

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

NV_NAMESPACE_END
