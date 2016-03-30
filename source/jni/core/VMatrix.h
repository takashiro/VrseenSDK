#pragma once

#include "vglobal.h"
#include "VVector.h"
#include "VTransform.h"
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
    T M[4][4];

        enum NoInitType { NoInit };

        // Construct with no memory initialization.
        VR4Matrix(NoInitType) { }

        // By default, we construct identity matrix.
        VR4Matrix()
        {
            SetIdentity();
        }

        VR4Matrix(T m11, T m12, T m13, T m14,
                T m21, T m22, T m23, T m24,
                T m31, T m32, T m33, T m34,
                T m41, T m42, T m43, T m44)
        {
            M[0][0] = m11; M[0][1] = m12; M[0][2] = m13; M[0][3] = m14;
            M[1][0] = m21; M[1][1] = m22; M[1][2] = m23; M[1][3] = m24;
            M[2][0] = m31; M[2][1] = m32; M[2][2] = m33; M[2][3] = m34;
            M[3][0] = m41; M[3][1] = m42; M[3][2] = m43; M[3][3] = m44;
        }

        VR4Matrix(T m11, T m12, T m13,
                T m21, T m22, T m23,
                T m31, T m32, T m33)
        {
            M[0][0] = m11; M[0][1] = m12; M[0][2] = m13; M[0][3] = 0;
            M[1][0] = m21; M[1][1] = m22; M[1][2] = m23; M[1][3] = 0;
            M[2][0] = m31; M[2][1] = m32; M[2][2] = m33; M[2][3] = 0;
            M[3][0] = 0;   M[3][1] = 0;   M[3][2] = 0;   M[3][3] = 1;
        }

        explicit VR4Matrix(const VQuat<T>& q)
        {
    		FromQuat( q );
        }

        explicit VR4Matrix(const VPos<T>& p)
        {
    		FromPose( p );
        }





   	void FromQuat( const VQuat<T> & q )
   	{
           T ww = q.w*q.w;
           T xx = q.x*q.x;
           T yy = q.y*q.y;
           T zz = q.z*q.z;

           M[0][0] = ww + xx - yy - zz;       M[0][1] = 2 * (q.x*q.y - q.w*q.z); M[0][2] = 2 * (q.x*q.z + q.w*q.y); M[0][3] = 0;
           M[1][0] = 2 * (q.x*q.y + q.w*q.z); M[1][1] = ww - xx + yy - zz;       M[1][2] = 2 * (q.y*q.z - q.w*q.x); M[1][3] = 0;
           M[2][0] = 2 * (q.x*q.z - q.w*q.y); M[2][1] = 2 * (q.y*q.z + q.w*q.x); M[2][2] = ww - xx - yy + zz;       M[2][3] = 0;
           M[3][0] = 0;                       M[3][1] = 0;                       M[3][2] = 0;                       M[3][3] = 1;
   	}

   	// allows construction without an extra copy
   	void FromPose( const VPos<T> & p )
   	{
   		FromQuat( p.Orientation );
   		SetTranslation( p.Position );
   	}

       static VR4Matrix FromString(const char* src)
       {
           VR4Matrix result;
           for (int r=0; r<4; r++)
               for (int c=0; c<4; c++)
               {
                   result.M[r][c] = (T)atof(src);
                   while (src && *src != ' ')
                       src++;
                   while (src && *src == ' ')
                       src++;
               }
           return result;
       }

       static const VR4Matrix& Identity()  { return IdentityValue; }

       void SetIdentity()
       {
           M[0][0] = M[1][1] = M[2][2] = M[3][3] = 1;
           M[0][1] = M[1][0] = M[2][3] = M[3][1] = 0;
           M[0][2] = M[1][2] = M[2][0] = M[3][2] = 0;
           M[0][3] = M[1][3] = M[2][1] = M[3][0] = 0;
       }

   	void SetXBasis( const V3Vectf & v )
   	{
   		M[0][0] = v.x;
   		M[1][0] = v.y;
   		M[2][0] = v.z;
   	}
   	V3Vectf GetXBasis() const
   	{
   		return V3Vectf( M[0][0], M[1][0], M[2][0] );
   	}

   	void SetYBasis( const V3Vectf & v )
   	{
   		M[0][1] = v.x;
   		M[1][1] = v.y;
   		M[2][1] = v.z;
   	}
   	V3Vectf GetYBasis() const
   	{
   		return V3Vectf( M[0][1], M[1][1], M[2][1] );
   	}

   	void SetZBasis( const V3Vectf & v )
   	{
   		M[0][2] = v.x;
   		M[1][2] = v.y;
   		M[2][2] = v.z;
   	}
   	V3Vectf GetZBasis() const
   	{
   		return V3Vectf( M[0][2], M[1][2], M[2][2] );
   	}


   	bool operator== (const VR4Matrix& b) const
   	{
   		bool isEqual = true;
           for (int i = 0; i < 4; i++)
               for (int j = 0; j < 4; j++)
                   isEqual &= (M[i][j] == b.M[i][j]);

   		return isEqual;
   	}

       VR4Matrix operator+ (const VR4Matrix& b) const
       {
           VR4Matrix result(*this);
           result += b;
           return result;
       }

       VR4Matrix& operator+= (const VR4Matrix& b)
       {
           for (int i = 0; i < 4; i++)
               for (int j = 0; j < 4; j++)
                   M[i][j] += b.M[i][j];
           return *this;
       }

       VR4Matrix operator- (const VR4Matrix& b) const
       {
           VR4Matrix result(*this);
           result -= b;
           return result;
       }

       VR4Matrix& operator-= (const VR4Matrix& b)
       {
           for (int i = 0; i < 4; i++)
               for (int j = 0; j < 4; j++)
                   M[i][j] -= b.M[i][j];
           return *this;
       }

       // Multiplies two matrices into destination with minimum copying.
   	// FIXME: take advantage of return value optimization instead.
       static VR4Matrix& Multiply(VR4Matrix* d, const VR4Matrix& a, const VR4Matrix& b)
       {
           OVR_ASSERT((d != &a) && (d != &b));
           int i = 0;
           do {
               d->M[i][0] = a.M[i][0] * b.M[0][0] + a.M[i][1] * b.M[1][0] + a.M[i][2] * b.M[2][0] + a.M[i][3] * b.M[3][0];
               d->M[i][1] = a.M[i][0] * b.M[0][1] + a.M[i][1] * b.M[1][1] + a.M[i][2] * b.M[2][1] + a.M[i][3] * b.M[3][1];
               d->M[i][2] = a.M[i][0] * b.M[0][2] + a.M[i][1] * b.M[1][2] + a.M[i][2] * b.M[2][2] + a.M[i][3] * b.M[3][2];
               d->M[i][3] = a.M[i][0] * b.M[0][3] + a.M[i][1] * b.M[1][3] + a.M[i][2] * b.M[2][3] + a.M[i][3] * b.M[3][3];
           } while((++i) < 4);

           return *d;
       }

       VR4Matrix operator* (const VR4Matrix& b) const
       {
           VR4Matrix result(VR4Matrix::NoInit);
           Multiply(&result, *this, b);
           return result;
       }

       VR4Matrix& operator*= (const VR4Matrix& b)
       {
           return Multiply(this, VR4Matrix(*this), b);
       }

       VR4Matrix operator* (T s) const
       {
           VR4Matrix result(*this);
           result *= s;
           return result;
       }

       VR4Matrix& operator*= (T s)
       {
           for (int i = 0; i < 4; i++)
               for (int j = 0; j < 4; j++)
                   M[i][j] *= s;
           return *this;
       }


       VR4Matrix operator/ (T s) const
       {
           VR4Matrix result(*this);
           result /= s;
           return result;
       }

       VR4Matrix& operator/= (T s)
       {
           for (int i = 0; i < 4; i++)
               for (int j = 0; j < 4; j++)
                   M[i][j] /= s;
           return *this;
       }

       V3Vect<T> Transform(const V3Vect<T>& v) const
       {
   		const T rcpW = T(1) / ( M[3][0] * v.x + M[3][1] * v.y + M[3][2] * v.z + M[3][3] );
           return V3Vect<T>((M[0][0] * v.x + M[0][1] * v.y + M[0][2] * v.z + M[0][3]) * rcpW,
                             (M[1][0] * v.x + M[1][1] * v.y + M[1][2] * v.z + M[1][3]) * rcpW,
                             (M[2][0] * v.x + M[2][1] * v.y + M[2][2] * v.z + M[2][3]) * rcpW);
       }

       V4Vect<T> Transform(const V4Vect<T>& v) const
       {
           return V4Vect<T>(M[0][0] * v.x + M[0][1] * v.y + M[0][2] * v.z + M[0][3] * v.w,
                             M[1][0] * v.x + M[1][1] * v.y + M[1][2] * v.z + M[1][3] * v.w,
                             M[2][0] * v.x + M[2][1] * v.y + M[2][2] * v.z + M[2][3] * v.w,
   						  M[3][0] * v.x + M[3][1] * v.y + M[3][2] * v.z + M[3][3] * v.w);
       }

       VR4Matrix Transposed() const
       {
           return VR4Matrix(M[0][0], M[1][0], M[2][0], M[3][0],
                           M[0][1], M[1][1], M[2][1], M[3][1],
                           M[0][2], M[1][2], M[2][2], M[3][2],
                           M[0][3], M[1][3], M[2][3], M[3][3]);
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

       T Cofactor(uint I, uint J) const
       {
           const uint indices[4][3] = {{1,2,3},{0,2,3},{0,1,3},{0,1,2}};
           return ((I+J)&1) ? -SubDet(indices[I],indices[J]) : SubDet(indices[I],indices[J]);
       }

       T Determinant() const
       {
           return M[0][0] * Cofactor(0,0) + M[0][1] * Cofactor(0,1) + M[0][2] * Cofactor(0,2) + M[0][3] * Cofactor(0,3);
       }

       VR4Matrix Adjugated() const
       {
           return VR4Matrix(Cofactor(0,0), Cofactor(1,0), Cofactor(2,0), Cofactor(3,0),
                           Cofactor(0,1), Cofactor(1,1), Cofactor(2,1), Cofactor(3,1),
                           Cofactor(0,2), Cofactor(1,2), Cofactor(2,2), Cofactor(3,2),
                           Cofactor(0,3), Cofactor(1,3), Cofactor(2,3), Cofactor(3,3));
       }

       VR4Matrix Inverted() const
       {
           T det = Determinant();
           assert(det != 0);
           return Adjugated() * (1.0f/det);
       }

       void Invert()
       {
           *this = Inverted();
       }

   	// This is more efficient than general inverse, but ONLY works
   	// correctly if it is a homogeneous transform matrix (rot + trans)
   	VR4Matrix InvertedHomogeneousTransform() const
   	{
   		// Make the inverse rotation matrix
   		VR4Matrix rinv = this->Transposed();
   		rinv.M[3][0] = rinv.M[3][1] = rinv.M[3][2] = 0.0f;
   		// Make the inverse translation matrix
   		V3Vect<T> tvinv(-M[0][3],-M[1][3],-M[2][3]);
   		VR4Matrix tinv = VR4Matrix::Translation(tvinv);
   		return rinv * tinv;  // "untranslate", then "unrotate"
   	}

   	// This is more efficient than general inverse, but ONLY works
   	// correctly if it is a homogeneous transform matrix (rot + trans)
   	void InvertHomogeneousTransform()
   	{
           *this = InvertedHomogeneousTransform();
   	}

   	// Matrix to Euler Angles conversion
       // a,b,c, are the YawPitchRoll angles to be returned
       // rotation a around VAxis A1
       // is followed by rotation b around VAxis A2
       // is followed by rotation c around VAxis A3
       // rotations are CCW or CW (D) in LH or RH coordinate system (S)
       template <VAxis A1, VAxis A2, VAxis A3, VRotateDirection D, VHandedSystem S>
       void ToEulerAngles(T *a, T *b, T *c) const
       {
           OVR_COMPILER_ASSERT((A1 != A2) && (A2 != A3) && (A1 != A3));

           T psign = -1;
           if (((A1 + 1) % 3 == A2) && ((A2 + 1) % 3 == A3)) // Determine whether even permutation
           psign = 1;

           T pm = psign*M[A1][A3];
           if (pm < -1.0f + VConstants<T>::SingularityRadius)
           { // South pole singularity
               *a = 0;
               *b = -S*D*VConstants<T>::Pi/2.0;
               *c = S*D*atan2( psign*M[A2][A1], M[A2][A2] );
           }
           else if (pm > 1.0f - VConstants<T>::SingularityRadius)
           { // North pole singularity
               *a = 0;
               *b = S*D*VConstants<T>::Pi/2.0;
               *c = S*D*atan2( psign*M[A2][A1], M[A2][A2] );
           }
           else
           { // Normal case (nonsingular)
               *a = S*D*atan2( -psign*M[A2][A3], M[A3][A3] );
               *b = S*D*asin(pm);
               *c = S*D*atan2( -psign*M[A1][A2], M[A1][A1] );
           }

           return;
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
            OVR_COMPILER_ASSERT(A1 != A2);

           // Determine the VAxis that was not supplied
           int m = 3 - A1 - A2;

           T psign = -1;
           if ((A1 + 1) % 3 == A2) // Determine whether even permutation
               psign = 1.0f;

           T c2 = M[A1][A1];
           if (c2 < -1 + VConstants<T>::SingularityRadius)
           { // South pole singularity
               *a = 0;
               *b = S*D*VConstants<T>::Pi;
               *c = S*D*atan2( -psign*M[A2][m],M[A2][A2]);
           }
           else if (c2 > 1.0f - VConstants<T>::SingularityRadius)
           { // North pole singularity
               *a = 0;
               *b = 0;
               *c = S*D*atan2( -psign*M[A2][m],M[A2][A2]);
           }
           else
           { // Normal case (nonsingular)
               *a = S*D*atan2( M[A2][A1],-psign*M[m][A1]);
               *b = S*D*acos(c2);
               *c = S*D*atan2( M[A1][A2],psign*M[A1][m]);
           }
           return;
       }

       // Creates a matrix that converts the vertices from one coordinate system
       // to another.
       static VR4Matrix VAxisConversion(const VWorldAxes& to, const VWorldAxes& from)
       {
           // Holds VAxis values from the 'to' structure
           int toArray[3] = { to.XAxis, to.YAxis, to.ZAxis };

           // The inverse of the toArray
           int inv[4];
           inv[0] = inv[abs(to.XAxis)] = 0;
           inv[abs(to.YAxis)] = 1;
           inv[abs(to.ZAxis)] = 2;

           VR4Matrix m(0,  0,  0,
                     0,  0,  0,
                     0,  0,  0);

           // Only three values in the matrix need to be changed to 1 or -1.
           m.M[inv[abs(from.XAxis)]][0] = T(from.XAxis/toArray[inv[abs(from.XAxis)]]);
           m.M[inv[abs(from.YAxis)]][1] = T(from.YAxis/toArray[inv[abs(from.YAxis)]]);
           m.M[inv[abs(from.ZAxis)]][2] = T(from.ZAxis/toArray[inv[abs(from.ZAxis)]]);
           return m;
       }


   	// Creates a matrix for translation by vector
       static VR4Matrix Translation(const V3Vect<T>& v)
       {
           VR4Matrix t;
           t.M[0][3] = v.x;
           t.M[1][3] = v.y;
           t.M[2][3] = v.z;
           return t;
       }

   	// Creates a matrix for translation by vector
       static VR4Matrix Translation(T x, T y, T z = T(0))
       {
           VR4Matrix t;
           t.M[0][3] = x;
           t.M[1][3] = y;
           t.M[2][3] = z;
           return t;
       }

   	// Sets the translation part
       void SetTranslation(const V3Vect<T>& v)
       {
           M[0][3] = v.x;
           M[1][3] = v.y;
           M[2][3] = v.z;
       }

       V3Vect<T> GetTranslation() const
       {
           return V3Vect<T>( M[0][3], M[1][3], M[2][3] );
       }

   	// Creates a matrix for scaling by vector
       static VR4Matrix Scaling(const V3Vect<T>& v)
       {
           VR4Matrix t;
           t.M[0][0] = v.x;
           t.M[1][1] = v.y;
           t.M[2][2] = v.z;
           return t;
       }

   	// Creates a matrix for scaling by vector
       static VR4Matrix Scaling(T x, T y, T z)
       {
           VR4Matrix t;
           t.M[0][0] = x;
           t.M[1][1] = y;
           t.M[2][2] = z;
           return t;
       }

   	// Creates a matrix for scaling by constant
       static VR4Matrix Scaling(T s)
       {
           VR4Matrix t;
           t.M[0][0] = s;
           t.M[1][1] = s;
           t.M[2][2] = s;
           return t;
       }

       // Simple L1 distance in R^12
   	T Distance(const VR4Matrix& m2) const
   	{
   		T d = fabs(M[0][0] - m2.M[0][0]) + fabs(M[0][1] - m2.M[0][1]);
   		d += fabs(M[0][2] - m2.M[0][2]) + fabs(M[0][3] - m2.M[0][3]);
   		d += fabs(M[1][0] - m2.M[1][0]) + fabs(M[1][1] - m2.M[1][1]);
   		d += fabs(M[1][2] - m2.M[1][2]) + fabs(M[1][3] - m2.M[1][3]);
   		d += fabs(M[2][0] - m2.M[2][0]) + fabs(M[2][1] - m2.M[2][1]);
   		d += fabs(M[2][2] - m2.M[2][2]) + fabs(M[2][3] - m2.M[2][3]);
   		d += fabs(M[3][0] - m2.M[3][0]) + fabs(M[3][1] - m2.M[3][1]);
   		d += fabs(M[3][2] - m2.M[3][2]) + fabs(M[3][3] - m2.M[3][3]);
   		return d;
   	}

       // Creates a rotation matrix rotating around the X VAxis by 'angle' radians.
       // Just for quick testing.  Not for final API.  Need to remove case.
       static VR4Matrix RotationVAxis(VAxis A, T angle, VRotateDirection d, VHandedSystem s)
       {
           T sina = s * d *sin(angle);
           T cosa = cos(angle);

           switch(A)
           {
           case VAxis_X:
               return VR4Matrix(1,  0,     0,
                              0,  cosa,  -sina,
                              0,  sina,  cosa);
           case VAxis_Y:
               return VR4Matrix(cosa,  0,   sina,
                              0,     1,   0,
                              -sina, 0,   cosa);
           case VAxis_Z:
               return VR4Matrix(cosa,  -sina,  0,
                              sina,  cosa,   0,
                              0,     0,      1);
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
           return VR4Matrix(cosa,  0,   sina,
                          0,     1,   0,
                          -sina, 0,   cosa);
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
           return VR4Matrix(cosa,  -sina,  0,
                          sina,  cosa,   0,
                          0,     0,      1);
       }

       static VR4Matrix RotationVAxisAngle(const V3Vect<T> &VAxis, T angle) {
   	T x = VAxis.x;
   	T y = VAxis.y;
   	T z = VAxis.z;
   	T c = cos(angle);
   	T s = sin(angle);
   	T t = 1 - c;
   	VR4Matrix m(
   		t*x*x+c,   t*x*y-z*s, t*x*z+y*s,
   		t*x*y+z*s, t*y*y+c,   t*y*z-x*s,
   		y*x*z-y*s, t*y*z+x*s, t*z*z+c);
   	return m;
       }

       // LookAtRH creates a View transformation matrix for right-handed coordinate system.
       // The resulting matrix points camera from 'eye' towards 'at' direction, with 'up'
       // specifying the up vector. The resulting matrix should be used with PerspectiveRH
       // projection.
       static VR4Matrix LookAtRH(const V3Vect<T>& eye, const V3Vect<T>& at, const V3Vect<T>& up)
       {
   		// FIXME: this fails when looking straight up, should probably at least assert
           V3Vect<T> z = (eye - at).Normalized();  // Forward
           V3Vect<T> x = up.Cross(z).Normalized(); // Right
           V3Vect<T> y = z.Cross(x);

           VR4Matrix m(x.x,  x.y,  x.z,  -(x.Dot(eye)),
                     y.x,  y.y,  y.z,  -(y.Dot(eye)),
                     z.x,  z.y,  z.z,  -(z.Dot(eye)),
                     0,    0,    0,    1 );
           return m;
       }

       // LookAtLH creates a View transformation matrix for left-handed coordinate system.
       // The resulting matrix points camera from 'eye' towards 'at' direction, with 'up'
       // specifying the up vector.
       static VR4Matrix LookAtLH(const V3Vect<T>& eye, const V3Vect<T>& at, const V3Vect<T>& up)
       {
   		// FIXME: this fails when looking straight up, should probably at least assert
           V3Vect<T> z = (at - eye).Normalized();  // Forward
           V3Vect<T> x = up.Cross(z).Normalized(); // Right
           V3Vect<T> y = z.Cross(x);

           VR4Matrix m(x.x,  x.y,  x.z,  -(x.Dot(eye)),
                     y.x,  y.y,  y.z,  -(y.Dot(eye)),
                     z.x,  z.y,  z.z,  -(z.Dot(eye)),
                     0,    0,    0,    1 );
           return m;
       }

       static VR4Matrix CreateFromBasisVectors( V3Vect<T> const & zBasis, V3Vect<T> const & up )
       {
   	    OVR_ASSERT( zBasis.IsNormalized() );
   	    OVR_ASSERT( up.IsNormalized() );
   	    T dot = zBasis.Dot( up );
   	    if ( dot < (T)-0.9999 || dot > (T)0.9999 )
   	    {
   		    // z basis cannot be parallel to the specified up
   		    OVR_ASSERT( dot >= (T)-0.9999 || dot <= (T)0.9999 );
   		    return VR4Matrix<T>();
   	    }

   	    V3Vect<T> xBasis = up.Cross( zBasis );
   	    xBasis.Normalize();

   	    V3Vect<T> yBasis = zBasis.Cross( xBasis );
           // no need to normalize yBasis because xBasis and zBasis must already be orthogonal

   	    return VR4Matrix<T>(	xBasis.x, yBasis.x, zBasis.x, (T)0,
   						    xBasis.y, yBasis.y, zBasis.y, (T)0,
   						    xBasis.z, yBasis.z, zBasis.z, (T)0,
   						        (T)0,     (T)0,     (T)0, (T)1 );
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

           m.M[0][0] = T(1) / (aspect * tanHalfFov);
           m.M[1][1] = T(1) / tanHalfFov;
           m.M[2][2] = zfar / (znear - zfar);
           // m.M[2][2] = zfar / (zfar - znear);
           m.M[3][2] = T(-1);
           m.M[2][3] = (zfar * znear) / (znear - zfar);
           m.M[3][3] = T(0);

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

           m.M[0][0] = T(1) / (aspect * tanHalfFov);
           m.M[1][1] = T(1) / tanHalfFov;
           m.M[2][2] = zfar / (zfar - znear);
           m.M[3][2] = T(1);
           m.M[2][3] = (zfar * znear) / (znear - zfar);
           m.M[3][3] = T(0);

           // Note: Post-projection matrix result assumes Left-Handed coordinate system,
           //       with Y up, X right and Z forward. This supports positive z-buffer values.
           return m;
       }

       static VR4Matrix Ortho2D(T w, T h)
       {
           VR4Matrix m;
           m.M[0][0] = T(2) / w;
           m.M[1][1] = T(-2) / h;
           m.M[0][3] = T(-1);
           m.M[1][3] = T(1);
           m.M[2][2] = T(0);
           return m;
       }

       // Returns a 3x3 minor of a 4x4 matrix.
       static T Matrix4_Minor( const VR4Matrix<T> * m, int r0, int r1, int r2, int c0, int c1, int c2 )
       {
           return	m->M[r0][c0] * ( m->M[r1][c1] * m->M[r2][c2] - m->M[r2][c1] * m->M[r1][c2] ) -
                     m->M[r0][c1] * ( m->M[r1][c0] * m->M[r2][c2] - m->M[r2][c0] * m->M[r1][c2] ) +
                     m->M[r0][c2] * ( m->M[r1][c0] * m->M[r2][c1] - m->M[r2][c0] * m->M[r1][c1] );
       }

       // Returns the inverse of a 4x4 matrix.
       static VR4Matrix<T> Matrix4_Inverse( const VR4Matrix<T> * m )
       {
           const float rcpDet = 1.0f / (	m->M[0][0] * Matrix4_Minor( m, 1, 2, 3, 1, 2, 3 ) -
                                            m->M[0][1] * Matrix4_Minor( m, 1, 2, 3, 0, 2, 3 ) +
                                            m->M[0][2] * Matrix4_Minor( m, 1, 2, 3, 0, 1, 3 ) -
                                            m->M[0][3] * Matrix4_Minor( m, 1, 2, 3, 0, 1, 2 ) );
           VR4Matrix<T> out;
           out.M[0][0] =  Matrix4_Minor( m, 1, 2, 3, 1, 2, 3 ) * rcpDet;
           out.M[0][1] = -Matrix4_Minor( m, 0, 2, 3, 1, 2, 3 ) * rcpDet;
           out.M[0][2] =  Matrix4_Minor( m, 0, 1, 3, 1, 2, 3 ) * rcpDet;
           out.M[0][3] = -Matrix4_Minor( m, 0, 1, 2, 1, 2, 3 ) * rcpDet;
           out.M[1][0] = -Matrix4_Minor( m, 1, 2, 3, 0, 2, 3 ) * rcpDet;
           out.M[1][1] =  Matrix4_Minor( m, 0, 2, 3, 0, 2, 3 ) * rcpDet;
           out.M[1][2] = -Matrix4_Minor( m, 0, 1, 3, 0, 2, 3 ) * rcpDet;
           out.M[1][3] =  Matrix4_Minor( m, 0, 1, 2, 0, 2, 3 ) * rcpDet;
           out.M[2][0] =  Matrix4_Minor( m, 1, 2, 3, 0, 1, 3 ) * rcpDet;
           out.M[2][1] = -Matrix4_Minor( m, 0, 2, 3, 0, 1, 3 ) * rcpDet;
           out.M[2][2] =  Matrix4_Minor( m, 0, 1, 3, 0, 1, 3 ) * rcpDet;
           out.M[2][3] = -Matrix4_Minor( m, 0, 1, 2, 0, 1, 3 ) * rcpDet;
           out.M[3][0] = -Matrix4_Minor( m, 1, 2, 3, 0, 1, 2 ) * rcpDet;
           out.M[3][1] =  Matrix4_Minor( m, 0, 2, 3, 0, 1, 2 ) * rcpDet;
           out.M[3][2] = -Matrix4_Minor( m, 0, 1, 3, 0, 1, 2 ) * rcpDet;
           out.M[3][3] =  Matrix4_Minor( m, 0, 1, 2, 0, 1, 2 ) * rcpDet;
           return out;
       }

       // Returns the 4x4 rotation matrix for the given quaternion.
       static VR4Matrix<T> Matrix4_CreateFromQuaternion( const VQuat<T> * q )
       {
           const float ww = q->w * q->w;
           const float xx = q->x * q->x;
           const float yy = q->y * q->y;
           const float zz = q->z * q->z;

           VR4Matrix<T> out;
           out.M[0][0] = ww + xx - yy - zz;
           out.M[0][1] = 2 * ( q->x * q->y - q->w * q->z );
           out.M[0][2] = 2 * ( q->x * q->z + q->w * q->y );
           out.M[0][3] = 0;
           out.M[1][0] = 2 * ( q->x * q->y + q->w * q->z );
           out.M[1][1] = ww - xx + yy - zz;
           out.M[1][2] = 2 * ( q->y * q->z - q->w * q->x );
           out.M[1][3] = 0;
           out.M[2][0] = 2 * ( q->x * q->z - q->w * q->y );
           out.M[2][1] = 2 * ( q->y * q->z + q->w * q->x );
           out.M[2][2] = ww - xx - yy + zz;
           out.M[2][3] = 0;
           out.M[3][0] = 0;
           out.M[3][1] = 0;
           out.M[3][2] = 0;
           out.M[3][3] = 1;
           return out;
       }


       // Convert a standard projection matrix into a TanAngle matrix for
       // the primary time warp surface.
       static VR4Matrix<T> TanAngleMatrixFromProjection( const VR4Matrix<T> * projection )
       {
           // A projection matrix goes from a view point to NDC, or -1 to 1 space.
           // Scale and bias to convert that to a 0 to 1 space.
           const VR4Matrix<T> tanAngleMatrix =
                   { {
                             { 0.5f * projection->M[0][0], 0.5f * projection->M[0][1], 0.5f * projection->M[0][2] - 0.5f, 0.5f * projection->M[0][3] },
                             { 0.5f * projection->M[1][0], 0.5f * projection->M[1][1], 0.5f * projection->M[1][2] - 0.5f, 0.5f * projection->M[1][3] },
                             { 0.0f, 0.0f, -1.0f, 0.0f },
                             { 0.0f, 0.0f, -1.0f, 0.0f }
                     } };
           return tanAngleMatrix;
       }

       // Trivial version of TanAngleMatrixFromProjection() for a symmetric field of view.
       static VR4Matrix<T> TanAngleMatrixFromFov( const float fovDegrees )
       {
           const float tanHalfFov = tanf( 0.5f * fovDegrees * ( M_PI / 180.0f ) );
           const VR4Matrix<T> tanAngleMatrix(0.5f / tanHalfFov, 0.0f, -0.5f, 0.0f,
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
           const VR4Matrix<T> inv = Matrix4_Inverse( modelView );
           VR4Matrix<T> m;
           m.M[0][0] = 0.5f * inv.M[2][0] - 0.5f * ( inv.M[0][0] * inv.M[2][3] - inv.M[0][3] * inv.M[2][0] );
           m.M[0][1] = 0.5f * inv.M[2][1] - 0.5f * ( inv.M[0][1] * inv.M[2][3] - inv.M[0][3] * inv.M[2][1] );
           m.M[0][2] = 0.5f * inv.M[2][2] - 0.5f * ( inv.M[0][2] * inv.M[2][3] - inv.M[0][3] * inv.M[2][2] );
           m.M[0][3] = 0.0f;
           m.M[1][0] = 0.5f * inv.M[2][0] + 0.5f * ( inv.M[1][0] * inv.M[2][3] - inv.M[1][3] * inv.M[2][0] );
           m.M[1][1] = 0.5f * inv.M[2][1] + 0.5f * ( inv.M[1][1] * inv.M[2][3] - inv.M[1][3] * inv.M[2][1] );
           m.M[1][2] = 0.5f * inv.M[2][2] + 0.5f * ( inv.M[1][2] * inv.M[2][3] - inv.M[1][3] * inv.M[2][2] );
           m.M[1][3] = 0.0f;
           m.M[2][0] = m.M[3][0] = inv.M[2][0];
           m.M[2][1] = m.M[3][1] = inv.M[2][1];
           m.M[2][2] = m.M[3][2] = inv.M[2][2];
           m.M[2][3] = m.M[3][3] = 0.0f;
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
           quat.x = viewMatrix->M[0][1] * sinHalfAngle;
           quat.y = viewMatrix->M[1][1] * sinHalfAngle;
           quat.z = viewMatrix->M[2][1] * sinHalfAngle;
           quat.w = cosHalfAngle;
           return Matrix4_CreateFromQuaternion( &quat );
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
		OVR_ASSERT((d != &a) && (d != &b));
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

	V3Vect<T> operator* (const V3Vect<T> &b) const
	{
		V3Vect<T> result;
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

	V3Vect<T> Transform(const V3Vect<T>& v) const
	{
		return V3Vect<T>(M[0][0] * v.x + M[0][1] * v.y + M[0][2] * v.z,
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
	inline void Rank1Add(const V3Vect<T> &a, const V3Vect<T> &b)
	{
		M[0][0] += a.x*b.x;		M[0][1] += a.x*b.y;		M[0][2] += a.x*b.z;
		M[1][0] += a.y*b.x;		M[1][1] += a.y*b.y;		M[1][2] += a.y*b.z;
		M[2][0] += a.z*b.x;		M[2][1] += a.z*b.y;		M[2][2] += a.z*b.z;
	}

	// M -= a*b.t()
	inline void Rank1Sub(const V3Vect<T> &a, const V3Vect<T> &b)
	{
		M[0][0] -= a.x*b.x;		M[0][1] -= a.x*b.y;		M[0][2] -= a.x*b.z;
		M[1][0] -= a.y*b.x;		M[1][1] -= a.y*b.y;		M[1][2] -= a.y*b.z;
		M[2][0] -= a.z*b.x;		M[2][1] -= a.z*b.y;		M[2][2] -= a.z*b.z;
	}

	inline V3Vect<T> Col(int c) const
	{
		return V3Vect<T>(M[0][c], M[1][c], M[2][c]);
	}

	inline V3Vect<T> Row(int r) const
	{
        return V3Vect<T>(M[r][0], M[r][1], M[r][2]);
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
	inline void Rank1(const V3Vect<T> &a)
	{
		v[0] = a.x*a.x; v[1] = a.x*a.y; v[2] = a.x*a.z;
		v[3] = a.y*a.y; v[4] = a.y*a.z;
		v[5] = a.z*a.z;
	}

	// M += a*a.t()
	inline void Rank1Add(const V3Vect<T> &a)
	{
		v[0] += a.x*a.x; v[1] += a.x*a.y; v[2] += a.x*a.z;
		v[3] += a.y*a.y; v[4] += a.y*a.z;
		v[5] += a.z*a.z;
	}

	// M -= a*a.t()
	inline void Rank1Sub(const V3Vect<T> &a)
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

inline V3Vectf GetViewMatrixPosition( VR4Matrixf const & m )
{
    return m.Inverted().GetTranslation();
}

inline V3Vectf GetViewMatrixForward( VR4Matrixf const & m )
{
    return V3Vectf( -m.M[2][0], -m.M[2][1], -m.M[2][2] ).Normalized();
}

NV_NAMESPACE_END
