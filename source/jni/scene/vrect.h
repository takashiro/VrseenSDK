#ifndef VRectangle_H
#define VRectangle_H
#include "VDimension.h"

namespace NervGear
{

    template <class T>
    class VRectangle
    {
    public:

        //! Default constructor creating empty VRectangleangle at (0,0)
        VRectangle() : UpperLeftCorner(0,0), LowerRightCorner(0,0) {}

        //! Constructor with two corners
        VRectangle(T x, T y, T x2, T y2)
            : UpperLeftCorner(x,y), LowerRightCorner(x2,y2) {}

        //! Constructor with two corners
        VRectangle(const V2Vect<T>& upperLeft, const V2Vect<T>& lowerRight)
            : UpperLeftCorner(upperLeft), LowerRightCorner(lowerRight) {}

        //! Constructor with upper left corner and dimension
        template <class U>
        VRectangle(const V2Vect<T>& pos, const VDimension<U>& size)
            : UpperLeftCorner(pos), LowerRightCorner(pos.x + size.Width, pos.y + size.Height) {}

        //! move right by given numbers
        VRectangle<T> operator+(const V2Vect<T>& pos) const
        {
            VRectangle<T> ret(*this);
            return ret+=pos;
        }

        //! move right by given numbers
        VRectangle<T>& operator+=(const V2Vect<T>& pos)
        {
            UpperLeftCorner += pos;
            LowerRightCorner += pos;
            return *this;
        }

        //! move left by given numbers
        VRectangle<T> operator-(const V2Vect<T>& pos) const
        {
            VRectangle<T> ret(*this);
            return ret-=pos;
        }

        //! move left by given numbers
        VRectangle<T>& operator-=(const V2Vect<T>& pos)
        {
            UpperLeftCorner -= pos;
            LowerRightCorner -= pos;
            return *this;
        }

        //! equality operator
        bool operator==(const VRectangle<T>& other) const
        {
            return (UpperLeftCorner == other.UpperLeftCorner &&
                LowerRightCorner == other.LowerRightCorner);
        }

        //! inequality operator
        bool operator!=(const VRectangle<T>& other) const
        {
            return (UpperLeftCorner != other.UpperLeftCorner ||
                LowerRightCorner != other.LowerRightCorner);
        }

        //! compares size of VRectangleangles
        bool operator<(const VRectangle<T>& other) const
        {
            return getArea() < other.getArea();
        }

        //! Returns size of VRectangleangle
        T getArea() const
        {
            return getWidth() * getHeight();
        }

        //! Returns if a 2d point is within this VRectangleangle.
        /** \param pos Position to test if it lies within this VRectangleangle.
        \return True if the position is within the VRectangleangle, false if not. */
        bool isPointInside(const V2Vect<T>& pos) const
        {
            return (UpperLeftCorner.x <= pos.x &&
                UpperLeftCorner.y <= pos.y &&
                LowerRightCorner.x >= pos.x &&
                LowerRightCorner.y >= pos.y);
        }

        //! Check if the VRectangleangle collides with another VRectangleangle.
        /** \param other VRectangleangle to test collision with
        \return True if the VRectangleangles collide. */
        bool isVRectangleCollided(const VRectangle<T>& other) const
        {
            return (LowerRightCorner.y > other.UpperLeftCorner.y &&
                UpperLeftCorner.y < other.LowerRightCorner.y &&
                LowerRightCorner.x > other.UpperLeftCorner.x &&
                UpperLeftCorner.x < other.LowerRightCorner.x);
        }

        //! Clips this VRectangleangle with another one.
        /** \param other VRectangleangle to clip with */
        void clipAgainst(const VRectangle<T>& other)
        {
            if (other.LowerRightCorner.x < LowerRightCorner.x)
                LowerRightCorner.x = other.LowerRightCorner.x;
            if (other.LowerRightCorner.y < LowerRightCorner.y)
                LowerRightCorner.y = other.LowerRightCorner.y;

            if (other.UpperLeftCorner.x > UpperLeftCorner.x)
                UpperLeftCorner.x = other.UpperLeftCorner.x;
            if (other.UpperLeftCorner.y > UpperLeftCorner.y)
                UpperLeftCorner.y = other.UpperLeftCorner.y;

            // corVRectangle possible invalid VRectangle resulting from clipping
            if (UpperLeftCorner.y > LowerRightCorner.y)
                UpperLeftCorner.y = LowerRightCorner.y;
            if (UpperLeftCorner.x > LowerRightCorner.x)
                UpperLeftCorner.x = LowerRightCorner.x;
        }

        //! Moves this VRectangleangle to fit inside another one.
        /** \return True on success, false if not possible */
        bool constrainTo(const VRectangle<T>& other)
        {
            if (other.getWidth() < getWidth() || other.getHeight() < getHeight())
                return false;

            T diff = other.LowerRightCorner.x - LowerRightCorner.x;
            if (diff < 0)
            {
                LowerRightCorner.x += diff;
                UpperLeftCorner.x  += diff;
            }

            diff = other.LowerRightCorner.y - LowerRightCorner.y;
            if (diff < 0)
            {
                LowerRightCorner.y += diff;
                UpperLeftCorner.y  += diff;
            }

            diff = UpperLeftCorner.x - other.UpperLeftCorner.x;
            if (diff < 0)
            {
                UpperLeftCorner.x  -= diff;
                LowerRightCorner.x -= diff;
            }

            diff = UpperLeftCorner.y - other.UpperLeftCorner.y;
            if (diff < 0)
            {
                UpperLeftCorner.y  -= diff;
                LowerRightCorner.y -= diff;
            }

            return true;
        }

        //! Get width of VRectangleangle.
        T getWidth() const
        {
            return LowerRightCorner.x - UpperLeftCorner.x;
        }

        //! Get height of VRectangleangle.
        T getHeight() const
        {
            return LowerRightCorner.y - UpperLeftCorner.y;
        }

        //! If the lower right corner of the VRectangle is smaller then the upper left, the points are swapped.
        void repair()
        {
            if (LowerRightCorner.x < UpperLeftCorner.x)
            {
                T t = LowerRightCorner.x;
                LowerRightCorner.x = UpperLeftCorner.x;
                UpperLeftCorner.x = t;
            }

            if (LowerRightCorner.y < UpperLeftCorner.y)
            {
                T t = LowerRightCorner.y;
                LowerRightCorner.y = UpperLeftCorner.y;
                UpperLeftCorner.y = t;
            }
        }

        //! Returns if the VRectangle is valid to draw.
        /** It would be invalid if the UpperLeftCorner is lower or more
        right than the LowerRightCorner. */
        bool isValid() const
        {
            return ((LowerRightCorner.x >= UpperLeftCorner.x) &&
                (LowerRightCorner.y >= UpperLeftCorner.y));
        }

        //! Get the center of the VRectangleangle
        V2Vect<T> getCenter() const
        {
            return V2Vect<T>(
                    (UpperLeftCorner.x + LowerRightCorner.x) / 2,
                    (UpperLeftCorner.y + LowerRightCorner.y) / 2);
        }

        //! Get the dimensions of the VRectangleangle
        VDimension<T> getSize() const
        {
            return VDimension<T>(getWidth(), getHeight());
        }


        //! Adds a point to the VRectangleangle
        /** Causes the VRectangleangle to grow bigger if point is outside of
        the box
        \param p Point to add to the box. */
        void addInternalPoint(const V2Vect<T>& p)
        {
            addInternalPoint(p.x, p.y);
        }

        //! Adds a point to the bounding VRectangleangle
        /** Causes the VRectangleangle to grow bigger if point is outside of
        the box
        \param x X-Coordinate of the point to add to this box.
        \param y Y-Coordinate of the point to add to this box. */
        void addInternalPoint(T x, T y)
        {
            if (x>LowerRightCorner.x)
                LowerRightCorner.x = x;
            if (y>LowerRightCorner.y)
                LowerRightCorner.y = y;

            if (x<UpperLeftCorner.x)
                UpperLeftCorner.x = x;
            if (y<UpperLeftCorner.y)
                UpperLeftCorner.y = y;
        }

        //! Upper left corner
        V2Vect<T> UpperLeftCorner;
        //! Lower right corner
        V2Vect<T> LowerRightCorner;
    };

    //! VRectangleangle with float values
    typedef VRectangle<float> VRectanglef;
    //! VRectangleangle with int values
    typedef VRectangle<int> VRectanglei;


}


#endif // VVRectangle_H

