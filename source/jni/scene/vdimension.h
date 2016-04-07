#ifndef VDimension_H
#define VDimension_H
#include "VBasicmath.h"
#include <math.h>

namespace NervGear
{

template <class T>
    class VDimension
    {
        public:
            //! Default constructor for empty VDimension
            VDimension() : Width(0), Height(0) {}
            //! Constructor with width and height
            VDimension(const T& width, const T& height)
                : Width(width), Height(height) {}

            VDimension(const V2Vect<T>& other); // Defined in VVector.h

            //! Use this constructor only where you are sure that the conversion is valid.
            template <class U>
            explicit VDimension(const VDimension<U>& other) :
                Width((T)other.Width), Height((T)other.Height) { }

            template <class U>
            VDimension<T>& operator=(const VDimension<U>& other)
            {
                Width = (T) other.Width;
                Height = (T) other.Height;
                return *this;
            }


            //! Equality operator
            bool operator==(const VDimension<T>& other) const
            {
                return NervGear::equal(Width, other.Width) &&
                        NervGear::equal(Height, other.Height);
            }

            //! Inequality operator
            bool operator!=(const VDimension<T>& other) const
            {
                return ! (*this == other);
            }

            bool operator==(const V2Vect<T>& other) const;  // Defined in VVector.h

            bool operator!=(const V2Vect<T>& other) const
            {
                return !(*this == other);
            }

            //! Set to new values
            VDimension<T>& set(const T& width, const T& height)
            {
                Width = width;
                Height = height;
                return *this;
            }

            //! Divide width and height by scalar
            VDimension<T>& operator/=(const T& scale)
            {
                Width /= scale;
                Height /= scale;
                return *this;
            }

            //! Divide width and height by scalar
            VDimension<T> operator/(const T& scale) const
            {
                return VDimension<T>(Width/scale, Height/scale);
            }

            //! Multiply width and height by scalar
            VDimension<T>& operator*=(const T& scale)
            {
                Width *= scale;
                Height *= scale;
                return *this;
            }

            //! Multiply width and height by scalar
            VDimension<T> operator*(const T& scale) const
            {
                return VDimension<T>(Width*scale, Height*scale);
            }

            //! Add another VDimension to this one.
            VDimension<T>& operator+=(const VDimension<T>& other)
            {
                Width += other.Width;
                Height += other.Height;
                return *this;
            }

            //! Add two VDimensions
            VDimension<T> operator+(const VDimension<T>& other) const
            {
                return VDimension<T>(Width+other.Width, Height+other.Height);
            }

            //! Subtract a VDimension from this one
            VDimension<T>& operator-=(const VDimension<T>& other)
            {
                Width -= other.Width;
                Height -= other.Height;
                return *this;
            }

            //! Subtract one VDimension from another
            VDimension<T> operator-(const VDimension<T>& other) const
            {
                return VDimension<T>(Width-other.Width, Height-other.Height);
            }

            //! Get area
            T getArea() const
            {
                return Width*Height;
            }

            //! Get the optimal size according to some properties
            /** This is a function often used for texture VDimension
            calculations. The function returns the next larger or
            smaller VDimension which is a power-of-two VDimension
            (2^n,2^m) and/or square (Width=Height).
            \param requirePowerOfTwo Forces the result to use only
            powers of two as values.
            \param requireSquare Makes width==height in the result
            \param larger Choose whether the result is larger or
            smaller than the current VDimension. If one VDimension
            need not be changed it is kept with any value of larger.
            \param maxValue Maximum texturesize. if value > 0 size is
            clamped to maxValue
            \return The optimal VDimension under the given
            constraints. */
            VDimension<T> getOptimalSize(
                    bool requirePowerOfTwo=true,
                    bool requireSquare=false,
                    bool larger=true,
                    uint maxValue = 0) const
            {
                uint i=1;
                uint j=1;
                if (requirePowerOfTwo)
                {
                    while (i<(uint)Width)
                        i<<=1;
                    if (!larger && i!=1 && i!=(uint)Width)
                        i>>=1;
                    while (j<(uint)Height)
                        j<<=1;
                    if (!larger && j!=1 && j!=(uint)Height)
                        j>>=1;
                }
                else
                {
                    i=(uint)Width;
                    j=(uint)Height;
                }

                if (requireSquare)
                {
                    if ((larger && (i>j)) || (!larger && (i<j)))
                        j=i;
                    else
                        i=j;
                }

                if ( maxValue > 0 && i > maxValue)
                    i = maxValue;

                if ( maxValue > 0 && j > maxValue)
                    j = maxValue;

                return VDimension<T>((T)i,(T)j);
            }

            //! Get the interpolated VDimension
            /** \param other Other VDimension to interpolate with.
            \param d Value between 0.0f and 1.0f.
            \return Interpolated VDimension. */
            VDimension<T> getInterpolated(const VDimension<T>& other, float d) const
            {
                float inv = (1.0f - d);
                return VDimension<T>( (T)(other.Width*inv + Width*d), (T)(other.Height*inv + Height*d));
            }


            //! Width of the VDimension.
            T Width;
            //! Height of the VDimension.
            T Height;
    };

    //! Typedef for an float VDimension.
    typedef VDimension<float> VDimensionf;
    //! Typedef for an unsigned integer VDimension.
    typedef VDimension<uint> VDimensionu;

    //! Typedef for an integer VDimension.
    /** There are few cases where negative VDimensions make sense. Please consider using
        VDimensionu instead. */
    typedef VDimension<int> VDimensioni;



}

#endif // VVDimension_H

