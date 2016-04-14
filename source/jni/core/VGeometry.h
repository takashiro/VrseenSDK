#pragma once

#include "VConstants.h"
#include "VVector.h"
#include "VMatrix.h"
#include "VSize.h"

#include <assert.h>
#include <stdlib.h>

namespace NervGear {

template<class T> class VSize;
template<class T> class VRect;
template<class T> class VAngle;


template <class T>
class VBox
{
public:
	enum InitType { Init };

	V3Vect<T>	b[2];

	VBox()
	{
	}

	VBox( const InitType init )
	{
		Clear();
	}

	VBox( const V3Vect<T> & mins, const V3Vect<T> & maxs )
	{
		b[0] = mins;
		b[1] = maxs;
	}

	VBox( const T minx, const T miny, const T minz,
			const T maxx, const T maxy, const T maxz )
	{
		b[0].x = minx;
		b[0].y = miny;
		b[0].z = minz;

		b[1].x = maxx;
		b[1].y = maxy;
		b[1].z = maxz;
	}

	VBox operator * ( float const s ) const
	{
		return VBox<T>(  b[0].x * s, b[0].y * s, b[0].z * s,
							b[1].x * s, b[1].y * s, b[1].z * s );
	}

	VBox operator * ( V3Vect<T> const & s ) const
	{
		return VBox<T>(  b[0].x * s.x, b[0].y * s.y, b[0].z * s.z,
							b[1].x * s.x, b[1].y * s.y, b[1].z * s.z );
	}

	void Clear()
	{
		b[0].x = b[0].y = b[0].z = VConstants<T>::MaxValue;
		b[1].x = b[1].y = b[1].z = -VConstants<T>::MaxValue;
	}

	void AddPoint( const V3Vect<T> & v )
	{
		b[0].x = std::min( b[0].x, v.x );
		b[0].y = std::min( b[0].y, v.y );
		b[0].z = std::min( b[0].z, v.z );
		b[1].x = std::max( b[1].x, v.x );
		b[1].y = std::max( b[1].y, v.y );
		b[1].z = std::max( b[1].z, v.z );
	}

	// return a bounds representing the union of a and b
	static VBox Union( const VBox & a, const VBox & b )
	{
		return VBox( std::min( a.b[0].x, b.b[0].x ),
						std::min( a.b[0].y, b.b[0].y ),
						std::min( a.b[0].z, b.b[0].z ),
						std::max( a.b[1].x, b.b[1].x ),
						std::max( a.b[1].y, b.b[1].y ),
						std::max( a.b[1].z, b.b[1].z ) );
	}

	const V3Vect<T> & GetMins() const { return b[0]; }
	const V3Vect<T> & GetMaxs() const { return b[1]; }

	V3Vect<T> & GetMins() { return b[0]; }
	V3Vect<T> & GetMaxs() { return b[1]; }

	V3Vect<T>	GetSize() const { return V3Vectf( b[1].x - b[0].x, b[1].y - b[0].y, b[1].z - b[0].z ); }

	V3Vect<T>	GetCenter() const { return V3Vectf( ( b[0].x + b[1].x ) * 0.5f, ( b[0].y + b[1].y ) * 0.5f, ( b[0].z + b[1].z ) * 0.5f ); }

	void Translate( const V3Vect<T> & t )
	{
		b[0] += t;
		b[1] += t;
	}

	bool IsInverted() const
	{
		return	b[0].x > b[1].x &&
				b[0].y > b[1].y &&
				b[0].z > b[1].z;
	}

	bool Contains(const V3Vect<T> &point, T expand = 0) const
	{
		return point.x >= b[0].x - expand && point.y >= b[0].y - expand && point.z >= b[0].z - expand && point.x <= b[1].x + expand && point.y <= b[1].y + expand && point.z <= b[1].z + expand;
	}

    // returns the axial aligned bounds of inputBounds after transformation by VPos
	static VBox Transform( const VPos<T> & VPos, const VBox<T> & inBounds )
	{
		const VR3Matrix<T> rotation( VPos.Orientation );
		const V3Vect<T> center = ( inBounds.b[0] + inBounds.b[1] ) * 0.5f;
		const V3Vect<T> extents = inBounds.b[1] - center;
		const V3Vect<T> newCenter = VPos.Position + rotation * center;
		const V3Vect<T> newExtents(
			fabs( extents[0] * rotation.M[0][0] ) + fabs( extents[1] * rotation.M[0][1] ) + fabs( extents[2] * rotation.M[0][2] ),
			fabs( extents[0] * rotation.M[1][0] ) + fabs( extents[1] * rotation.M[1][1] ) + fabs( extents[2] * rotation.M[1][2] ),
			fabs( extents[0] * rotation.M[2][0] ) + fabs( extents[1] * rotation.M[2][1] ) + fabs( extents[2] * rotation.M[2][2] ) );
		return VBox<T>( newCenter - newExtents, newCenter + newExtents );
	}

	static VBox<T> Expand( const VBox<T> & b, const V3Vect<T> & minExpand, const V3Vect<T> & maxExpand )
	{
		return VBox<T>( b.GetMins() + minExpand, b.GetMaxs() + maxExpand );
	}
};

typedef VBox<float>	VBoxf;
typedef VBox<double> VBoxd;


//-------------------------------------------------------------------------------------
// ***** VSize

// VSize class represents 2D VSize with Width, Height components.
// Used to describe distentions of render targets, etc.

typedef VSize<int>       VSizei;
typedef VSize<unsigned>  VSizeu;
typedef VSize<float>     VSizef;
typedef VSize<double>    VSized;



//-----------------------------------------------------------------------------------
// ***** VRect

// VRect describes a VRectangular area for rendering, that includes position and VSize.

template<class T>
class VRect
{
public:
	T x, y;
    T w, h;

    VRect() { }
    VRect(T x1, T y1, T w1, T h1)                   : x(x1), y(y1), w(w1), h(h1) { }
    VRect(const V2Vect<T>& pos, const VSize<T>& sz) : x(pos.x), y(pos.y), w(sz.w), h(sz.h) { }
    VRect(const VSize<T>& sz)                        : x(0), y(0), w(sz.w), h(sz.h) { }

   V2Vect<T> GetPos() const                { return V2Vect<T>(x, y); }
   VSize<T>    GetSize() const               { return VSize<T>(w, h); }
   void       SetPos(const V2Vect<T>& pos) { x = pos.x; y = pos.y; }
   void       SetSize(const VSize<T>& sz)    { w = sz.w; h = sz.h; }

   bool operator == (const VRect& vp) const
   { return (x == vp.x) && (y == vp.y) && (w == vp.w) && (h == vp.h); }
   bool operator != (const VRect& vp) const
   { return !operator == (vp); }
};
typedef VRect<int> VRecti;


//-------------------------------------------------------------------------------------//


// ***** VAngle

// Cleanly representing the algebra of 2D rotations.
// The operations maintain the VAngle between -Pi and Pi, the same range as atan2.

template<class T>
class VAngle
{
public:
	enum AngularUnits
		{
			Radians = 0,
			Degrees = 1
		};

	    VAngle() : a(0) {}

		// Fix the range to be between -Pi and Pi
        VAngle(T a_, AngularUnits u = Radians) : a((u == Radians) ? a_ : a_*VConstants<T>::VDTR) { FixRange(); }

        T    Get(AngularUnits u = Radians) const       { return (u == Radians) ? a : a*VConstants<T>::VRTD; }
        void Set(const T& x, AngularUnits u = Radians) { a = (u == Radians) ? x : x*VConstants<T>::VDTR; FixRange(); }
		int Sign() const                               { if (a == 0) return 0; else return (a > 0) ? 1 : -1; }
		T   Abs() const                                { return (a > 0) ? a : -a; }

	    bool operator== (const VAngle& b) const    { return a == b.a; }
	    bool operator!= (const VAngle& b) const    { return a != b.a; }
	//	bool operator<  (const VAngle& b) const    { return a < a.b; }
	//	bool operator>  (const VAngle& b) const    { return a > a.b; }
	//	bool operator<= (const VAngle& b) const    { return a <= a.b; }
	//	bool operator>= (const VAngle& b) const    { return a >= a.b; }
	//	bool operator= (const T& x)               { a = x; FixRange(); }

		// These operations assume a is already between -Pi and Pi.
		VAngle& operator+= (const VAngle& b)        { a = a + b.a; FastFixRange(); return *this; }
		VAngle& operator+= (const T& x)            { a = a + x; FixRange(); return *this; }
	    VAngle  operator+  (const VAngle& b) const  { VAngle res = *this; res += b; return res; }
		VAngle  operator+  (const T& x) const      { VAngle res = *this; res += x; return res; }
		VAngle& operator-= (const VAngle& b)        { a = a - b.a; FastFixRange(); return *this; }
		VAngle& operator-= (const T& x)            { a = a - x; FixRange(); return *this; }
		VAngle  operator-  (const VAngle& b) const  { VAngle res = *this; res -= b; return res; }
		VAngle  operator-  (const T& x) const      { VAngle res = *this; res -= x; return res; }

		T   Distance(const VAngle& b)              { T c = fabs(a - b.a); return (c <= VConstants<T>::Pi) ? c : VConstants<T>::TwoPi - c; }

	private:

		// The stored VAngle, which should be maintained between -Pi and Pi
		T a;

		// Fixes the VAngle range to [-Pi,Pi], but assumes no more than 2Pi away on either side
		inline void FastFixRange()
		{
			if (a < -VConstants<T>::Pi)
				a += VConstants<T>::TwoPi;
			else if (a > VConstants<T>::Pi)
				a -= VConstants<T>::TwoPi;
		}

		// Fixes the VAngle range to [-Pi,Pi] for any given range, but slower then the fast method
		inline void FixRange()
		{
	        // do nothing if the value is already in the corVRect range, since fmod call is expensive
	        if (a >= -VConstants<T>::Pi && a <= VConstants<T>::Pi)
	            return;
			a = fmod(a,VConstants<T>::TwoPi);
			if (a < -VConstants<T>::Pi)
				a += VConstants<T>::TwoPi;
			else if (a > VConstants<T>::Pi)
				a -= VConstants<T>::TwoPi;
		}
};


typedef VAngle<float>  VAnglef;
typedef VAngle<double> VAngled;


//-------------------------------------------------------------------------------------
// ***** VPlane

// Consists of a normal vector and distance from the origin where the VPlane is located.

template<class T>
class VPlane
{
public:
		V3Vect<T> N;
	    T          D;

	    VPlane() : D(0) {}

	    // Normals must already be normalized
	    VPlane(const V3Vect<T>& n, T d) : N(n), D(d) {}
	    VPlane(T x, T y, T z, T d) : N(x,y,z), D(d) {}

	    // construct from a point on the VPlane and the normal
	    VPlane(const V3Vect<T>& p, const V3Vect<T>& n) : N(n), D(-(p * n)) {}

	    // Find the point to VPlane distance. The sign indicates what side of the VPlane the point is on (0 = point on VPlane).
	    T TestSide(const V3Vect<T>& p) const
	    {
	        return (N.Dot(p)) + D;
	    }

	    VPlane<T> Flipped() const
	    {
	        return VPlane(-N, -D);
	    }

	    void Flip()
	    {
	        N = -N;
	        D = -D;
	    }

		bool operator==(const VPlane<T>& rhs) const
		{
			return (this->D == rhs.D && this->N == rhs.N);
		}
};

typedef VPlane<float> VPlanef;
typedef VPlane<double> VPlaned;
}
