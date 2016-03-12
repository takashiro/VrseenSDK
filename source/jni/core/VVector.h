#pragma once

#include "vglobal.h"
#include "VConstants.h"
#include <assert.h>
#include <stdlib.h>
#include <math.h>
#include "Types.h"


//-------------------------------------------------------------------------------------
// ***** V2Vect<>

// V2Vectf (V2Vectd) represents a 2-dimensional vector or point in space,
// consisting of coordinates x and y
namespace NervGear{
template<class T> class V2Vect;
template<class T> class V3Vect;

template<> struct VCompatibleTypes<V2Vect<int> >    { typedef ovrVector2i Type; };
template<> struct VCompatibleTypes<V2Vect<float> >  { typedef ovrVector2f Type; };
template<> struct VCompatibleTypes<V3Vect<float> >  { typedef ovrVector3f Type; };
template<> struct VCompatibleTypes<V3Vect<double> > { typedef ovrVector3d Type; };

template<class T>
class V2Vect
{
public:

	  T x, y;

	         V2Vect() : x(0), y(0) { }
	         V2Vect(T x_, T y_) : x(x_), y(y_) { }
	         explicit V2Vect(T s) : x(s), y(s) { }
	         explicit V2Vect(const V2Vect<typename VConstants<T>::VdifFloat> &src)
	             : x((T)src.x), y((T)src.y) { }

	         static const V2Vect ZERO;
    // C-interop support.
      typedef  typename VCompatibleTypes<V2Vect<T> >::Type VCompatibleType;

      V2Vect(const VCompatibleType& s) : x(s.x), y(s.y) {  }

      operator const VCompatibleType& () const
      {
          OVR_COMPILER_ASSERT(sizeof(V2Vect<T>) == sizeof(VCompatibleType));
          return reinterpret_cast<const VCompatibleType&>(*this);
      }





         bool     operator== (const V2Vect& b) const  { return x == b.x && y == b.y; }
         bool     operator!= (const V2Vect& b) const  { return x != b.x || y != b.y; }

         V2Vect  operator+  (const V2Vect& b) const  { return V2Vect(x + b.x, y + b.y); }
         V2Vect& operator+= (const V2Vect& b)        { x += b.x; y += b.y; return *this; }
         V2Vect  operator-  (const V2Vect& b) const  { return V2Vect(x - b.x, y - b.y); }
         V2Vect& operator-= (const V2Vect& b)        { x -= b.x; y -= b.y; return *this; }
         V2Vect  operator- () const                   { return V2Vect(-x, -y); }

         // Scalar multiplication/division scales vector.
         V2Vect  operator*  (T s) const               { return V2Vect(x*s, y*s); }
         V2Vect& operator*= (T s)                     { x *= s; y *= s; return *this; }

         V2Vect  operator/  (T s) const               { T rcp = T(1)/s;
                                                         return V2Vect(x*rcp, y*rcp); }
         V2Vect& operator/= (T s)                     { T rcp = T(1)/s;
                                                         x *= rcp; y *= rcp;
                                                         return *this; }

         static V2Vect  Min(const V2Vect& a, const V2Vect& b) { return V2Vect((a.x < b.x) ? a.x : b.x,
                                                                                  (a.y < b.y) ? a.y : b.y); }
         static V2Vect  Max(const V2Vect& a, const V2Vect& b) { return V2Vect((a.x > b.x) ? a.x : b.x,
                                                                                  (a.y > b.y) ? a.y : b.y); }

         // Compare two vectors for equality with tolerance. Returns true if vectors match withing tolerance.
         bool	Compare(const V2Vect&b, T tolerance = VConstantsf::Tolerance)
         {
             return (fabs(b.x-x) < tolerance) && (fabs(b.y-y) < tolerance);
         }

         T& operator[] (int idx)
         {
             OVR_ASSERT(0 <= idx && idx < 2);
             return *(&x + idx);
         }

         const T& operator[] (int idx) const
         {
             OVR_ASSERT(0 <= idx && idx < 2);
             return *(&x + idx);
         }

     	// Entrywise product of two vectors
         V2Vect	EntrywiseMultiply(const V2Vect& b) const	{ return V2Vect(x * b.x, y * b.y);}


         // Multiply and divide operators do entry-wise VConstants. Used Dot() for dot product.
         V2Vect  operator*  (const V2Vect& b) const        { return V2Vect(x * b.x,  y * b.y); }
         V2Vect  operator/  (const V2Vect& b) const        { return V2Vect(x / b.x,  y / b.y); }

     	// Dot product
         // Used to calculate angle q between two vectors among other things,
         // as (A dot B) = |a||b|cos(q).
         T		Dot(const V2Vect& b) const                 { return x*b.x + y*b.y; }

         // Returns the angle from this vector to b, in radians.
         T       Angle(const V2Vect& b) const
     	{
     		T div = LengthSq()*b.LengthSq();
     		OVR_ASSERT(div != T(0));
     		T result = Acos((this->Dot(b))/sqrt(div));
     		return result;
     	}

         // Return Length of the vector squared.
         T       LengthSq() const                     { return (x * x + y * y); }

         // Return vector length.
         T       Length() const                       { return sqrt(LengthSq()); }

         // Returns squared distance between two points represented by vectors.
         T       DistanceSq(V2Vect& b) const         { return (*this - b).LengthSq(); }

     	// Returns distance between two points represented by vectors.
         T       Distance(V2Vect& b) const           { return (*this - b).Length(); }

     	// Determine if this a unit vector.
         bool    IsNormalized() const                 { return fabs(LengthSq() - T(1)) < VConstants<T>::Tolerance; }

         // Normalize
         void    Normalize()
     	{
     #if 0	// FIXME: use this safe normalization instead
     		T l = LengthSq();
     		OVR_ASSERT(l >= VConstants<T>::SmallestNonDenormal);
     		*this *= RcpSqrt(l);
     #else
     		T l = Length();
     		OVR_ASSERT(l != T(0));
     		*this /= l;
     #endif
     	}

         // Returns normalized (unit) version of the vector without modifying itself.
         V2Vect Normalized() const
     	{
     #if 0	// FIXME: use this safe normalization instead
     		T l = LengthSq();
     		OVR_ASSERT(l >= VConstants<T>::SmallestNonDenormal);
     		return *this * RcpSqrt(l);
     #else
     		T l = Length();
     		OVR_ASSERT(l != T(0));
     		return *this / l;
     #endif
     	}

         // Linearly interpolates from this vector to another.
         // Factor should be between 0.0 and 1.0, with 0 giving full value to this.
         V2Vect Lerp(const V2Vect& b, T f) const    { return *this*(T(1) - f) + b*f; }

         // Projects this vector onto the argument; in other words,
         // A.Project(B) returns projection of vector A onto B.
         V2Vect ProjectTo(const V2Vect& b) const
     	{
     		T l2 = b.LengthSq();
     		OVR_ASSERT(l2 != T(0));
     		return b * ( Dot(b) / l2 );
     	}
};


typedef V2Vect<float>  V2Vectf;
typedef V2Vect<double> V2Vectd;
typedef V2Vect<int>    V2Vecti;

//-------------------------------------------------------------------------------------
// ***** V3Vect<> - 3D vector of {x, y, z}

//
// V3Vectf (V3Vectd) represents a 3-dimensional vector or point in space,
// consisting of coordinates x, y and z.

template<class T>
class V3Vect
{
public:
	  T x, y, z;

	// FIXME: default initialization of a vector class can be very expensive in a full-blown
	// application.  A few hundred thousand vector constructions is not unlikely and can add
	// up to milliseconds of time on certain processors.
	   V3Vect() : x(0), y(0), z(0) { }
	   V3Vect(T x_, T y_, T z_ = 0) : x(x_), y(y_), z(z_) { }
	   explicit V3Vect(T s) : x(s), y(s), z(s) { }
	   explicit V3Vect(const V3Vect<typename VConstants<T>::VdifFloat> &src)
		   : x((T)src.x), y((T)src.y), z((T)src.z) { }
	   V3Vect( const V2Vect<T> & xy, const T z_ ) : x( xy.x ), y( xy.y ), z( z_ ) { }

	   static const V3Vect ZERO;

    // C-interop support.
       typedef  typename VCompatibleTypes<V3Vect<T> >::Type VCompatibleType;

       V3Vect(const VCompatibleType& s) : x(s.x), y(s.y), z(s.z) {  }

       operator const VCompatibleType& () const
       {
           OVR_COMPILER_ASSERT(sizeof(V3Vect<T>) == sizeof(VCompatibleType));
           return reinterpret_cast<const VCompatibleType&>(*this);
       }



           bool     operator== (const V3Vect& b) const  { return x == b.x && y == b.y && z == b.z; }
           bool     operator!= (const V3Vect& b) const  { return x != b.x || y != b.y || z != b.z; }

           V3Vect  operator+  (const V3Vect& b) const  { return V3Vect(x + b.x, y + b.y, z + b.z); }
           V3Vect& operator+= (const V3Vect& b)        { x += b.x; y += b.y; z += b.z; return *this; }
           V3Vect  operator-  (const V3Vect& b) const  { return V3Vect(x - b.x, y - b.y, z - b.z); }
           V3Vect& operator-= (const V3Vect& b)        { x -= b.x; y -= b.y; z -= b.z; return *this; }
           V3Vect  operator- () const                   { return V3Vect(-x, -y, -z); }

           // Scalar multiplication/division scales vector.
       	V3Vect  operator*  (T s) const               { return V3Vect(x*s, y*s, z*s); }
           V3Vect& operator*= (T s)                     { x *= s; y *= s; z *= s; return *this; }

           V3Vect  operator/  (T s) const               { T rcp = T(1)/s;
                                                           return V3Vect(x*rcp, y*rcp, z*rcp); }
           V3Vect& operator/= (T s)                     { T rcp = T(1)/s;
                                                           x *= rcp; y *= rcp; z *= rcp;
                                                           return *this; }

           static V3Vect  Min(const V3Vect& a, const V3Vect& b)
           {
               return V3Vect((a.x < b.x) ? a.x : b.x,
                              (a.y < b.y) ? a.y : b.y,
                              (a.z < b.z) ? a.z : b.z);
           }
           static V3Vect  Max(const V3Vect& a, const V3Vect& b)
           {
               return V3Vect((a.x > b.x) ? a.x : b.x,
                              (a.y > b.y) ? a.y : b.y,
                              (a.z > b.z) ? a.z : b.z);
           }

           // Compare two vectors for equality with tolerance. Returns true if vectors match withing tolerance.
           bool      Compare(const V3Vect&b, T tolerance = VConstantsf::Tolerance)
           {
               return (fabs(b.x-x) < tolerance) &&
       			   (fabs(b.y-y) < tolerance) &&
       			   (fabs(b.z-z) < tolerance);
           }

           T& operator[] (int idx)
           {
               OVR_ASSERT(0 <= idx && idx < 3);
               return *(&x + idx);
           }

           const T& operator[] (int idx) const
           {
               OVR_ASSERT(0 <= idx && idx < 3);
               return *(&x + idx);
           }

           // Entrywise product of two vectors
           V3Vect	EntrywiseMultiply(const V3Vect& b) const	{ return V3Vect(x * b.x,
       																		 y * b.y,
       																		 z * b.z);}

           // Multiply and divide operators do entry-wise VConstants
           V3Vect  operator*  (const V3Vect& b) const        { return V3Vect(x * b.x,
       																		 y * b.y,
       																		 z * b.z); }

           V3Vect  operator/  (const V3Vect& b) const        { return V3Vect(x / b.x,
       																		 y / b.y,
       																		 z / b.z); }


       	// Dot product
           // Used to calculate angle q between two vectors among other things,
           // as (A dot B) = |a||b|cos(q).
            T      Dot(const V3Vect& b) const          { return x*b.x + y*b.y + z*b.z; }

           // Compute cross product, which generates a normal vector.
           // Direction vector can be determined by right-hand rule: Pointing index finder in
           // direction a and middle finger in direction b, thumb will point in a.Cross(b).
           V3Vect Cross(const V3Vect& b) const        { return V3Vect(y*b.z - z*b.y,
                                                                         z*b.x - x*b.z,
                                                                         x*b.y - y*b.x); }

           // Returns the angle from this vector to b, in radians.
           T       Angle(const V3Vect& b) const
       	{
       		T div = LengthSq()*b.LengthSq();
       		OVR_ASSERT(div != T(0));
       		T result = VArccos((this->Dot(b))/sqrt(div));
       		return result;
       	}

           // Return Length of the vector squared.
           T       LengthSq() const                     { return (x * x + y * y + z * z); }

           // Return vector length.
           T       Length() const                       { return sqrt(LengthSq()); }

           // Returns squared distance between two points represented by vectors.
           T       DistanceSq(V3Vect& b) const         { return (*this - b).LengthSq(); }

           // Returns distance between two points represented by vectors.
           T       Distance(V3Vect const& b) const     { return (*this - b).Length(); }

           // Determine if this a unit vector.
           bool    IsNormalized() const                 { return fabs(LengthSq() - T(1)) < VConstants<T>::Tolerance; }

           // Normalize
           void    Normalize()
       	{
       #if 0	// FIXME: use this safe normalization instead
       		T l = LengthSq();
       		OVR_ASSERT(l >= VConstants<T>::SmallestNonDenormal);
       		*this *= RcpSqrt(l);
       #else
       		T l = Length();
       		OVR_ASSERT(l != T(0));
       		*this /= l;
       #endif
       	}

           // Returns normalized (unit) version of the vector without modifying itself.
           V3Vect Normalized() const
       	{
       #if 0	// FIXME: use this safe normalization instead
       		T l = LengthSq();
       		OVR_ASSERT(l >= VConstants<T>::SmallestNonDenormal);
       		return *this * RcpSqrt(l);
       #else
       		T l = Length();
       		OVR_ASSERT(l != T(0));
       		return *this / l;
       #endif
       	}

           // Linearly interpolates from this vector to another.
           // Factor should be between 0.0 and 1.0, with 0 giving full value to this.
           V3Vect Lerp(const V3Vect& b, T f) const    { return *this*(T(1) - f) + b*f; }

           // Projects this vector onto the argument; in other words,
           // A.Project(B) returns projection of vector A onto B.
           V3Vect ProjectTo(const V3Vect& b) const
       	{
       		T l2 = b.LengthSq();
       		OVR_ASSERT(l2 != T(0));
       		return b * ( Dot(b) / l2 );
       	}

           // Projects this vector onto a plane defined by a normal vector
           V3Vect ProjectToPlane(const V3Vect& normal) const { return *this - this->ProjectTo(normal); }

       	void	Set( const float x, const float y, const float z )
       	{
       		this->x = x;
       		this->y = y;
       		this->z = z;
       	}

       	bool	IsNaN() const { return x != x || y != y || z != z; }
};

// allow multiplication in order scalar * vector (member operator handles vector * scalar)
template<class T>
V3Vect<T> operator* ( T s, const V3Vect<T> & v )
{
	return V3Vect<T>( v.x * s, v.y * s, v.z * s );
}
typedef V3Vect<float>  V3Vectf;
typedef V3Vect<double> V3Vectd;
typedef V3Vect<int>    V3Vecti;


//-------------------------------------------------------------------------------------
// ***** V4Vect<> - 4D vector of {x, y, z, w}

//
// V4Vectf (V4Vectd) represents a 4-dimensional vector,
// consisting of coordinates x, y, z and w.

template<class T>
class V4Vect
{
public:
	 T x, y, z, w;

	// FIXME: default initialization of a vector class can be very expensive in a full-blown
	// application.  A few hundred thousand vector constructions is not unlikely and can add
	// up to milliseconds of time on certain processors.
	  V4Vect() : x(0), y(0), z(0), w(0) { }
	  V4Vect(T x_, T y_, T z_, T w_) : x(x_), y(y_), z(z_), w(w_) { }
	  explicit V4Vect(T s) : x(s), y(s), z(s), w(s) { }
	explicit V4Vect(const V3Vect<T>& v, const float w_=1) : x(v.x), y(v.y), z(v.z), w(w_) { }
	  explicit V4Vect(const V4Vect<typename VConstants<T>::VdifFloat> &src)
		  : x((T)src.x), y((T)src.y), z((T)src.z), w((T)src.w) { }

	  static const V4Vect ZERO;

    // C-interop support.
       typedef  typename VCompatibleTypes< V4Vect<T> >::Type VCompatibleType;

       V4Vect(const VCompatibleType& s) : x(s.x), y(s.y), z(s.z), w(s.w) {  }

       operator const VCompatibleType& () const
       {
           OVR_COMPILER_ASSERT(sizeof(V4Vect<T>) == sizeof(VCompatibleType));
           return reinterpret_cast<const VCompatibleType&>(*this);
       }


          bool     operator== (const V4Vect& b) const  { return x == b.x && y == b.y && z == b.z && w == b.w; }
          bool     operator!= (const V4Vect& b) const  { return x != b.x || y != b.y || z != b.z || w != b.w; }

          V4Vect  operator+  (const V4Vect& b) const  { return V4Vect(x + b.x, y + b.y, z + b.z, w + b.w); }
          V4Vect& operator+= (const V4Vect& b)        { x += b.x; y += b.y; z += b.z; w += b.w; return *this; }
          V4Vect  operator-  (const V4Vect& b) const  { return V4Vect(x - b.x, y - b.y, z - b.z, w - b.w); }
          V4Vect& operator-= (const V4Vect& b)        { x -= b.x; y -= b.y; z -= b.z; w -= b.w; return *this; }
          V4Vect  operator- () const                   { return V4Vect(-x, -y, -z, -w); }

          // Scalar multiplication/division scales vector.
          V4Vect  operator*  (T s) const               { return V4Vect(x*s, y*s, z*s, w*s); }
          V4Vect& operator*= (T s)                     { x *= s; y *= s; z *= s; w *= s;return *this; }

          V4Vect  operator/  (T s) const               { T rcp = T(1)/s;
                                                          return V4Vect(x*rcp, y*rcp, z*rcp, w*rcp); }
          V4Vect& operator/= (T s)                     { T rcp = T(1)/s;
                                                          x *= rcp; y *= rcp; z *= rcp; w *= rcp;
                                                          return *this; }

          static V4Vect  Min(const V4Vect& a, const V4Vect& b)
          {
              return V4Vect((a.x < b.x) ? a.x : b.x,
                             (a.y < b.y) ? a.y : b.y,
                             (a.z < b.z) ? a.z : b.z,
      					   (a.w < b.w) ? a.w : b.w);
          }
          static V4Vect  Max(const V4Vect& a, const V4Vect& b)
          {
              return V4Vect((a.x > b.x) ? a.x : b.x,
                             (a.y > b.y) ? a.y : b.y,
                             (a.z > b.z) ? a.z : b.z,
      					   (a.w > b.w) ? a.w : b.w);
          }

          // Compare two vectors for equality with tolerance. Returns true if vectors match withing tolerance.
          bool      Compare(const V4Vect&b, T tolerance = VConstantsf::Tolerance)
          {
              return (fabs(b.x-x) < tolerance) &&
      			   (fabs(b.y-y) < tolerance) &&
      			   (fabs(b.z-z) < tolerance) &&
      			   (fabs(b.w-w) < tolerance);
          }

          T& operator[] (int idx)
          {
              OVR_ASSERT(0 <= idx && idx < 4);
              return *(&x + idx);
          }

          const T& operator[] (int idx) const
          {
              OVR_ASSERT(0 <= idx && idx < 4);
              return *(&x + idx);
          }

          // Entrywise product of two vectors
          V4Vect	EntrywiseMultiply(const V4Vect& b) const	{ return V4Vect(x * b.x,
      																		 y * b.y,
      																		 z * b.z);}

          // Multiply and divide operators do entry-wise VConstants
          V4Vect  operator*  (const V4Vect& b) const        { return V4Vect(x * b.x,
      																		 y * b.y,
      																		 z * b.z,
      																		 w * b.w); }

          V4Vect  operator/  (const V4Vect& b) const        { return V4Vect(x / b.x,
      																		 y / b.y,
      																		 z / b.z,
      																		 w / b.w); }


      	// Dot product
          T      Dot(const V4Vect& b) const          { return x*b.x + y*b.y + z*b.z + w*b.w; }

          // Return Length of the vector squared.
          T       LengthSq() const                     { return (x * x + y * y + z * z + w * w); }

          // Return vector length.
          T       Length() const                       { return sqrt(LengthSq()); }

          // Determine if this a unit vector.
          bool    IsNormalized() const                 { return fabs(LengthSq() - T(1)) < VConstants<T>::Tolerance; }

          // Normalize
          void    Normalize()
      	{
      #if 0	// FIXME: use this safe normalization instead
      		T l = LengthSq();
      		OVR_ASSERT(l >= VConstants<T>::SmallestNonDenormal);
      		*this *= RcpSqrt(l);
      #else
      		T l = Length();
      		OVR_ASSERT(l != T(0));
      		*this /= l;
      #endif
      	}

          // Returns normalized (unit) version of the vector without modifying itself.
          V4Vect Normalized() const
      	{
      #if 0	// FIXME: use this safe normalization instead
      		T l = LengthSq();
      		OVR_ASSERT(l >= VConstants<T>::SmallestNonDenormal);
      		return *this * RcpSqrt(l);
      #else
      		T l = Length();
      		OVR_ASSERT(l != T(0));
      		return *this / l;
      #endif
      	}

      	bool	IsNaN( ) const { return x != x || y != y || z != z || w != w; }
};

typedef V4Vect<float>  V4Vectf;
typedef V4Vect<double> V4Vectd;
typedef V4Vect<int>    V4Vecti;

}
