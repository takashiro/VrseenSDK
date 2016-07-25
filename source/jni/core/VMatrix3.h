#pragma once

#include "VQuat.h"
#include "VVect2.h"

#include <memory.h>

NV_NAMESPACE_BEGIN

template<class T>
class VMatrix3
{
public:
    T cell[3][3];

    VMatrix3()
    {
        memset(cell, 0, sizeof(cell));
        cell[0][0] = cell[1][1] = cell[2][2] = 1;
    }

    VMatrix3(T m11, T m12, T m13,
             T m21, T m22, T m23,
             T m31, T m32, T m33)
    {
        cell[0][0] = m11; cell[0][1] = m12; cell[0][2] = m13;
        cell[1][0] = m21; cell[1][1] = m22; cell[1][2] = m23;
        cell[2][0] = m31; cell[2][1] = m32; cell[2][2] = m33;
    }

    explicit VMatrix3(const VQuat<T> &q)
    {
        const T tx  = q.x + q.x, ty  = q.y + q.y, tz  = q.z + q.z;
        const T twx = q.w * tx,  twy = q.w * ty,  twz = q.w * tz;
        const T txx = q.x * tx,  txy = q.x * ty,  txz = q.x * tz;
        const T tyy = q.y * ty,  tyz = q.y * tz,  tzz = q.z * tz;
        cell[0][0] = T(1) - (tyy + tzz);
        cell[0][1] = txy - twz;
        cell[0][2] = txz + twy;
        cell[1][0] = txy + twz;
        cell[1][1] = T(1) - (txx + tzz);
        cell[1][2] = tyz - twx;
        cell[2][0] = txz - twy;
        cell[2][1] = tyz + twx;
        cell[2][2] = T(1) - (txx + tyy);
    }

    inline explicit VMatrix3(T value)
    {
        memset(cell, 0, sizeof(cell));
        cell[0][0] = cell[1][1] = cell[2][2] = value;
    }

    bool operator == (const VMatrix3 &matrix) const
    {
        for (int i = 0; i < 3; i++) {
            for (int j = 0; j < 3; j++) {
                if (cell[i][j] != matrix.cell[i][j]) {
                    return false;
                }
            }
        }
        return true;
    }

    VMatrix3 operator + (const VMatrix3 &matrix) const
    {
        VMatrix3 result(*this);
        result += matrix;
        return result;
    }

    VMatrix3 &operator += (const VMatrix3 &matrix)
    {
        for (int i = 0; i < 3; i++) {
            for (int j = 0; j < 3; j++) {
                cell[i][j] += matrix.cell[i][j];
            }
        }
        return *this;
    }

    VMatrix3 operator - (const VMatrix3 &matrix) const
    {
        VMatrix3 result(*this);
        result -= matrix;
        return result;
    }

    VMatrix3 &operator -= (const VMatrix3 &matrix)
    {
        for (int i = 0; i < 3; i++) {
            for (int j = 0; j < 3; j++) {
                cell[i][j] -= matrix.cell[i][j];
            }
        }
        return *this;
    }

    VMatrix3 operator * (const VMatrix3 &matrix) const
    {
        VMatrix3 result;
        for (int i = 0; i < 3; i++) {
            result.cell[i][0] = cell[i][0] * matrix.cell[0][0] + cell[i][1] * matrix.cell[1][0] + cell[i][2] * matrix.cell[2][0];
            result.cell[i][1] = cell[i][0] * matrix.cell[0][1] + cell[i][1] * matrix.cell[1][1] + cell[i][2] * matrix.cell[2][1];
            result.cell[i][2] = cell[i][0] * matrix.cell[0][2] + cell[i][1] * matrix.cell[1][2] + cell[i][2] * matrix.cell[2][2];
        }
        return result;
    }

    VMatrix3 &operator *= (const VMatrix3 &matrix)
    {
        *this = *this * matrix;
    }

    VMatrix3 operator * (T factor) const
    {
        VMatrix3 result(*this);
        result *= factor;
        return result;
    }

    VMatrix3 &operator *= (T factor)
    {
        for (T (&row)[3] : cell) {
            for (T &value : row) {
                value *= factor;
            }
        }
        return *this;
    }

    VVect3<T> operator * (const VVect3<T> &vect) const
    {
        VVect3<T> result;
        result.x = cell[0][0] * vect.x + cell[0][1] * vect.y + cell[0][2] * vect.z;
        result.y = cell[1][0] * vect.x + cell[1][1] * vect.y + cell[1][2] * vect.z;
        result.z = cell[2][0] * vect.x + cell[2][1] * vect.y + cell[2][2] * vect.z;
        return result;
    }

    VMatrix3 operator / (T divisor) const
    {
        VMatrix3 result(*this);
        result /= divisor;
        return result;
    }

    VMatrix3 &operator /= (T divisor)
    {
        for (T (&row)[3] : cell) {
            for (T &value : row) {
                value /= divisor;
            }
        }
        return *this;
    }

    VVect2<T> transform(const VVect2<T> &vect) const
    {
        const T rcpZ = T(1) / (cell[2][0] * vect.x + cell[2][1] * vect.y + cell[2][2]);
        return VVect2<T>((cell[0][0] * vect.x + cell[0][1] * vect.y + cell[0][2]) * rcpZ,
                         (cell[1][0] * vect.x + cell[1][1] * vect.y + cell[1][2]) * rcpZ);
    }

    VVect3<T> transform(const VVect3<T>& v) const
    {
        return VVect3<T>(cell[0][0] * v.x + cell[0][1] * v.y + cell[0][2] * v.z,
                         cell[1][0] * v.x + cell[1][1] * v.y + cell[1][2] * v.z,
                         cell[2][0] * v.x + cell[2][1] * v.y + cell[2][2] * v.z);
    }

    VMatrix3 transposed() const
    {
        return VMatrix3(cell[0][0], cell[1][0], cell[2][0],
                         cell[0][1], cell[1][1], cell[2][1],
                         cell[0][2], cell[1][2], cell[2][2]);
    }

    void transpose()
    {
        for (int i = 0; i < 3; i++) {
            for (int j = i + 1; j < 3; j++) {
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

    // M += a*b.t()
    inline void rank1Add(const VVect3<T> &a, const VVect3<T> &b)
    {
        cell[0][0] += a.x * b.x;
        cell[0][1] += a.x * b.y;
        cell[0][2] += a.x * b.z;
        cell[1][0] += a.y * b.x;
        cell[1][1] += a.y * b.y;
        cell[1][2] += a.y * b.z;
        cell[2][0] += a.z * b.x;
        cell[2][1] += a.z * b.y;
        cell[2][2] += a.z * b.z;
    }

    // M -= a*b.t()
    inline void rank1Sub(const VVect3<T> &a, const VVect3<T> &b)
    {
        cell[0][0] -= a.x * b.x;
        cell[0][1] -= a.x * b.y;
        cell[0][2] -= a.x * b.z;
        cell[1][0] -= a.y * b.x;
        cell[1][1] -= a.y * b.y;
        cell[1][2] -= a.y * b.z;
        cell[2][0] -= a.z * b.x;
        cell[2][1] -= a.z * b.y;
        cell[2][2] -= a.z * b.z;
    }

    inline VVect3<T> col(int i) const
    {
        return VVect3<T>(cell[0][i], cell[1][i], cell[2][i]);
    }

    inline VVect3<T> row(int i) const
    {
        return VVect3<T>(cell[i][0], cell[i][1], cell[i][2]);
    }

    inline T determinant() const
    {
        T d;
        d  = cell[0][0] * (cell[1][1] * cell[2][2] - cell[1][2] * cell[2][1]);
        d -= cell[0][1] * (cell[1][0] * cell[2][2] - cell[1][2] * cell[2][0]);
        d += cell[0][2] * (cell[1][0] * cell[2][1] - cell[1][1] * cell[2][0]);
        return d;
    }

    inline VMatrix3<T> inverse() const
    {
        T s = T(1) / determinant();

        VMatrix3<T> a;
        a.cell[0][0] = s * (cell[1][1] * cell[2][2] - cell[1][2] * cell[2][1]);
        a.cell[1][0] = s * (cell[1][2] * cell[2][0] - cell[1][0] * cell[2][2]);
        a.cell[2][0] = s * (cell[1][0] * cell[2][1] - cell[1][1] * cell[2][0]);
        a.cell[0][1] = s * (cell[0][2] * cell[2][1] - cell[0][1] * cell[2][2]);
        a.cell[1][1] = s * (cell[0][0] * cell[2][2] - cell[0][2] * cell[2][0]);
        a.cell[2][1] = s * (cell[0][1] * cell[2][0] - cell[0][0] * cell[2][1]);
        a.cell[0][2] = s * (cell[0][1] * cell[1][2] - cell[0][2] * cell[1][1]);
        a.cell[1][2] = s * (cell[0][2] * cell[1][0] - cell[0][0] * cell[1][2]);
        a.cell[2][2] = s * (cell[0][0] * cell[1][1] - cell[0][1] * cell[1][0]);
        return a;
    }
};

typedef VMatrix3<float> VMatrix3f;
typedef VMatrix3<double> VMatrix3d;

NV_NAMESPACE_END
